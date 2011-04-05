/* v3ChannelGet.cpp */
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


#include <pvData.h>
#include <pvAccess.h>

#include "pvDatabase.h"
#include "v3Channel.h"
#include "v3Util.h"


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelGet::V3ChannelGet(
    V3Channel &v3Channel,
    ChannelGetRequester &channelGetRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),channelGetRequester(channelGetRequester),
  dbAddr(dbAddr),
  getListNode(*this),
  process(false),
  firstTime(true),
  propertyMask(0),
  pvStructure(0),bitSet(0),
  pNotify(0),
  notifyAddr(0),
  event()
{
}

V3ChannelGet::~V3ChannelGet() {}


ChannelGetListNode * V3ChannelGet::init(PVStructure &pvRequest)
{
    propertyMask = V3Util::getProperties(channelGetRequester,pvRequest,dbAddr);
    pvStructure =  std::auto_ptr<PVStructure>(
        V3Util::createPVStructure(channelGetRequester, propertyMask, dbAddr));
    V3Util::getPropertyData(
        channelGetRequester,propertyMask,dbAddr,*pvStructure);
    int numFields = pvStructure->getStructure()->getNumberFields();
    bitSet = std::auto_ptr<BitSet>(new BitSet(numFields));
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
    channelGetRequester.channelGetConnect(
       Status::OK,this,pvStructure.get(),bitSet.get());
    return &getListNode;
}

String V3ChannelGet::getRequesterName() {
    return channelGetRequester.getRequesterName();
}

void V3ChannelGet::message(String message,MessageType messageType)
{
    channelGetRequester.message(message,messageType);
}

void V3ChannelGet::destroy() {
    v3Channel.removeChannelGet(getListNode);
    delete this;
}

void V3ChannelGet::get(bool lastRequest)
{
    if(process) {
        epicsUInt8 value = 1;
        pNotify.get()->pbuffer = &value;
        dbPutNotify(pNotify.get());
        event.wait();
    }
    bitSet->clear();
    dbScanLock(dbAddr.precord);
    Status status = V3Util::get(
        channelGetRequester,propertyMask,dbAddr,*pvStructure,*bitSet,0);
    dbScanUnlock(dbAddr.precord);
    if(firstTime) {
        firstTime = false;
        bitSet->clear();
        bitSet->set(0);
    }
    channelGetRequester.getDone(status);
    if(lastRequest) destroy();
}

void V3ChannelGet::notifyCallback(struct putNotify *pn)
{
    V3ChannelGet * cget = static_cast<V3ChannelGet *>(pn->usrPvt);
    cget->event.signal();
}

}}

