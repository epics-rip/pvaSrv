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
    PVServiceBase::shared_pointer const &v3Channel,
    ChannelPutRequester::shared_pointer const &channelPutRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),
  channelPutRequester(channelPutRequester),
  dbAddr(dbAddr),
  propertyMask(0),
  process(false),
  firstTime(true),
  pvStructure(),
  bitSet(),
  pNotify(0),
  notifyAddr(0),
  event()
{
printf("V3ChannelPut::V3ChannelPut()\n");
}

V3ChannelPut::~V3ChannelPut()
{
printf("V3ChannelPut::~V3ChannelPut()\n");
}


bool V3ChannelPut::init(PVStructure::shared_pointer const &pvRequest)
{
    propertyMask = V3Util::getProperties(
        channelPutRequester,
        pvRequest,
        dbAddr);
    if(propertyMask==V3Util::noAccessBit) return false;
    if(propertyMask==V3Util::noModBit) {
        channelPutRequester->message(
             String("field not allowed to be changed"),errorMessage);
        return 0;
    }
    pvStructure.reset(
        V3Util::createPVStructure(
            channelPutRequester,
            propertyMask,
            dbAddr));
    if(pvStructure.get()==0) return 0;
    if((propertyMask&V3Util::dbPutBit)!=0) {
        if((propertyMask&V3Util::processBit)!=0) {
            channelPutRequester->message(
             String("process determined by dbPutField"),errorMessage);
        }
    } else if((propertyMask&V3Util::processBit)!=0) {
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
    int numFields = pvStructure->getNumberFields();
    bitSet.reset(new BitSet(numFields));
    channelPutRequester->channelPutConnect(
       Status::OK,
       getPtrSelf(),
       pvStructure,
       bitSet);
    return true;
}

String V3ChannelPut::getRequesterName() {
    return channelPutRequester->getRequesterName();
}

void V3ChannelPut::message(String message,MessageType messageType)
{
    channelPutRequester->message(message,messageType);
}

void V3ChannelPut::destroy() {
    v3Channel->removeChannelPut(*this);
}

void V3ChannelPut::put(bool lastRequest)
{
    PVField *pvField = pvStructure.get()->getPVFields()[0];
    if(propertyMask&V3Util::dbPutBit) {
        Status status = V3Util::putField(
            channelPutRequester,propertyMask,dbAddr,pvField);
        channelPutRequester->putDone(status);
        if(lastRequest) destroy();
        return;
    }
    dbScanLock(dbAddr.precord);
    Status status = V3Util::put(
        channelPutRequester,propertyMask,dbAddr,pvField);
    dbScanUnlock(dbAddr.precord);
    if(process) {
        epicsUInt8 value = 1;
        pNotify.get()->pbuffer = &value;
        dbPutNotify(pNotify.get());
        event.wait();
    }
    channelPutRequester->putDone(status);
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
        channelPutRequester,
        propertyMask,dbAddr,
        pvStructure,
        bitSet,
        0);
    if(firstTime) {
        firstTime = false;
        bitSet->set(pvStructure->getFieldOffset());
    }
    dbScanUnlock(dbAddr.precord);
    channelPutRequester->getDone(Status::OK);
}

}}

