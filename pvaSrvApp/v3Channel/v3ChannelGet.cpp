/* vsChannelGet.cpp */
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
typedef long (*get_enum_strs) (DBADDR *, struct dbr_enumStrs  *);
}

static int scalarValueBit   = 0x01;
static int arrayValueBit    = 0x02;
static int enumValueBit     = 0x04;
static int timeStampBit     = 0x08;
static int alarmBit         = 0x10;
static int displayBit       = 0x20;
static int controlBit       = 0x40;

static String recordString("record");
static String processString("process");
static String fieldString("field");
static String fieldListString("fieldList");
static String valueString("value");
static String timeStampString("timeStamp");
static String alarmString("alarm");
static String displayString("display");
static String controlString("control");
static String indexString("index");

V3ChannelGet::V3ChannelGet(
    V3Channel &v3Channel,
    ChannelGetRequester &channelGetRequester,
    DbAddr &dbaddr)
: v3Channel(v3Channel),channelGetRequester(channelGetRequester),
  dbaddr(dbaddr),
  getListNode(*this),
  process(false),
  firstTime(true),
  whatMask(0),
  pvStructure(0),bitSet(0),
  pNotify(0),
  notifyAddr(0),
  event()
{
}

V3ChannelGet::~V3ChannelGet() {}


