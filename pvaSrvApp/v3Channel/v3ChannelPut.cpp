/* vsChannelPut.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/* Marty Kraimer 2011.03 */
/* This connects to a V3 record and presents the data as a PVStructure
 * It provides access to  value, alarm, display, and control.
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <memory>

#include <dbAccess.h>
#include <dbEvent.h>
#include <dbNotify.h>
#include <dbCommon.h>

#include <pvIntrospect.h>
#include <pvData.h>
#include <pvAccess.h>

#include "pvDatabase.h"
#include "v3Channel.h"
#include "v3Util.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;


V3ChannelPut::V3ChannelPut(
    V3Channel &v3Channel,
    ChannelPutRequester &channelPutRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),channelPutRequester(channelPutRequester),
  dbAddr(dbAddr),
  putListNode(*this),
  propertyMask(0),
  process(false),
  firstTime(true),
  pvStructure(0),bitSet(0),
  pNotify(0),
  notifyAddr(0),
  event()
{
}

V3ChannelPut::~V3ChannelPut() {}


ChannelPutListNode * V3ChannelPut::init(PVStructure &pvRequest)
{
    propertyMask = V3Util::getProperties(channelPutRequester,pvRequest,dbAddr);
    propertyMask &= (
         V3Util::processBit
        |V3Util::shareArrayBit
        |V3Util::scalarValueBit
        |V3Util::arrayValueBit
        |V3Util::enumValueBit
    );
    pvStructure =  std::auto_ptr<PVStructure>(
        V3Util::createPVStructure(channelPutRequester, propertyMask, dbAddr));
    if((propertyMask&V3Util::processBit)!=0) {
       process = true;
       pNotify = std::auto_ptr<struct putNotify>(new (struct putNotify)());
       notifyAddr = std::auto_ptr<DbAddr>(new DbAddr());
       memcpy(notifyAddr.get(),&dbAddr,sizeof(DbAddr));
       DbAddr *paddr = notifyAddr.get();
       struct dbCommon *precord = paddr->precord;
       char buffer[sizeof(precord->name) + 10];
       strcpy(buffer,precord->name);
       strcat(buffer,".PROC");
       if(dbNameToAddr(buffer,paddr)!=0) {
            throw std::logic_error(String("dbNameToAddr failed"));
       }
       struct putNotify *pn = pNotify.get();
       pn->userCallback = this->notifyCallback;
       pn->paddr = paddr;
       pn->nRequest = 1;
       pn->dbrType = DBR_CHAR;
       pn->usrPvt = this;
    }
    int numFields = pvStructure->getStructure()->getNumberFields();
    bitSet = std::auto_ptr<BitSet>(new BitSet(numFields));
    channelPutRequester.channelPutConnect(
       Status::OK,this,pvStructure.get(),bitSet.get());
    return &putListNode;
}

String V3ChannelPut::getRequesterName() {
    return channelPutRequester.getRequesterName();
}

void V3ChannelPut::message(String message,MessageType messageType)
{
    channelPutRequester.message(message,messageType);
}

void V3ChannelPut::destroy() {
    v3Channel.removeChannelPut(putListNode);
    delete this;
}

void V3ChannelPut::put(bool lastRequest)
{
    dbScanLock(dbAddr.precord);
    Status status = V3Util::put(
        channelPutRequester,propertyMask,dbAddr,*pvStructure);
    dbScanUnlock(dbAddr.precord);
    if(process) {
        epicsUInt8 value = 1;
        pNotify.get()->pbuffer = &value;
        dbPutNotify(pNotify.get());
        event.wait();
    }
    dbFldDes *pfldDes = dbAddr.pfldDes;
    struct dbCommon *precord = dbAddr.precord;
    int isValueField = dbIsValueField(pfldDes);
    if (isValueField) precord->udf = FALSE;
    if (precord->mlis.count &&
        !(isValueField && pfldDes->process_passive))
        db_post_events(precord, dbAddr.pfield, DBE_VALUE | DBE_LOG);
    channelPutRequester.putDone(status);
    if(lastRequest) destroy();
}

void V3ChannelPut::notifyCallback(struct putNotify *pn)
{
    V3ChannelPut * cput = static_cast<V3ChannelPut *>(pn->usrPvt);
    cput->event.signal();
}

void V3ChannelPut::get()
{
    bitSet->clear();
    dbScanLock(dbAddr.precord);
    Status status = V3Util::get(
        channelPutRequester,propertyMask,dbAddr,*pvStructure,*bitSet,0);
    if(firstTime) {
        firstTime = false;
        bitSet->set(pvStructure->getFieldOffset());
    }
    dbScanUnlock(dbAddr.precord);
    channelPutRequester.getDone(Status::OK);
}

}}

