/* v3ChannelArray.cpp */
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

#include <epicsExport.h>

#include <noDefaultMethods.h>
#include <pvIntrospect.h>
#include <pvData.h>
#include <pvAccess.h>
#include "standardField.h"
#include "standardPVField.h"
#include "alarm.h"
#include "control.h"
#include "display.h"
#include "timeStamp.h"
#include "pvAlarm.h"
#include "pvControl.h"
#include "pvDisplay.h"
#include "pvEnumerated.h"
#include "pvTimeStamp.h"

#include "pvDatabase.h"
#include "v3Channel.h"

#include "support.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

extern "C" {
typedef long (*get_array_info) (DBADDR *,long *,long *);
typedef long (*put_array_info) (DBADDR *,long );
}

V3ChannelArray::V3ChannelArray(
    V3Channel &v3Channel,
    ChannelArrayRequester &channelArrayRequester,
    DbAddr &dbaddr)
: v3Channel(v3Channel),channelArrayRequester(channelArrayRequester),
  dbaddr(dbaddr),
  arrayListNode(*this),
  pvScalarArray(0)
{
}

V3ChannelArray::~V3ChannelArray() {}


ChannelArrayListNode * V3ChannelArray::init(PVStructure &pvRequest)
{
    if(!dbaddr.no_elements>1) {
        channelArrayRequester.message(
           String("field in V3 record is not an array"),errorMessage);
        return false;
    }
    ScalarType scalarType(pvBoolean);
    // Note that pvBoolean is not a supported type
    switch(dbaddr.field_type) {
    case DBF_CHAR:
    case DBF_UCHAR:
        scalarType = pvByte; break;
    case DBF_SHORT:
    case DBF_USHORT:
        scalarType = pvShort; break;
    case DBF_LONG:
    case DBF_ULONG:
        scalarType = pvInt; break;
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
        channelArrayRequester.message(
           String("unsupported field in V3 record"),errorMessage);
        return false;
    }
    StandardPVField *standardPVField = getStandardPVField();
    pvScalarArray = std::auto_ptr<PVScalarArray>(
        standardPVField->scalarArrayValue(0,scalarType));
    channelArrayRequester.channelArrayConnect(Status::OK,this,pvScalarArray.get());
    return &arrayListNode;
}

void V3ChannelArray::destroy() {
    v3Channel.removeChannelArray(arrayListNode);
    delete this;
}

