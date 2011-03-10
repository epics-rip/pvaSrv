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

#include <cantProceed.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <epicsExit.h>
#include <dbAccess.h>
#include <dbNotify.h>

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
#include <recSup.h>

#include "pvDatabase.h"
#include "v3Channel.h"

#include "support.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

extern "C" {
typedef long (*get_array_info) (void *,long *,long *);
typedef long (*put_array_info) (void *,long);
}

static int scalarValueBit   = 0x01;
static int arrayValueBit    = 0x02;
static int enumValueBit     = 0x04;

static String recordString("record");
static String processString("process");
static String fieldString("field");
static String fieldListString("fieldList");
static String valueString("value");

V3ChannelPut::V3ChannelPut(
    V3Channel &v3Channel,
    ChannelPutRequester &channelPutRequester,
    DbAddr &dbaddr)
: v3Channel(v3Channel),channelPutRequester(channelPutRequester),
  dbaddr(dbaddr),
  putListNode(*this),
  process(false),
  whatMask(0),
  pvStructure(0),bitSet(0),
  pNotify(0),
  notifyAddr(0),
  event()
{
}

V3ChannelPut::~V3ChannelPut() {}


ChannelPutListNode * V3ChannelPut::init(PVStructure &pvRequest)
{
    PVField *pvField = pvRequest.getSubField(recordString);
    if(pvField!=0) {
        PVStructure *pvStructure = static_cast<PVStructure *>(pvField);
        PVString *pvString = pvStructure->getStringField(processString);
        if(pvString!=0) {
            String value = pvString->get();
            if(value.compare("true")==0) process = true;
        }
    }
    pvField = pvRequest.getSubField(fieldString);
    PVString *list = 0;
    if(pvField!=0) {
        PVStructure * pvStructure = static_cast<PVStructure * >(pvField);
        list = pvStructure->getStringField(fieldListString);
    } else {
        list = pvRequest.getStringField(fieldListString);
    }
    if(list==0) {
        Status invalidPVRequest(Status::STATUSTYPE_ERROR, "pvRequest contains no " + fieldListString + " field");
        channelPutRequester.channelPutConnect(invalidPVRequest,0,0,0);
        return 0;
    }
    StandardPVField *standardPVField = getStandardPVField();
    String properties;
    String fieldList = list->get();
    if(fieldList.find(valueString)!=String::npos) {
        Type type = scalar;
        if(dbaddr.no_elements>1) type = scalarArray;
        ScalarType scalarType(pvBoolean);
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
          //MARTY MUST HANDLE ENUM,and MENU
          channelPutRequester.message(
              String("no support for field type"),errorMessage);
          return 0;
        }
        if(type==scalar) {
           whatMask |= scalarValueBit;
           pvStructure = std::auto_ptr<PVStructure>(
               standardPVField->scalarValue(0,scalarType,properties));
        } else {
           whatMask |= arrayValueBit;
           pvStructure = std::auto_ptr<PVStructure>(
               standardPVField->scalarArrayValue(0,scalarType,properties));
        }
        int numFields = pvStructure->getNumberFields();
        bitSet = std::auto_ptr<BitSet>(new BitSet(numFields));
        if(process) {
           pNotify = std::auto_ptr<struct putNotify>(new (struct putNotify)());
           notifyAddr = std::auto_ptr<DbAddr>(new DbAddr());
           memcpy(notifyAddr.get(),&dbaddr,sizeof(DbAddr));
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
        channelPutRequester.channelPutConnect(
           Status::OK,this,pvStructure.get(),bitSet.get());
    }
    return &putListNode;
}

String V3ChannelPut::getRequesterName() {
    return channelPutRequester.getRequesterName();
}

void V3ChannelPut::message(String message,MessageType messageType)
{
    channelPutRequester.message(message,messageType);
}

void V3ChannelPut::destroy() {
    v3Channel.removeChannelPut(putListNode);
    delete this;
}

