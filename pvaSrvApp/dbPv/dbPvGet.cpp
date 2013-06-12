/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
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

#include "dbPv.h"
#include "dbUtil.h"

namespace epics { namespace pvaSrv { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelGet::V3ChannelGet(
    ChannelBase::shared_pointer const &v3Channel,
    ChannelGetRequester::shared_pointer const &channelGetRequester,
    DbAddr &dbAddr)
: v3Util(V3Util::getV3Util()),
  v3Channel(v3Channel),
  channelGetRequester(channelGetRequester),
  dbAddr(dbAddr),
  process(false),
  firstTime(true),
  propertyMask(0),
  beingDestroyed(false)
{
    if(V3ChannelDebug::getLevel()>0)printf("V3ChannelGet::V3ChannelGet\n");
}

V3ChannelGet::~V3ChannelGet()
{
    if(V3ChannelDebug::getLevel()>0)printf("V3ChannelGet::~V3ChannelGet\n");
}

bool V3ChannelGet::init(PVStructure::shared_pointer const &pvRequest)
{
    propertyMask = v3Util->getProperties(
        channelGetRequester,
        pvRequest,
        dbAddr,
        false);
    if(propertyMask==v3Util->noAccessBit) return false;
    pvStructure =  PVStructure::shared_pointer(
        v3Util->createPVStructure(
             channelGetRequester, propertyMask, dbAddr));
    if(pvStructure.get()==0) return false;
    v3Util->getPropertyData(channelGetRequester,propertyMask,dbAddr,pvStructure);
    int numFields = pvStructure->getNumberFields();
    bitSet.reset(new BitSet(numFields));
    if((propertyMask&v3Util->processBit)!=0) {
       process = true;
       pNotify.reset(new (struct putNotify)());
       notifyAddr.reset(new DbAddr());
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
       Status::Ok,
       getPtrSelf(),
       pvStructure,
       bitSet);
    return true;
}

String V3ChannelGet::getRequesterName() {
    return channelGetRequester->getRequesterName();
}

void V3ChannelGet::message(String const &message,MessageType messageType)
{
    channelGetRequester->message(message,messageType);
}

void V3ChannelGet::destroy() {
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelGet::destroy beingDestroyed %s\n",
         (beingDestroyed ? "true" : "false"));
    {
        Lock xx(mutex);
        if(beingDestroyed) return;
        beingDestroyed = true;
    }
    v3Channel->removeChannelGet(getPtrSelf());
}

void V3ChannelGet::get(bool lastRequest)
{
    if(process) {
        epicsUInt8 value = 1;
        pNotify.get()->pbuffer = &value;
        dbPutNotify(pNotify.get());
        event.wait();
    }

    Lock lock(dataMutex);

    bitSet->clear();
    dbScanLock(dbAddr.precord);
    Status status = v3Util->get(
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
    
    lock.unlock();
    channelGetRequester->getDone(status);
    if(lastRequest) destroy();
}

void V3ChannelGet::notifyCallback(struct putNotify *pn)
{
    V3ChannelGet * cget = static_cast<V3ChannelGet *>(pn->usrPvt);
    cget->event.signal();
}

void V3ChannelGet::lock()
{
    dataMutex.lock();
}

void V3ChannelGet::unlock()
{
    dataMutex.unlock();
}

}}
