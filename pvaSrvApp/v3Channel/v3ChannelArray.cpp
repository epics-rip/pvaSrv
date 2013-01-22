/* v3ChannelArray.cpp */
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

#include <cantProceed.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <dbAccess.h>
#include <dbCommon.h>
#include <recSup.h>
#include <dbBase.h>
#include <dbEvent.h>

#include <epicsExport.h>

#include <pv/noDefaultMethods.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/alarm.h>
#include <pv/control.h>
#include <pv/display.h>
#include <pv/timeStamp.h>
#include <pv/pvAlarm.h>
#include <pv/pvControl.h>
#include <pv/pvDisplay.h>
#include <pv/pvEnumerated.h>
#include <pv/pvTimeStamp.h>

#include <v3Channel.h>


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;

extern "C" {
typedef long (*get_array_info) (DBADDR *,long *,long *);
typedef long (*put_array_info) (DBADDR *,long );
}

V3ChannelArray::V3ChannelArray(
    ChannelBase::shared_pointer const &v3Channel,
    ChannelArrayRequester::shared_pointer const &channelArrayRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),
  channelArrayRequester(channelArrayRequester),
  dbAddr(dbAddr),
  beingDestroyed(false)
{
    if(V3ChannelDebug::getLevel()>0)printf("V3ChannelArray::V3ChannelArray\n");
}

V3ChannelArray::~V3ChannelArray()
{
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelArray::~V3ChannelArray\n");
}


bool V3ChannelArray::init(PVStructure::shared_pointer const &pvRequest)
{
    if(!dbAddr.no_elements>1) {
        channelArrayRequester.get()->message("field in V3 record is not an array",errorMessage);
        return false;
    }
    ScalarType scalarType(pvBoolean);
    // Note that pvBoolean is not a supported type
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
    case DBF_STRING:
        scalarType = pvString; break;
    default:
      break;
    }
    if(scalarType==pvBoolean) {
        channelArrayRequester.get()->message("unsupported field in V3 record",errorMessage);
        return false;
    }
    pvScalarArray = getPVDataCreate()->createPVScalarArray(scalarType);
    channelArrayRequester->channelArrayConnect(
        Status::Ok,
        getPtrSelf(),
        pvScalarArray);
    return true;
}

void V3ChannelArray::destroy() {
    if(V3ChannelDebug::getLevel()>0) printf("V3ChannelArray::destroy beingDestroyed %s\n",
         (beingDestroyed ? "true" : "false"));
     {
        Lock xx(mutex);
        if(beingDestroyed) return;
        beingDestroyed = true;
    }
    v3Channel->removeChannelArray(getPtrSelf());
}

