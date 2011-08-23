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


#include <pv/pvData.h>
#include <pv/pvAccess.h>

#include <pv/pvDatabase.h>
#include <pv/v3Channel.h>
#include "v3Util.h"


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelGet::V3ChannelGet(
    PVServiceBase::shared_pointer const &v3Channel,
    ChannelGetRequester::shared_pointer const &channelGetRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),
  channelGetRequester(channelGetRequester),
  dbAddr(dbAddr),
  process(false),
  firstTime(true),
  propertyMask(0),
  pvStructure(),
  bitSet(),
  pNotify(0),
  notifyAddr(0),
  event()
{
//printf("V3ChannelGet construct\n");
}

V3ChannelGet::~V3ChannelGet()
{
//printf("V3ChannelGet destruct\n");
}


bool V3ChannelGet::init(PVStructure::shared_pointer const &pvRequest)
{
    propertyMask = V3Util::getProperties(
        channelGetRequester,
        pvRequest,
        dbAddr);
    if(propertyMask==V3Util::noAccessBit) return false;
    pvStructure =  PVStructure::shared_pointer(
        V3Util::createPVStructure(
             channelGetRequester, propertyMask, dbAddr));
    if(pvStructure.get()==0) return 0;
    V3Util::getPropertyData( channelGetRequester,propertyMask,dbAddr,pvStructure);
    int numFields = pvStructure->getNumberFields();
    bitSet.reset(new BitSet(numFields));
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
    channelGetRequester->channelGetConnect(
       Status::OK,
       getPtrSelf(),
       pvStructure,
       bitSet);
    return true;
}

String V3ChannelGet::getRequesterName() {
    return channelGetRequester->getRequesterName();
}

void V3ChannelGet::message(String message,MessageType messageType)
{
    channelGetRequester->message(message,messageType);
}

void V3ChannelGet::destroy() {
//printf("V3ChannelGet::destroy\n");
    v3Channel->removeChannelGet(*this);
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
        channelGetRequester,
        propertyMask,dbAddr,
        pvStructure,
        bitSet,
        0);
    dbScanUnlock(dbAddr.precord);
    if(firstTime) {
        firstTime = false;
        bitSet->clear();
        bitSet->set(0);
    }
    channelGetRequester->getDone(status);
    if(lastRequest) destroy();
}

void V3ChannelGet::notifyCallback(struct putNotify *pn)
{
    V3ChannelGet * cget = static_cast<V3ChannelGet *>(pn->usrPvt);
    cget->event.signal();
}

}}

