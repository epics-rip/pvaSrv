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

#include <pvData.h>
#include <convert.h>
#include <monitor.h>
#include <monitorQueue.h>
#include <pvAccess.h>

#include "pvDatabase.h"
#include "v3Channel.h"
#include "support.h"

#include "CAV3Context.h"
#include "v3CAMonitor.h"
#include "v3Util.h"


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelMonitor::V3ChannelMonitor(
    V3Channel &v3Channel,
    MonitorRequester &monitorRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),monitorRequester(monitorRequester),
  dbAddr(dbAddr),
  monitorListNode(*this),
  event(),
  propertyMask(0),
  firstTime(true),
  gotEvent(false),
  overrun(false),
  v3Type(v3Byte),
  queueSize(2),
  pvStructurePtrArray(0),
  monitorQueue(std::auto_ptr<MonitorQueue>(0)),
  caV3Monitor(std::auto_ptr<CAV3Monitor>(0))
{
}

V3ChannelMonitor::~V3ChannelMonitor() {
    if(pvStructurePtrArray!=0) {
        for(int i=0; i<queueSize; i++) {
            delete pvStructurePtrArray[i];
        }
        delete[]pvStructurePtrArray;
        pvStructurePtrArray = 0;
    }
}


ChannelMonitorListNode * V3ChannelMonitor::init(
    PVStructure &pvRequest)
{
    String queueSizeString(V3Util::queueSizeString);
    PVField *pvField = pvRequest.getSubField(queueSizeString);
    if(pvField!=0) {
        PVString *pvString = pvRequest.getStringField(queueSizeString);
        if(pvString!=0) {
             String value = pvString->get();
             queueSize = atoi(value.c_str());
        }
    }
    pvStructurePtrArray = new PVStructurePtr[queueSize];
    propertyMask = V3Util::getProperties(monitorRequester,pvRequest,dbAddr);
    if(propertyMask==V3Util::noAccessBit) return 0;
    if(propertyMask&V3Util::isLinkBit) {
        monitorRequester.message(
             String("can not monitor a link field"),errorMessage);
        return 0;
    }
    pvStructurePtrArray[0] = V3Util::createPVStructure(
        monitorRequester,propertyMask,dbAddr);
    PVDataCreate * pvDataCreate = getPVDataCreate();
    for(int i=1; i<queueSize; i++) {
        pvStructurePtrArray[i] = pvDataCreate->createPVStructure(
            0,String(""),pvStructurePtrArray[0]);
    }
    V3Util::getPropertyData(
        monitorRequester,propertyMask,dbAddr,*pvStructurePtrArray[0]);
    if((propertyMask&V3Util::enumValueBit)!=0) {
        v3Type = v3Enum;
    } else {
        ScalarType scalarType = V3Util::getScalarType(monitorRequester,dbAddr);
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
    MonitorElement** array = MonitorQueue::createElements(
        pvStructurePtrArray,queueSize);
    monitorQueue = std::auto_ptr<MonitorQueue>(
        new MonitorQueue(array,queueSize));
    String pvName = v3Channel.getChannelName();
    for(int i=1; i<queueSize; i++) {
        getConvert()->copyStructure(pvStructurePtrArray[0],pvStructurePtrArray[i]);
    }
    caV3Monitor = std::auto_ptr<CAV3Monitor>(
        new CAV3Monitor( *this, pvName, v3Type));
    caV3Monitor.get()->connect();
    event.wait();
    monitorRequester.monitorConnect(
       Status::OK,
       this,
       pvStructurePtrArray[0]->getStructure());
    return &monitorListNode;
}

String V3ChannelMonitor::getRequesterName() {
    return monitorRequester.getRequesterName();
}

void V3ChannelMonitor::message(String message,MessageType messageType)
{
    monitorRequester.message(message,messageType);
}

void V3ChannelMonitor::destroy() {
    v3Channel.removeChannelMonitor(monitorListNode);
    delete this;
}

Status V3ChannelMonitor::start()
{
    caV3Monitor.get()->start();
    return Status::OK;
}

Status V3ChannelMonitor::stop()
{
    caV3Monitor.get()->stop();
    return Status::OK;
}

MonitorElement* V3ChannelMonitor::poll()
{
    return monitorQueue->getUsed();
}

void V3ChannelMonitor::release(MonitorElement* monitorElement)
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

void V3ChannelMonitor::eventCallback(long status)
{
    if(status!=1) {
         // what to do?
    }
    MonitorElement *element = monitorQueue->getFree();
    if(element==0) {
        overrun = true;
        return;
    }
    PVStructure *pvStructure = element->getPVStructure();
    BitSet *bitSet = element->getChangedBitSet();
    dbScanLock(dbAddr.precord);
    CAV3Data &caV3Data = caV3Monitor.get()->getData();
    Status stat = V3Util::get(
       monitorRequester,propertyMask,dbAddr,
       *pvStructure,*bitSet,
       &caV3Data);
    dbScanUnlock(dbAddr.precord);
    if(firstTime) {
        firstTime = false;
        bitSet->clear();
        bitSet->set(0);
    }
    monitorQueue->setUsed(element);
    monitorRequester.monitorEvent(this);
}

}}