void V3ChannelPut::put(bool lastRequest)
{
    PVField *pvField = pvStructure.get()->getSubField(valueString);
    dbScanLock(dbaddr.precord);
    if((whatMask&scalarValueBit)!=0) {
        switch(dbaddr.field_type) {
        case DBF_CHAR:
        case DBF_UCHAR: {
            int8 * val = static_cast<int8 *>(dbaddr.pfield);
            PVByte *pv = static_cast<PVByte *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_SHORT:
        case DBF_USHORT: {
            int16 * val = static_cast<int16 *>(dbaddr.pfield);
            PVShort *pv = static_cast<PVShort *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_LONG:
        case DBF_ULONG: {
            int32 * val = static_cast<int32 *>(dbaddr.pfield);
            PVInt *pv = static_cast<PVInt *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_FLOAT: {
            float * val = static_cast<float *>(dbaddr.pfield);
            PVFloat *pv = static_cast<PVFloat *>(pvField);
            pv->put(*val);
            break;
        }
        case DBF_DOUBLE: {
            double * val = static_cast<double *>(dbaddr.pfield);
            PVDouble *pv = static_cast<PVDouble *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_STRING: {
            char * val = static_cast<char *>(dbaddr.pfield);
            String sval(val);
            PVString *pvString = static_cast<PVString *>(pvField);
            memcpy(val,pvString->get().c_str(),sizeof(double));
        }
        }
    } else if((whatMask&arrayValueBit)!=0) {
        long length = dbaddr.no_elements;
        PVArray *pvArray = static_cast<PVArray *>(pvField);
        int len = pvArray->getLength();
        if(len<length) length = len;
        struct rset *prset = dbGetRset(&dbaddr);
        if(prset && prset->put_array_info) {
            put_array_info put_info;
            put_info = (put_array_info)(prset->put_array_info);
            put_info(&dbaddr, length);
         }
        int field_size = dbaddr.field_size;
        switch(dbaddr.field_type) {
        case DBF_CHAR:
        case DBF_UCHAR: {
            PVByteArray *pv = static_cast<PVByteArray *>(pvField);
            ByteArrayData data;
            pv->get(0,length,&data);
            memcpy(dbaddr.pfield,data.data,length*field_size);
            break;
        }
        case DBF_SHORT:
        case DBF_USHORT: {
            PVShortArray *pv = static_cast<PVShortArray *>(pvField);
            ShortArrayData data;
            pv->get(0,length,&data);
            memcpy(dbaddr.pfield,data.data,length*field_size);
            break;
        }
        case DBF_LONG:
        case DBF_ULONG: {
            PVIntArray *pv = static_cast<PVIntArray *>(pvField);
            IntArrayData data;
            pv->get(0,length,&data);
            memcpy(dbaddr.pfield,data.data,length*field_size);
            break;
        }
        case DBF_FLOAT: {
            PVFloatArray *pv = static_cast<PVFloatArray *>(pvField);
            FloatArrayData data;
            pv->get(0,length,&data);
            memcpy(dbaddr.pfield,data.data,length*field_size);
            break;
        }
        case DBF_DOUBLE: {
            PVDoubleArray *pv = static_cast<PVDoubleArray *>(pvField);
            DoubleArrayData data;
            pv->get(0,length,&data);
            memcpy(dbaddr.pfield,data.data,length*field_size);
            break;
        }
        case DBF_STRING: {
            PVStringArray *pv = static_cast<PVStringArray *>(pvField);
            StringArrayData data;
            pv->get(0,length,&data);
            int index = 0;
            char *pchar = static_cast<char *>(dbaddr.pfield);
            while(index<length) {
                strcpy(pchar,data.data[index].c_str());
                index++;
                pchar += MAX_STRING_SIZE;
            }
            break;
         }
         }
    }
    dbScanUnlock(dbaddr.precord);
    if(process) {
        epicsUInt8 value = 1;
        pNotify.get()->pbuffer = &value;
        dbPutNotify(pNotify.get());
        event.wait();
    }
    channelPutRequester.putDone(Status::OK);
    
}

void V3ChannelPut::notifyCallback(struct putNotify *pn)
{
    V3ChannelPut * cput = static_cast<V3ChannelPut *>(pn->usrPvt);
    cput->event.signal();
}

void V3ChannelPut::get()
{
    bitSet->clear();
    dbScanLock(dbaddr.precord);
    PVField *pvField = pvStructure.get()->getSubField(valueString);
    if((whatMask&scalarValueBit)!=0) {
        switch(dbaddr.field_type) {
        case DBF_CHAR:
        case DBF_UCHAR: {
            int8 * val = static_cast<int8 *>(dbaddr.pfield);
            PVByte *pv = static_cast<PVByte *>(pvField);
            pv->put(*val);
            break;
        }
        case DBF_SHORT:
        case DBF_USHORT: {
            int16 * val = static_cast<int16 *>(dbaddr.pfield);
            PVShort *pv = static_cast<PVShort *>(pvField);
            pv->put(*val);
            break;
        }
        case DBF_LONG:
        case DBF_ULONG: {
            int32 * val = static_cast<int32 *>(dbaddr.pfield);
            PVInt *pv = static_cast<PVInt *>(pvField);
            pv->put(*val);
            break;
        }
        case DBF_FLOAT: {
            float * val = static_cast<float *>(dbaddr.pfield);
            PVFloat *pv = static_cast<PVFloat *>(pvField);
            pv->put(*val);
            break;
        }
        case DBF_DOUBLE: {
            double * val = static_cast<double *>(dbaddr.pfield);
            PVDouble *pv = static_cast<PVDouble *>(pvField);
            pv->put(*val);
            break;
        }
        case DBF_STRING: {
            char * val = static_cast<char *>(dbaddr.pfield);
            String sval(val);
            PVString *pvString = static_cast<PVString *>(pvField);
            pvString->put(sval);
        }
        }
        bitSet->set(pvField->getFieldOffset());
    } else if((whatMask&scalarValueBit)!=0) {
        long length = dbaddr.no_elements;
        long offset = 0;
        struct rset *prset = dbGetRset(&dbaddr);
        if(prset && prset->get_array_info) {
            get_array_info get_info;
            get_info = (get_array_info)(prset->get_array_info);
            get_info(&dbaddr, &length, &offset);
            if(offset!=0) {
                dbScanUnlock(dbaddr.precord);
                channelPutRequester.getDone(
                    Status(Status::STATUSTYPE_ERROR,
                       String("offset not supported"),
                       String("")));
                return;
            }
        }
        int field_size = dbaddr.field_size;
        PVArray *pvArray = static_cast<PVArray *>(pvField);
        int capacity = pvArray->getCapacity();
        if(capacity!=length) {
            pvArray->setCapacity(length);
            pvArray->setLength(length);
        }
        switch(dbaddr.field_type) {
        case DBF_CHAR:
        case DBF_UCHAR: {
            PVByteArray *pv = static_cast<PVByteArray *>(pvField);
            ByteArrayData data;
            pv->get(0,length,&data);
            memcpy(data.data,dbaddr.pfield,length*field_size);
            pv->postPut();
            break;
        }
        case DBF_SHORT:
        case DBF_USHORT: {
            PVShortArray *pv = static_cast<PVShortArray *>(pvField);
            ShortArrayData data;
            pv->get(0,length,&data);
            memcpy(data.data,dbaddr.pfield,length*field_size);
            pv->postPut();
            break;
        }
        case DBF_LONG:
        case DBF_ULONG: {
            PVIntArray *pv = static_cast<PVIntArray *>(pvField);
            IntArrayData data;
            pv->get(0,length,&data);
            memcpy(data.data,dbaddr.pfield,length*field_size);
            pv->postPut();
            break;
        }
        case DBF_FLOAT: {
            PVFloatArray *pv = static_cast<PVFloatArray *>(pvField);
            FloatArrayData data;
            pv->get(0,length,&data);
            memcpy(data.data,dbaddr.pfield,length*field_size);
            pv->postPut();
            break;
        }
        case DBF_DOUBLE: {
            PVDoubleArray *pv = static_cast<PVDoubleArray *>(pvField);
            DoubleArrayData data;
            pv->get(0,length,&data);
            memcpy(data.data,dbaddr.pfield,length*field_size);
            pv->postPut();
            break;
        }
        case DBF_STRING: {
            PVStringArray *pv = static_cast<PVStringArray *>(pvField);
            StringArrayData data;
            pv->get(0,length,&data);
            int index = 0;
            char *pchar = static_cast<char *>(dbaddr.pfield);
            while(index<length) {
                data.data[index] = String(pchar);
                index++;
                pchar += MAX_STRING_SIZE;
            }
            pv->postPut();
            break;
        }
        }
        bitSet->set(pvField->getFieldOffset());
    }
    dbScanUnlock(dbaddr.precord);
    channelPutRequester.getDone(Status::OK);
}

}}