void V3ChannelArray::getArray(bool lastRequest,int offset,int count)
{
    dbScanLock(dbAddr.precord);
    long length = 0;
    long v3offset = 0;
    struct rset *prset = dbGetRset(&dbAddr);
    if(prset && prset->get_array_info) {
        get_array_info get_info;
        get_info = (get_array_info)(prset->get_array_info);
        get_info(&dbAddr, &length, &v3offset);
        if(v3offset!=0) {
            dbScanUnlock(dbAddr.precord);
            channelArrayRequester->getArrayDone(
                Status(Status::STATUSTYPE_ERROR,
                   String("v3offset not supported"),
                   String("")));
            if(lastRequest) destroy();
            return;
        }
    }
    if(count<=0) count = length - offset;
    if((offset+count)>length) count = length -offset;
    if(count<0) {
        dbScanUnlock(dbAddr.precord);
        channelArrayRequester->getArrayDone(Status::Ok);
        if(lastRequest) destroy();
        return;
    }
    {
    Lock lock(dataMutex);
    pvScalarArray->setLength(count);
    switch(dbAddr.field_type) {
    case DBF_CHAR: {
        PVByteArrayPtr pv = static_pointer_cast<PVByteArray>(pvScalarArray);
        int8 *from = static_cast<int8 *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
   }
    case DBF_UCHAR: {
        PVUByteArrayPtr pv = static_pointer_cast<PVUByteArray>(pvScalarArray);
        uint8 *from = static_cast<uint8 *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_SHORT: {
        PVShortArrayPtr pv = static_pointer_cast<PVShortArray>(pvScalarArray);
        int16 *from = static_cast<int16 *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_USHORT: {
        PVUShortArrayPtr pv = static_pointer_cast<PVUShortArray>(pvScalarArray);
        uint16 *from = static_cast<uint16 *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_LONG: {
        PVIntArrayPtr pv = static_pointer_cast<PVIntArray>(pvScalarArray);
        int32 *from = static_cast<int32 *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_ULONG: {
        PVUIntArrayPtr pv = static_pointer_cast<PVUIntArray>(pvScalarArray);
        uint32 *from = static_cast<uint32 *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_FLOAT: {
        PVFloatArrayPtr pv = static_pointer_cast<PVFloatArray>(pvScalarArray);
        float *from = static_cast<float *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_DOUBLE: {
        PVDoubleArrayPtr pv = static_pointer_cast<PVDoubleArray>(pvScalarArray);
        double *from = static_cast<double *>(dbAddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_STRING: {
        PVStringArrayPtr pv = static_pointer_cast<PVStringArray>(pvScalarArray);
        StringArrayData data;
        pv->get(0,length,data);
        int index = 0;
        char *pchar = static_cast<char *>(dbAddr.pfield);
        pchar += dbAddr.field_size*offset;
        while(index<count) {
            data.data[index] = String(pchar);
            index++;
            pchar += dbAddr.field_size;
        }
        break;
    }
    }
    }
    dbScanUnlock(dbAddr.precord);
    channelArrayRequester->getArrayDone(Status::Ok);
    if(lastRequest) destroy();
}

void V3ChannelArray::putArray(bool lastRequest,int offset,int count)
{
    dbScanLock(dbAddr.precord);
    long no_elements = dbAddr.no_elements;
    if((offset+count)>no_elements) count = no_elements - count;
    if(count<=0) {
        dbScanUnlock(dbAddr.precord);
        channelArrayRequester->getArrayDone(Status::Ok);
        if(lastRequest) destroy();
        return;
    }
    long length = offset + count;
    struct rset *prset = dbGetRset(&dbAddr);
    if(prset && prset->get_array_info) {
        long oldLength = 0;
        long v3offset = 0;
        get_array_info get_info;
        get_info = (get_array_info)(prset->get_array_info);
        get_info(&dbAddr, &oldLength, &v3offset);
        if(length>oldLength) {
           if(prset && prset->put_array_info) {
               put_array_info put_info;
               put_info = (put_array_info)(prset->put_array_info);
               put_info(&dbAddr, length);
           }
        }
    }
    {
    Lock lock(dataMutex);
    switch(dbAddr.field_type) {
    case DBF_CHAR: {
        PVByteArrayPtr pv = static_pointer_cast<PVByteArray>(pvScalarArray);
        ByteArrayData data;
        pv->get(0,count,data);
        int8 *to = static_cast<int8 *>(dbAddr.pfield);
        ByteArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_UCHAR: {
        PVUByteArrayPtr pv = static_pointer_cast<PVUByteArray>(pvScalarArray);
        UByteArrayData data;
        pv->get(0,count,data);
        uint8 *to = static_cast<uint8 *>(dbAddr.pfield);
        UByteArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_SHORT: {
        PVShortArrayPtr pv = static_pointer_cast<PVShortArray>(pvScalarArray);
        ShortArrayData data;
        pv->get(0,count,data);
        int16 *to = static_cast<int16 *>(dbAddr.pfield);
        ShortArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_USHORT: {
        PVUShortArrayPtr pv = static_pointer_cast<PVUShortArray>(pvScalarArray);
        UShortArrayData data;
        pv->get(0,count,data);
        uint16 *to = static_cast<uint16 *>(dbAddr.pfield);
        UShortArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_LONG: {
        PVIntArrayPtr pv = static_pointer_cast<PVIntArray>(pvScalarArray);
        IntArrayData data;
        pv->get(0,count,data);
        int32 *to = static_cast<int32 *>(dbAddr.pfield);
        IntArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_ULONG: {
        PVUIntArrayPtr pv = static_pointer_cast<PVUIntArray>(pvScalarArray);
        UIntArrayData data;
        pv->get(0,count,data);
        uint32 *to = static_cast<uint32 *>(dbAddr.pfield);
        UIntArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_FLOAT: {
        PVFloatArrayPtr pv = static_pointer_cast<PVFloatArray>(pvScalarArray);
        FloatArrayData data;
        pv->get(0,count,data);
        float *to = static_cast<float *>(dbAddr.pfield);
        FloatArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_DOUBLE: {
        PVDoubleArrayPtr pv = static_pointer_cast<PVDoubleArray>(pvScalarArray);
        DoubleArrayData data;
        pv->get(0,count,data);
        double *to = static_cast<double *>(dbAddr.pfield);
        DoubleArray_iterator iter = data.data.begin();
        int ind = 0;
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            to[offset+ind] = *iter;
            ind++;
        }
        break;
    }
    case DBF_STRING: {
        PVStringArrayPtr pv = static_pointer_cast<PVStringArray>(pvScalarArray);
        StringArrayData data;
        pv->get(0,length,data);
        int index = 0;
        char *to = static_cast<char *>(dbAddr.pfield);
        int len = dbAddr.field_size;
        StringArray_iterator iter = data.data.begin();
        for(iter=data.data.begin();iter!=data.data.end();++iter) {
            String val = *iter;
            const char * from = val.c_str();
            if(from!=NULL) {
                strncpy(to,from,len-1);
            }
            *(to + len -1) = 0;
            to += len;
            index++;
        }
        break;
    }
    }
    }
    db_post_events(dbAddr.precord,dbAddr.pfield,DBE_VALUE | DBE_LOG);
    dbScanUnlock(dbAddr.precord);
    channelArrayRequester->getArrayDone(Status::Ok);
    if(lastRequest) destroy();
}

void V3ChannelArray::setLength(bool lastRequest,int length,int capacity)
{
    dbScanLock(dbAddr.precord);
    long no_elements = dbAddr.no_elements;
    if(length>no_elements) length = no_elements;
    struct rset *prset = dbGetRset(&dbAddr);
    if(prset && prset->put_array_info) {
        put_array_info put_info;
        put_info = (put_array_info)(prset->put_array_info);
        put_info(&dbAddr, length);
    }
    dbScanUnlock(dbAddr.precord);
    channelArrayRequester->setLengthDone(Status::Ok);
    if(lastRequest) destroy();
}

void V3ChannelArray::lock()
{
    dataMutex.lock();
}

void V3ChannelArray::unlock()
{
    dataMutex.unlock();
}

}}

