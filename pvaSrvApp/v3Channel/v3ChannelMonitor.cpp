/* vsChannelMonitor.cpp */
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
#include <dbNotify.h>
#include <dbCommon.h>

#include <epicsExport.h>

#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/monitor.h>
#include <pv/monitorQueue.h>
#include <pv/pvAccess.h>

#include <pv/pvDatabase.h>
#include <pv/v3Channel.h>
#include <pv/support.h>

#include "CAV3Context.h"
#include <pv/v3CAMonitor.h>
#include "v3Util.h"


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelMonitor::V3ChannelMonitor(
    PVServiceBase::shared_pointer const &v3Channel,
    MonitorRequester::shared_pointer const &monitorRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),
  monitorRequester(monitorRequester),
  dbAddr(dbAddr),
  event(),
  propertyMask(0),
  firstTime(true),
  gotEvent(false),
  v3Type(v3Byte),
  queueSize(2),
  monitorQueue(std::auto_ptr<MonitorQueue>()),
  caV3Monitor(std::auto_ptr<CAV3Monitor>()),
  currentElement(),
  nextElement()
{
printf("V3ChannelMonitor construct\n");
}

V3ChannelMonitor::~V3ChannelMonitor() {
printf("~V3ChannelMonitor\n");
}


bool V3ChannelMonitor::init(
    PVStructure::shared_pointer const &pvRequest)
{
    String queueSizeString(V3Util::queueSizeString);
    PVField *pvField = pvRequest.get()->getSubField(queueSizeString);
    if(pvField!=0) {
        PVString *pvString = pvRequest.get()->getStringField(queueSizeString);
        if(pvString!=0) {
             String value = pvString->get();
             queueSize = atoi(value.c_str());
        }
    }
    propertyMask = V3Util::getProperties(
        monitorRequester,
        pvRequest,
        dbAddr);
    if(propertyMask==V3Util::noAccessBit) return false;
    if(propertyMask&V3Util::isLinkBit) {
        monitorRequester->message(
             String("can not monitor a link field"),errorMessage);
        return 0;
    }
    PVStructurePtrArray pvStructurePtrArray = new PVStructurePtr[queueSize];
    for(int i=0; i<queueSize; i++) {
        pvStructurePtrArray[i] = V3Util::createPVStructure(
                monitorRequester,
                propertyMask,
                dbAddr);
    }
    StructureConstPtr saveStructure = pvStructurePtrArray[0]->getStructure();
    PVStructureSharedPointerPtrArray array = 
        MonitorQueue::createStructures( pvStructurePtrArray,queueSize);
    for(int i=0; i<queueSize; i++) {
        V3Util::getPropertyData(
            monitorRequester,
            propertyMask,
            dbAddr,
            *array[i]);
    }
    if((propertyMask&V3Util::enumValueBit)!=0) {
        v3Type = v3Enum;
    } else {
        ScalarType scalarType = V3Util::getScalarType(
            monitorRequester,
            dbAddr);
        switch(scalarType) {
        case pvByte: v3Type = v3Byte; break;
        case pvShort: v3Type = v3Short; break;
        case pvInt: v3Type = v3Int; break;
        case pvFloat: v3Type = v3Float; break;
        case pvDouble: v3Type = v3Double; break;
        case pvString: v3Type = v3String; break;
        default:
            throw std::logic_error(String("bad scalarType"));
        }
    }
    monitorQueue = std::auto_ptr<MonitorQueue>(
        new MonitorQueue(array,queueSize));
    String pvName = v3Channel->getChannelName();
    caV3Monitor = std::auto_ptr<CAV3Monitor>(
        new CAV3Monitor( *this, pvName, v3Type));
    caV3Monitor.get()->connect();
    event.wait();
    monitorRequester->monitorConnect(
       Status::OK,
       getPtrSelf(),
       saveStructure);
    return true;
}

