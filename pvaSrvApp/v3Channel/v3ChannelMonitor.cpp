/* vsChannelMonitor.cpp */
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

#include <dbAccess.h>
#include <dbNotify.h>
#include <dbCommon.h>

#include <epicsExport.h>

#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/monitor.h>
#include <pv/pvAccess.h>

#include <pv/v3Channel.h>

#include "CAV3Context.h"
#include <pv/v3CAMonitor.h>
#include "v3Util.h"


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::dynamic_pointer_cast;

V3ChannelMonitor::V3ChannelMonitor(
    ChannelBase::shared_pointer const &v3Channel,
    MonitorRequester::shared_pointer const &monitorRequester,
    DbAddr &dbAddr)
: v3Util(V3Util::getV3Util()),
  v3Channel(v3Channel),
  monitorRequester(monitorRequester),
  dbAddr(dbAddr),
  event(),
  propertyMask(0),
  firstTime(true),
  gotEvent(false),
  v3Type(v3Byte),
  queueSize(2),
  caV3Monitor(),
  numberFree(queueSize),
  numberUsed(0),
  nextGetFree(0),
  nextSetUsed(0),
  nextGetUsed(0),
  nextReleaseUsed(0),
  beingDestroyed(false),
  isStarted(false)
{
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::V3ChannelMonitor\n");
}

V3ChannelMonitor::~V3ChannelMonitor() {
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::~V3ChannelMonitor\n");
}


bool V3ChannelMonitor::init(
    PVStructure::shared_pointer const &pvRequest)
{
    String queueSizeString("record._options.queueSize");
    PVFieldPtr pvField = pvRequest.get()->getSubField(queueSizeString);
    if(pvField!=0) {
        PVStringPtr pvString = pvRequest.get()->getStringField(queueSizeString);
        if(pvString.get()!=NULL) {
             String value = pvString->get();
             queueSize = atoi(value.c_str());
        }
    }
    propertyMask = v3Util->getProperties(
        monitorRequester,
        pvRequest,
        dbAddr,
        false);
    if(propertyMask==v3Util->noAccessBit) return false;
    if(propertyMask&v3Util->isLinkBit) {
        monitorRequester->message("can not monitor a link field",errorMessage);
        return 0;
    }
    elements.reserve(queueSize);
    for(int i=0; i<queueSize; i++) {
        PVStructurePtr pvStructure(v3Util->createPVStructure(
                monitorRequester,
                propertyMask,
                dbAddr));
        MonitorElementPtr element(new MonitorElement(pvStructure));
        elements.push_back(element);
    }
    MonitorElementPtr element = elements[0];
    StructureConstPtr saveStructure = element->pvStructurePtr->getStructure();
    if((propertyMask&v3Util->enumValueBit)!=0) {
        v3Type = v3Enum;
    } else {
        ScalarType scalarType = v3Util->getScalarType(
            monitorRequester,
            dbAddr);
        switch(scalarType) {
        case pvByte: v3Type = v3Byte; break;
        case pvUByte: v3Type = v3UByte; break;
        case pvShort: v3Type = v3Short; break;
        case pvUShort: v3Type = v3UShort; break;
        case pvInt: v3Type = v3Int; break;
        case pvUInt: v3Type = v3UInt; break;
        case pvFloat: v3Type = v3Float; break;
        case pvDouble: v3Type = v3Double; break;
        case pvString: v3Type = v3String; break;
        default:
            throw std::logic_error(String("bad scalarType"));
        }
    }
    String pvName = v3Channel->getChannelName();
    caV3Monitor.reset(
        new CAV3Monitor(getPtrSelf(), pvName, v3Type));
    caV3Monitor->connect();
    event.wait();
    Monitor::shared_pointer thisPointer = dynamic_pointer_cast<Monitor>(getPtrSelf());
    monitorRequester->monitorConnect(
       Status::Ok,
       thisPointer,
       saveStructure);
    return true;
}

String V3ChannelMonitor::getRequesterName() {
    return monitorRequester.get()->getRequesterName();
}

void V3ChannelMonitor::message(String const &message,MessageType messageType)
{
    monitorRequester->message(message,messageType);
}

