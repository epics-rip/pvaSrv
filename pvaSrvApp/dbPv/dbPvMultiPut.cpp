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
#include <dbEvent.h>
#include <dbNotify.h>
#include <dbCommon.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/standardPVField.h>

#include "dbPv.h"
#include "dbUtil.h"

namespace epics { namespace pvaSrv { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;

V3ChannelMultiPut::V3ChannelMultiPut(
    ChannelBase::shared_pointer const &v3Channel,
    ChannelPutRequester::shared_pointer const &channelPutRequester,
    DbAddr &dbAddr)
: v3Util(V3Util::getV3Util()),
  v3Channel(v3Channel),
  channelPutRequester(channelPutRequester),
  dbAddr(dbAddr),
  propertyMask(0),
  process(false),
  firstTime(true),
  beingDestroyed(false)
{
    if(V3ChannelDebug::getLevel()>0)
        printf("V3ChannelMultiPut::V3ChannelMultiPut()\n");
}

V3ChannelMultiPut::~V3ChannelMultiPut()
{
    if(V3ChannelDebug::getLevel()>0)
        printf("V3ChannelMultiPut::~V3ChannelMultiPut()\n");
}

bool V3ChannelMultiPut::init(PVStructurePtr const &pvRequest)
{
    if(V3ChannelDebug::getLevel()>0)
        printf("V3ChannelMultiPut::init()\n");
    propertyMask = v3Util->getProperties(
        channelPutRequester,
        pvRequest,
        dbAddr,
        true);
    if(propertyMask==v3Util->noAccessBit) return false;
    if(propertyMask==v3Util->noModBit) {
        channelPutRequester->message(
             String("field not allowed to be changed"),errorMessage);
        return 0;
    }
    String channelName(v3Channel->getChannelName());
    if(channelName.find('.') != String::npos) {
        String message(channelName);
        message += " field of a record not allowed";
        channelPutRequester->message(message,errorMessage);
        return 0;
    }
    if((propertyMask&v3Util->processBit)!=0) process = true;
    PVStructurePtr pvPutField = pvRequest->getStructureField("putField");
    StringArray const & extraNames = pvPutField->getStructure()->getFieldNames();
    size_t n = extraNames.size() + 1;
    dbAddrArray.resize(n);
    dbAddrArray[0] = dbAddr;
    short fieldType = dbAddr.field_type;
    for(size_t i = 1; i<n; i++) {
        channelName = extraNames[i-1];
        if(channelName.find('.') != String::npos) {
            String message(channelName);
            message += " field of a record not allowed";
            channelPutRequester->message(message,errorMessage);
            return 0;
        }
        if(dbNameToAddr(extraNames[i-1].c_str(),&dbAddrArray[i])!=0) {
            if(V3ChannelDebug::getLevel()>0) {
                printf("dbNameToAddr failed for %s\n",extraNames[i-1].c_str());
            }
            String message("record not found ");
            message += extraNames[i-1];
            channelPutRequester->message(message,errorMessage);
            return 0;
        }
        if(dbAddrArray[i].field_type!=fieldType) {
            if(V3ChannelDebug::getLevel()>0) {
                printf("scalarType not the same failed %s\n",extraNames[i-1].c_str());
            }
            String message("scalarType not the same for ");
            message += extraNames[i-1];
            channelPutRequester->message(message,errorMessage);
            return 0;
        }
    }
    ScalarType scalarType(pvBoolean);
    switch(dbAddr.field_type) {
        case DBF_CHAR:
            scalarType = pvByte; break;
        case DBF_UCHAR:
            scalarType = pvUByte; break;
        case DBF_SHORT:
            scalarType = pvShort; break;
        case DBF_USHORT:
            scalarType = pvUShort; break;
        case DBF_LONG:
            scalarType = pvInt; break;
        case DBF_ULONG:
            scalarType = pvUInt; break;
        case DBF_FLOAT:
            scalarType = pvFloat; break;
        case DBF_DOUBLE:
            scalarType = pvDouble; break;
        default:
            channelPutRequester->message(
                String("record is not a scalar"),errorMessage);
            return 0;
    }
    String prop;
    pvStructure = getStandardPVField()->scalarArray(scalarType,prop);
    pvScalarArray = pvStructure->getScalarArrayField("value",scalarType);
    pvScalarArray->setCapacity(n);
    pvScalarArray->setCapacityMutable(false);
    pvScalarArray->setLength(n);
    int numFields = pvStructure->getNumberFields();
    bitSet.reset(new BitSet(numFields));
    channelPutRequester->channelPutConnect(
       Status::Ok,
       getPtrSelf(),
       pvStructure,
       bitSet);
    if(V3ChannelDebug::getLevel()>0)
        printf("V3ChannelMultiPut::init() returning true\n");
    return true;
}

String V3ChannelMultiPut::getRequesterName() {
    return channelPutRequester->getRequesterName();
}

void V3ChannelMultiPut::message(String const &message,MessageType messageType)
{
    channelPutRequester->message(message,messageType);
}

void V3ChannelMultiPut::destroy() {
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelMultiPut::destroy beingDestroyed %s\n",
         (beingDestroyed ? "true" : "false"));
    {
        Lock xx(mutex);
        if(beingDestroyed) return;
        beingDestroyed = true;
    }
    v3Channel->removeChannelPut(getPtrSelf());
}

void V3ChannelMultiPut::put(bool lastRequest)
{
    if(V3ChannelDebug::getLevel()>0)
        printf("V3ChannelMultiPut::put()\n");
    Lock lock(dataMutex);
    bool isSameLockSet = true;
    size_t n = dbAddrArray.size();
    struct dbCommon *precord = dbAddr.precord;
    dbScanLock(precord);
    unsigned long lockId = dbLockGetLockId(precord);
    for(size_t i=1; i<n; i++) {
        precord = dbAddrArray[i].precord;
        unsigned long id = dbLockGetLockId(precord);
        if(id!=lockId) {
            isSameLockSet = false;
        }
    }
    if(!isSameLockSet) dbScanUnlock(dbAddr.precord);
    for(size_t i=0; i<n; i++) {
        precord = dbAddrArray[i].precord;
        if(!isSameLockSet) dbScanLock(precord);
        ScalarType scalarType = pvScalarArray->getScalarArray()->getElementType();
        switch(scalarType) {
        case pvByte: {
            int8 * val = static_cast<int8 *>(dbAddrArray[i].pfield);
            PVByteArrayPtr pv = static_pointer_cast<PVByteArray>(pvScalarArray);
            int8 * array = pv->get();
            *val = array[i];
            break;
        }
        case pvUByte: {
            uint8 * val = static_cast<uint8 *>(dbAddrArray[i].pfield);
            PVUByteArrayPtr pv = static_pointer_cast<PVUByteArray>(pvScalarArray);
            uint8 * array = pv->get();
            *val = array[i];
            break;
        }
        case pvShort: {
            int16 * val = static_cast<int16 *>(dbAddrArray[i].pfield);
            PVShortArrayPtr pv = static_pointer_cast<PVShortArray>(pvScalarArray);
            int16 * array = pv->get();
            *val = array[i];
            break;
        }
        case pvUShort: {
            uint16 * val = static_cast<uint16 *>(dbAddrArray[i].pfield);
            PVUShortArrayPtr pv = static_pointer_cast<PVUShortArray>(pvScalarArray);
            uint16 * array = pv->get();
            *val = array[i];
            break;
        }
        case pvInt: {
            int32 * val = static_cast<int32 *>(dbAddrArray[i].pfield);
            PVIntArrayPtr pv = static_pointer_cast<PVIntArray>(pvScalarArray);
            int32 * array = pv->get();
            *val = array[i];
            break;
        }
        case pvUInt: {
            uint32 * val = static_cast<uint32 *>(dbAddrArray[i].pfield);
            PVUIntArrayPtr pv = static_pointer_cast<PVUIntArray>(pvScalarArray);
            uint32 * array = pv->get();
            *val = array[i];
            break;
        }
        case pvFloat: {
            float * val = static_cast<float *>(dbAddrArray[i].pfield);
            PVFloatArrayPtr pv = static_pointer_cast<PVFloatArray>(pvScalarArray);
            float * array = pv->get();
            *val = array[i];
            break;
        }
        case pvDouble: {
            double * val = static_cast<double *>(dbAddrArray[i].pfield);
            PVDoubleArrayPtr pv = static_pointer_cast<PVDoubleArray>(pvScalarArray);
            double * array = pv->get();
            *val = array[i];
            break;
        }
        default:
            channelPutRequester->message(
                String("Logic Error did not handle scalarType"),errorMessage);
        }
        if(process) {
            if(!precord->disp) dbProcess(precord);
        }
        if(!isSameLockSet) dbScanUnlock(precord);
    }
    if(isSameLockSet) dbScanUnlock(dbAddr.precord);
    String message("atomic ");
    message += (isSameLockSet ? "true" : "false");
    channelPutRequester->message(message,infoMessage);
    lock.unlock();
    channelPutRequester->putDone(Status::Ok);
    if(lastRequest) destroy();
}

void V3ChannelMultiPut::get()
{
    if(V3ChannelDebug::getLevel()>0)
        printf("V3ChannelMultiPut::get()\n");
    Lock lock(dataMutex);
    bool isSameLockSet = true;
    size_t n = dbAddrArray.size();
    struct dbCommon *precord = dbAddr.precord;
    dbScanLock(precord);
    unsigned long lockId = dbLockGetLockId(precord);
    for(size_t i=1; i<n; i++) {
        precord = dbAddrArray[i].precord;
        unsigned long id = dbLockGetLockId(precord);
        if(id!=lockId) {
            isSameLockSet = false;
        }
    }
    if(!isSameLockSet) dbScanUnlock(dbAddr.precord);
    for(size_t i=0; i<n; i++) {
        precord = dbAddrArray[i].precord;
        if(!isSameLockSet) dbScanLock(precord);
        ScalarType scalarType = pvScalarArray->getScalarArray()->getElementType();
        switch(scalarType) {
        case pvByte: {
            int8 * val = static_cast<int8 *>(dbAddrArray[i].pfield);
            PVByteArrayPtr pv = static_pointer_cast<PVByteArray>(pvScalarArray);
            int8 * array = pv->get();
            array[i] = *val;
            break;
        }
        case pvUByte: {
            uint8 * val = static_cast<uint8 *>(dbAddrArray[i].pfield);
            PVUByteArrayPtr pv = static_pointer_cast<PVUByteArray>(pvScalarArray);
            uint8 * array = pv->get();
            array[i] = *val;
            break;
        }
        case pvShort: {
            int16 * val = static_cast<int16 *>(dbAddrArray[i].pfield);
            PVShortArrayPtr pv = static_pointer_cast<PVShortArray>(pvScalarArray);
            int16 * array = pv->get();
            array[i] = *val;
            break;
        }
        case pvUShort: {
            uint16 * val = static_cast<uint16 *>(dbAddrArray[i].pfield);
            PVUShortArrayPtr pv = static_pointer_cast<PVUShortArray>(pvScalarArray);
            uint16 * array = pv->get();
            array[i] = *val;
            break;
        }
        case pvInt: {
            int32 * val = static_cast<int32 *>(dbAddrArray[i].pfield);
            PVIntArrayPtr pv = static_pointer_cast<PVIntArray>(pvScalarArray);
            int32 * array = pv->get();
            array[i] = *val;
            break;
        }
        case pvUInt: {
            uint32 * val = static_cast<uint32 *>(dbAddrArray[i].pfield);
            PVUIntArrayPtr pv = static_pointer_cast<PVUIntArray>(pvScalarArray);
            uint32 * array = pv->get();
            array[i] = *val;
            break;
        }
        case pvFloat: {
            float * val = static_cast<float *>(dbAddrArray[i].pfield);
            PVFloatArrayPtr pv = static_pointer_cast<PVFloatArray>(pvScalarArray);
            float * array = pv->get();
            array[i] = *val;
            break;
        }
        case pvDouble: {
            double * val = static_cast<double *>(dbAddrArray[i].pfield);
            PVDoubleArrayPtr pv = static_pointer_cast<PVDoubleArray>(pvScalarArray);
            double * array = pv->get();
            array[i] = *val;
            break;
        }
        default:
            channelPutRequester->message(
                String("Logic Error did not handle scalarType"),errorMessage);
        }
        if(!isSameLockSet) dbScanUnlock(precord);
    }
    bitSet->clear();
    bitSet->set(0);
    if(isSameLockSet) dbScanUnlock(dbAddr.precord);
    lock.unlock();
    channelPutRequester->getDone(Status::Ok);
}

void V3ChannelMultiPut::lock()
{
    dataMutex.lock();
}

void V3ChannelMultiPut::unlock()
{
    dataMutex.unlock();
}

}}