void V3ChannelArray::getArray(bool lastRequest,int offset,int count)
{
    dbScanLock(dbaddr.precord);
    long length = 0;
    long v3offset = 0;
    struct rset *prset = dbGetRset(&dbaddr);
    if(prset && prset->get_array_info) {
        get_array_info get_info;
        get_info = (get_array_info)(prset->get_array_info);
        get_info(&dbaddr, &length, &v3offset);
        if(v3offset!=0) {
            dbScanUnlock(dbaddr.precord);
            channelArrayRequester.getArrayDone(
                Status(Status::STATUSTYPE_ERROR,
                   String("v3offset not supported"),
                   String("")));
            return;
        }
    }
    if(count<=0) count = length - offset;
    if((offset+count)>length) count = length -offset;
    if(count<0) {
        dbScanUnlock(dbaddr.precord);
        channelArrayRequester.getArrayDone(Status::OK);
        return;
    }
    pvScalarArray->setLength(count);
    switch(dbaddr.field_type) {
    case DBF_CHAR:
    case DBF_UCHAR: {
        PVByteArray *pv = static_cast<PVByteArray *>(pvScalarArray.get());
        int8 *from = static_cast<int8 *>(dbaddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_SHORT:
    case DBF_USHORT: {
        PVShortArray *pv = static_cast<PVShortArray *>(pvScalarArray.get());
        int16 *from = static_cast<int16 *>(dbaddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_LONG:
    case DBF_ULONG: {
        PVIntArray *pv = static_cast<PVIntArray *>(pvScalarArray.get());
        int32 *from = static_cast<int32 *>(dbaddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_FLOAT: {
        PVFloatArray *pv = static_cast<PVFloatArray *>(pvScalarArray.get());
        float *from = static_cast<float *>(dbaddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_DOUBLE: {
        PVDoubleArray *pv = static_cast<PVDoubleArray *>(pvScalarArray.get());
        double *from = static_cast<double *>(dbaddr.pfield);
        pv->put(0,count,from,offset);
        break;
    }
    case DBF_STRING: {
        PVStringArray *pv = static_cast<PVStringArray *>(pvScalarArray.get());
        StringArrayData data;
        pv->get(0,length,&data);
        int index = 0;
        char *pchar = static_cast<char *>(dbaddr.pfield);
        pchar += dbaddr.field_size*offset;
        while(index<count) {
            data.data[index] = String(pchar);
            index++;
            pchar += dbaddr.field_size;
        }
        break;
    }
    }
    dbScanUnlock(dbaddr.precord);
    channelArrayRequester.getArrayDone(Status::OK);
}

void V3ChannelArray::putArray(bool lastRequest,int offset,int count)
{
    dbScanLock(dbaddr.precord);
    long no_elements = dbaddr.no_elements;
    if((offset+count)>no_elements) count = no_elements - count;
    if(count<=0) {
        dbScanUnlock(dbaddr.precord);
        channelArrayRequester.getArrayDone(Status::OK);
        return;
    }
    long length = offset + count;
    struct rset *prset = dbGetRset(&dbaddr);
    if(prset && prset->get_array_info) {
        long oldLength = 0;
        long v3offset = 0;
        get_array_info get_info;
        get_info = (get_array_info)(prset->get_array_info);
        get_info(&dbaddr, &oldLength, &v3offset);
        if(length>oldLength) {
           if(prset && prset->put_array_info) {
               put_array_info put_info;
               put_info = (put_array_info)(prset->put_array_info);
               put_info(&dbaddr, length);
           }
        }
    }
    switch(dbaddr.field_type) {
    case DBF_CHAR:
    case DBF_UCHAR: {
        PVByteArray *pv = static_cast<PVByteArray *>(pvScalarArray.get());
        ByteArrayData data;
        pv->get(0,count,&data);
        int8 *to = static_cast<int8 *>(dbaddr.pfield);
        int8 *from = data.data;
        for(int i=0; i<count; i++)  to[offset+i] = from[i];
        break;
    }
    case DBF_SHORT:
    case DBF_USHORT: {
        PVShortArray *pv = static_cast<PVShortArray *>(pvScalarArray.get());
        ShortArrayData data;
        pv->get(0,count,&data);
        int16 *to = static_cast<int16 *>(dbaddr.pfield);
        int16 *from = data.data;
        for(int i=0; i<count; i++)  to[offset+i] = from[i];
        break;
    }
    case DBF_LONG:
    case DBF_ULONG: {
        PVIntArray *pv = static_cast<PVIntArray *>(pvScalarArray.get());
        IntArrayData data;
        pv->get(0,count,&data);
        int32 *to = static_cast<int32 *>(dbaddr.pfield);
        int32 *from = data.data;
        for(int i=0; i<count; i++)  to[offset+i] = from[i];
        break;
    }
    case DBF_FLOAT: {
        PVFloatArray *pv = static_cast<PVFloatArray *>(pvScalarArray.get());
        FloatArrayData data;
        pv->get(0,count,&data);
        float *to = static_cast<float *>(dbaddr.pfield);
        float *from = data.data;
        for(int i=0; i<count; i++)  to[offset+i] = from[i];
        break;
        break;
    }
    case DBF_DOUBLE: {
        PVDoubleArray *pv = static_cast<PVDoubleArray *>(pvScalarArray.get());
        DoubleArrayData data;
        pv->get(0,count,&data);
        double *to = static_cast<double *>(dbaddr.pfield);
        double *from = data.data;
        for(int i=0; i<count; i++)  to[offset+i] = from[i];
        break;
    }
    case DBF_STRING: {
        PVStringArray *pv = static_cast<PVStringArray *>(pvScalarArray.get());
        StringArrayData data;
        pv->get(0,length,&data);
        int index = 0;
        char *to = static_cast<char *>(dbaddr.pfield);
        int len = dbaddr.field_size;
        while(index<length) {
            const char * from = data.data[index].c_str();
            if(from!=0) {
                strncpy(to,from,len-1);
            }
            *(to + len -1) = 0;
            to += len;
            index++;
        }
        break;
    }
    }
    dbScanUnlock(dbaddr.precord);
    channelArrayRequester.getArrayDone(Status::OK);
}

void V3ChannelArray::setLength(bool lastRequest,int length,int capacity)
{
    dbScanLock(dbaddr.precord);
    long no_elements = dbaddr.no_elements;
    if(length>no_elements) length = no_elements;
    struct rset *prset = dbGetRset(&dbaddr);
    if(prset && prset->put_array_info) {
        put_array_info put_info;
        put_info = (put_array_info)(prset->put_array_info);
        put_info(&dbaddr, length);
    }
    dbScanUnlock(dbaddr.precord);
    channelArrayRequester.setLengthDone(Status::OK);
}

}}