String V3ChannelMonitor::getRequesterName() {
    return monitorRequester.get()->getRequesterName();
}

void V3ChannelMonitor::message(String message,MessageType messageType)
{
    monitorRequester->message(message,messageType);
}

void V3ChannelMonitor::destroy() {
printf("V3ChannelMonitor::destroy\n");
    v3Channel->removeChannelMonitor(*this);
}

Status V3ChannelMonitor::start()
{
    currentElement = monitorQueue->getFree();
    if(currentElement.get()==0) {
        printf("V3ChannelMonitor::start will throw\n");
        throw std::logic_error(String(
            "V3ChannelMonitor::start no free queue element"));
    }
    BitSet::shared_pointer bitSet = currentElement->getChangedBitSet();
    bitSet->clear();
    nextElement = monitorQueue->getFree();
    if(nextElement.get()!=0) {
        BitSet::shared_pointer bitSet = nextElement->getChangedBitSet();
        bitSet->clear();
    }
    caV3Monitor.get()->start();
    return Status::OK;
}

Status V3ChannelMonitor::stop()
{
    caV3Monitor.get()->stop();
    return Status::OK;
}

MonitorElement::shared_pointer const & V3ChannelMonitor::poll()
{
    MonitorElement::shared_pointer element = monitorQueue->getUsed();
    MonitorElement::shared_pointer const & xxx = element;
    return xxx;
}

void V3ChannelMonitor::release(MonitorElement::shared_pointer const & monitorElement)
{
    monitorQueue->releaseUsed(monitorElement);
}

void V3ChannelMonitor::exceptionCallback(long status,long op)
{
}

void V3ChannelMonitor::connectionCallback()
{
    event.signal();
}

void V3ChannelMonitor::accessRightsCallback()
{
}

void V3ChannelMonitor::eventCallback(const char *status)
{
    if(status!=0) {
         monitorRequester->message(String(status),errorMessage);
    }
    PVStructure::shared_pointer pvStructure = currentElement->getPVStructure();
    BitSet::shared_pointer bitSet = currentElement->getChangedBitSet();
    dbScanLock(dbAddr.precord);
    CAV3Data &caV3Data = caV3Monitor.get()->getData();
    BitSet::shared_pointer overrunBitSet = currentElement->getOverrunBitSet();
    Status stat = V3Util::get(
       monitorRequester,
       propertyMask,
       dbAddr,
       pvStructure,
       overrunBitSet,
       &caV3Data);
    int index = overrunBitSet->nextSetBit(0);
    while(index>=0) {
        bool wasSet = bitSet->get(index);
        if(!wasSet) {
            bitSet->set(index);
            overrunBitSet->clear(index);
        }
        index = overrunBitSet->nextSetBit(index+1);
    }
    if(bitSet->nextSetBit(0)>=0) {
        if(nextElement.get()==0) nextElement = monitorQueue->getFree();
        if(nextElement.get()!=0) {
            PVStructure::shared_pointer pvNext = nextElement->getPVStructure();
            BitSet::shared_pointer bitSetNext = nextElement->getChangedBitSet();
            Status stat = V3Util::get(
                monitorRequester,
                propertyMask,
                dbAddr,
                pvNext,
                bitSetNext,
                &caV3Data);
            bitSetNext->clear();
            *bitSetNext |= *bitSet;
        }
    }
    dbScanUnlock(dbAddr.precord);
    if(firstTime) {
        firstTime = false;
        bitSet->clear();
        bitSet->set(0);
    }
    if(bitSet->nextSetBit(0)<0) return;
    if(nextElement.get()==0) return;
    monitorQueue->setUsed(currentElement);
    currentElement = nextElement;
    nextElement = monitorQueue->getFree();
    if(nextElement!=0) {
        nextElement->getChangedBitSet()->clear();
        nextElement->getOverrunBitSet()->clear();
    }
    monitorRequester->monitorEvent(getPtrSelf());

}

}}

