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

#ifndef RECORDV3_H
#define RECORDV3_H
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

V3ChannelGet::V3ChannelGet(
    V3Channel &v3Channel,
    epics::pvAccess::ChannelGetRequester &channelGetRequester,
    DbAddr &dbaddr)
: v3Channel(v3Channel),channelGetRequester(channelGetRequester),
  dbaddr(dbaddr),
  getListNode(*this),
  process(false),
  whatMask(0),
  pvStructure(0),bitSet(0)
{
}

V3ChannelGet::~V3ChannelGet() {}


ChannelGetListNode * V3ChannelGet::init(PVStructure &pvRequest)
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
    if(list==0) return 0;
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
          channelGetRequester.message(String("no support for field type"),errorMessage);
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
    } else if((whatMask&scalarValueBit)!=0) {
    }
    
    channelGetRequester.getDone(Status::OK);
    
}

}}

#endif  /* RECORDV3_H */