ChannelGetListNode * V3ChannelGet::init(PVStructure &pvRequest)
{
    PVField *pvField = pvRequest.getSubField(recordString);
    if(pvField!=0) {
        PVStructure *pvStructure = static_cast<PVStructure *>(pvField);
        if(pvStructure->getSubField(processString)!=0) {
            PVString *pvString = pvStructure->getStringField(processString);
            if(pvString!=0) {
                String value = pvString->get();
                if(value.compare("true")==0) process = true;
            }
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
        Status invalidPVRequest(Status::STATUSTYPE_ERROR,
            "pvRequest contains no " + fieldListString + " field");
        channelGetRequester.channelGetConnect(invalidPVRequest,0,0,0);
        return 0;
    }
    StandardPVField *standardPVField = getStandardPVField();
    String properties;
    String fieldList = list->get();
    if(fieldList.find(timeStampString)!=String::npos) {
        properties+= timeStampString;
        whatMask |= timeStampBit;
    }
    if(fieldList.find(alarmString)!=String::npos) {
        if(!properties.empty()) properties += ",";
        properties += alarmString;
        whatMask |= alarmBit;
    }
    if(fieldList.find(displayString)!=String::npos) {
        if(!properties.empty()) properties += ",";
        properties += displayString;
        whatMask |= displayBit;
    }
    if(fieldList.find(controlString)!=String::npos) {
        if(!properties.empty()) properties += ",";
        properties += controlString;
        whatMask |= controlBit;
    }
    if(fieldList.find(valueString)!=String::npos) {
        Type type = scalar;
        if(dbaddr.no_elements>1) type = scalarArray;
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
        if(type==scalar&&scalarType!=pvBoolean) {
           whatMask |= scalarValueBit;
           pvStructure = std::auto_ptr<PVStructure>(
               standardPVField->scalarValue(0,scalarType,properties));
        } else if(type==scalarArray) {
            whatMask |= arrayValueBit;
            pvStructure = std::auto_ptr<PVStructure>(
                standardPVField->scalarArrayValue(0,scalarType,properties));
        } else if(dbaddr.field_type==DBF_MENU) {
            whatMask |= enumValueBit;
            dbMenu *pdbMenu = (dbMenu *)(dbaddr.pfldDes->ftPvt);
            unsigned long no_str = pdbMenu->nChoice;
            char **papChoiceValue = pdbMenu->papChoiceValue;
            int32 length = no_str;
            String choices[length];
            for(int i=0; i<length; i++) {
                choices[i] = String(papChoiceValue[i]);
            }
            pvStructure = std::auto_ptr<PVStructure>(
            standardPVField->enumeratedValue(0,choices,length,properties));
        } else if(dbaddr.field_type==DBF_ENUM) {
            whatMask |= enumValueBit;
            struct dbr_enumStrs enumStrs;
            struct rset *prset = dbGetRset(&dbaddr);
            if(prset && prset->get_enum_strs) {
                get_enum_strs get_strs;
                get_strs = (get_enum_strs)(prset->get_enum_strs);
                get_strs(&dbaddr,&enumStrs);
                int32 length = enumStrs.no_str;
                String choices[length];
                for(int i=0; i<length; i++) {
                     choices[i] = String(enumStrs.strs[i]);
                }
                pvStructure = std::auto_ptr<PVStructure>(
                standardPVField->enumeratedValue(0,choices,length,properties));
                    standardPVField->enumeratedValue(0,choices,length);
            } else {
                channelGetRequester.message(
                   String("bad enum field in V3 record"),errorMessage);
                return false;
            }
        } else if(dbaddr.field_type==DBF_DEVICE) {
            whatMask |= enumValueBit;
            dbDeviceMenu *pdbDeviceMenu = (dbDeviceMenu *)
                 (dbaddr.pfldDes->ftPvt);
            if(!pdbDeviceMenu) {
                channelGetRequester.message(
                   String("bad device in V3 record"),errorMessage);
                return false;
            }
            char **papChoiceValue = pdbDeviceMenu->papChoice;
            int32 length = static_cast<int32>(pdbDeviceMenu->nChoice);
            String choices[length];
            for(int i=0; i<length; i++) {
                choices[i] = String(papChoiceValue[i]);
            }
            pvStructure = std::auto_ptr<PVStructure>(
            standardPVField->enumeratedValue(0,choices,length,properties));
        } else {
            channelGetRequester.message(
               String("unsupported field in V3 record"),errorMessage);
            return false;
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
        }
        numFields = pvStructure->getNumberFields();
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
        channelGetRequester.channelGetConnect(
           Status::OK,this,pvStructure.get(),bitSet.get());
    }
    return &getListNode;
}

String V3ChannelGet::getRequesterName() {
    return channelGetRequester.getRequesterName();
}

void V3ChannelGet::message(String message,MessageType messageType)
{
    channelGetRequester.message(message,messageType);
}

void V3ChannelGet::destroy() {
    v3Channel.removeChannelGet(getListNode);
    delete this;
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
    dbScanLock(dbaddr.precord);
    if((whatMask&timeStampBit)!=0) {
        TimeStamp timeStamp;
        PVTimeStamp pvTimeStamp;
        PVField *pvField = pvStructure.get()->getSubField(timeStampString);
        if(!pvTimeStamp.attach(pvField)) {
            throw std::logic_error(String("V3ChannelGet::get logic error"));
        }
        struct dbCommon *precord = dbaddr.precord;
        epicsUInt32 secPastEpoch = precord->time.secPastEpoch;
        epicsUInt32 nsec = precord->time.nsec;
        int64 seconds = secPastEpoch;
        seconds += POSIX_TIME_AT_EPICS_EPOCH;
        int32 nanoseconds = nsec;
        pvTimeStamp.get(timeStamp);
        int64 oldSecs = timeStamp.getSecondsPastEpoch();
        int32 oldNano = timeStamp.getNanoSeconds();
        if(oldSecs!=seconds || oldNano!=nanoseconds) {
            timeStamp.put(seconds,nanoseconds);
            pvTimeStamp.set(timeStamp);
            bitSet->set(pvField->getFieldOffset());
        }
    }
    if((whatMask&alarmBit)!=0) {
        Alarm alarm;
        PVAlarm pvAlarm;
        PVField *pvField = pvStructure.get()->getSubField(alarmString);
        if(!pvAlarm.attach(pvField)) {
            throw std::logic_error(String("V3ChannelGet::get logic error"));
        }
        struct dbCommon *precord = dbaddr.precord;
        epicsEnum16 stat = precord->stat;
        epicsEnum16 sevr = precord->sevr;
        pvAlarm.get(alarm);
        AlarmSeverity prev = alarm.getSeverity();
        epicsEnum16 prevSevr = static_cast<epicsEnum16>(prev);
        if(prevSevr!=sevr) {
            String message("not implemented");
            AlarmSeverity severity = static_cast<AlarmSeverity>(sevr);
            alarm.setSeverity(severity);
            alarm.setMessage(message);
            pvAlarm.set(alarm);
            bitSet->set(pvField->getFieldOffset());
        }
    }
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
    } else if((whatMask&arrayValueBit)!=0) {
        long length = dbaddr.no_elements;
        long offset = 0;
        struct rset *prset = dbGetRset(&dbaddr);
        if(prset && prset->get_array_info) {
            get_array_info get_info;
            get_info = (get_array_info)(prset->get_array_info);
            get_info(&dbaddr, &length, &offset);
            if(offset!=0) {
                dbScanUnlock(dbaddr.precord);
                channelGetRequester.getDone(
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
    } else if((whatMask&enumValueBit)!=0) {
        PVStructure *pvEnum = static_cast<PVStructure *>(pvField);
        PVInt *pvIndex = pvEnum->getIntField(indexString);
        if(dbaddr.field_type==DBF_MENU) {
            epicsEnum16 *value = static_cast<epicsEnum16*>(dbaddr.pfield);
            pvIndex->put(*value);
        } else if(dbaddr.field_type==DBF_ENUM) {
            epicsEnum16 *value = static_cast<epicsEnum16*>(dbaddr.pfield);
            pvIndex->put(*value);
        } else if(dbaddr.field_type==DBF_DEVICE) {
            epicsEnum16 value = static_cast<epicsEnum16>(dbaddr.precord->dtyp);
            pvIndex->put(value);
        }
        if(firstTime) {
            firstTime = false;
            bitSet->set(pvField->getFieldOffset());
        } else {
            bitSet->set(pvIndex->getFieldOffset());
        }
    }
    dbScanUnlock(dbaddr.precord);
    channelGetRequester.getDone(Status::OK);
    
}

void V3ChannelGet::notifyCallback(struct putNotify *pn)
{
    V3ChannelGet * cget = static_cast<V3ChannelGet *>(pn->usrPvt);
    cget->event.signal();
}

}}