void V3ChannelMonitor::destroy() {
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::destroy beingDestroyed %s\n",
         (beingDestroyed ? "true" : "false"));
    {
        Lock xx(mutex);
        if(beingDestroyed) return;
        beingDestroyed = true;
    }
    stop();
    caV3Monitor.reset();
    v3Channel->removeChannelMonitor(getPtrSelf());
    v3Channel.reset();
}

Status V3ChannelMonitor::start()
{
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::start\n");
    {
        Lock xx(mutex);
        if(beingDestroyed) {
             Status status(Status::STATUSTYPE_ERROR,"beingDestroyed");
             return status;
        }
        if(isStarted) return Status::Ok;
        isStarted = true;
    }
    currentElement =getFree();
    if(currentElement.get()==0) {
        printf("V3ChannelMonitor::start will throw\n");
        throw std::logic_error(String(
            "V3ChannelMonitor::start no free queue element"));
    }
    BitSet::shared_pointer bitSet = currentElement->changedBitSet;
    bitSet->clear();
    nextElement =getFree();
    if(nextElement.get()!=0) {
        BitSet::shared_pointer bitSet = nextElement->changedBitSet;
        bitSet->clear();
    }
    caV3Monitor.get()->start();
    return Status::Ok;
}

Status V3ChannelMonitor::stop()
{
    {
        Lock xx(mutex);
        if(!isStarted) return Status::Ok;
        isStarted = false;
    }
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::stop\n");
    caV3Monitor.get()->stop();
    return Status::Ok;
}

MonitorElementPtr  V3ChannelMonitor::poll()
{
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::poll\n");
    if(numberUsed==0) return nullElement;
    int ind = nextGetUsed;
    nextGetUsed++;
    if(nextGetUsed>=queueSize) nextGetUsed = 0;
    return elements[ind];
}

void V3ChannelMonitor::release(MonitorElementPtr const & element)
{
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::release\n");
   if(element!=elements[nextReleaseUsed++]) {
        throw std::logic_error(
           "not queueElement returned by last call to getUsed");
    }
    if(nextReleaseUsed>=queueSize) nextReleaseUsed = 0;
    numberUsed--;
    numberFree++;
}

void V3ChannelMonitor::exceptionCallback(long status,long op)
{
}

void V3ChannelMonitor::connectionCallback()
{
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::connectionCallback\n");
    event.signal();
}

void V3ChannelMonitor::accessRightsCallback()
{
}

void V3ChannelMonitor::eventCallback(const char *status)
{
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMonitor::eventCallback\n");
    if(beingDestroyed) return;
    if(status!=0) {
         monitorRequester->message(String(status),errorMessage);
    }
    PVStructure::shared_pointer pvStructure = currentElement->pvStructurePtr;
    BitSet::shared_pointer bitSet = currentElement->changedBitSet;
    dbScanLock(dbAddr.precord);
    CAV3Data &caV3Data = caV3Monitor.get()->getData();
    BitSet::shared_pointer overrunBitSet = currentElement->overrunBitSet;
    Status stat = v3Util->get(
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
        if(nextElement.get()==0) nextElement =getFree();
        if(nextElement.get()!=0) {
            PVStructure::shared_pointer pvNext = nextElement->pvStructurePtr;
            BitSet::shared_pointer bitSetNext = nextElement->changedBitSet;
            Status stat = v3Util->get(
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
    numberUsed++;
    nextSetUsed++;
    if(nextSetUsed>=queueSize) nextSetUsed = 0;
    currentElement = nextElement;
    nextElement =getFree();
    if(nextElement!=0) {
        nextElement->changedBitSet->clear();
        nextElement->overrunBitSet->clear();
    }
    monitorRequester->monitorEvent(getPtrSelf());

}

void V3ChannelMonitor::lock()
{
}

void V3ChannelMonitor::unlock()
{
}

MonitorElementPtr &V3ChannelMonitor::getFree()
{
    if(numberFree==0) return nullElement;
    numberFree--;
    int ind = nextGetFree;
    nextGetFree++;
    if(nextGetFree>=queueSize) nextGetFree = 0;
    return elements[ind];

}

}}

