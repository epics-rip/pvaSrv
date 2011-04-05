/* v3Util.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#include <string>
#include <cstring>
#include <stdexcept>

#include <dbAccess.h>
#include <dbNotify.h>

#include <pvIntrospect.h>
#include <pvData.h>
#include <pvEnumerated.h>
#include <pvAccess.h>
#include <standardField.h>
#include <standardPVField.h>
#include <alarm.h>
#include <control.h>
#include <display.h>
#include <timeStamp.h>
#include "pvAlarm.h"
#include "pvTimeStamp.h"
#include "pvControl.h"
#include "pvDisplay.h"

#include "v3Util.h"
#include "v3Array.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;

int V3Util::processBit       = 0x0001;
int V3Util::shareArrayBit    = 0x0002;
int V3Util::scalarValueBit   = 0x0004;
int V3Util::arrayValueBit    = 0x0008;
int V3Util::enumValueBit     = 0x0010;
int V3Util::timeStampBit     = 0x0020;
int V3Util::alarmBit         = 0x0040;
int V3Util::displayBit       = 0x0080;
int V3Util::controlBit       = 0x0100;

String V3Util::recordString("record");
String V3Util::processString("record.process");
String V3Util::queueSizeString("record.queueSize");
String V3Util::recordShareString("record.shareData");
String V3Util::fieldString("field");
String V3Util::fieldListString("fieldList");
String V3Util::valueString("value");
String V3Util::valueShareArrayString("value.leaf.shareData");
String V3Util::timeStampString("timeStamp");
String V3Util::alarmString("alarm");
String V3Util::displayString("display");
String V3Util::controlString("control");
String V3Util::allString("value,timeStamp,alarm,display,control");
String V3Util::indexString("index");



int V3Util::getProperties(
    Requester &requester,PVStructure &request,DbAddr &dbAddr)
{
    int propertyMask = 0;
    PVField *pvField = request.getSubField(processString);
    if(pvField!=0) {
        PVString *pvString = request.getStringField(processString);
        if(pvString!=0) {
            String value = pvString->get();
            if(value.compare("true")==0) propertyMask |= processBit;
        }
    }
    PVStructure *pvRequest = &request;
    PVField *pvTemp = request.getSubField(fieldString);
    if(pvTemp!=0) pvRequest = static_cast<PVStructure * >(pvTemp);
    PVString *pvShareString = 0;
    pvField = request.getSubField(recordShareString);
    if(pvField!=0) {
         pvShareString = request.getStringField(recordShareString);
    } else {
         pvField = pvRequest->getSubField(valueShareArrayString);
         if(pvField!=0) pvShareString =
             pvRequest->getStringField(valueShareArrayString);
    }
    if(pvShareString!=0) {
        String value = pvShareString->get();
        if(value.compare("true")==0) propertyMask |= shareArrayBit;
    }
    bool getValue;
    String fieldList;
    if(pvRequest->getStructure()->getNumberFields()==0) {
        getValue = true;
        fieldList = allString;
    } else if(pvRequest->getSubField(fieldListString)!=0) {
        PVString *pvStr = pvRequest->getStringField(fieldListString);
        if(pvStr!=0) fieldList = pvStr->get();
    }
    if(fieldList.length()>0) {
        if(fieldList.find(valueString)!=String::npos) getValue = true;
    } else {
        if(pvRequest->getSubField(valueString)!=0) getValue = true;
    }
    if(getValue) {
        Type type = scalar;
        if(dbAddr.no_elements>1) type = scalarArray;
        ScalarType scalarType(pvBoolean);
        // Note that pvBoolean is not a supported type
        switch(dbAddr.field_type) {
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
           propertyMask |= scalarValueBit;
        } else if(type==scalarArray) {
            propertyMask |= arrayValueBit;
        } else if(dbAddr.field_type==DBF_ENUM) {
            propertyMask |= enumValueBit;
        } else {
            requester.message(
               String("unsupported field in V3 record"),errorMessage);
            return 0;
        }
    }
    if(fieldList.length()!=0) {
        if(fieldList.find(timeStampString)!=String::npos) {
            propertyMask |= timeStampBit;
        }
        if(fieldList.find(alarmString)!=String::npos) {
            propertyMask |= alarmBit;
        }
        if(fieldList.find(displayString)!=String::npos) {
            if(dbAddr.field_type==DBF_LONG||dbAddr.field_type==DBF_DOUBLE) {
                propertyMask |= displayBit;
            }
        }
        if(fieldList.find(controlString)!=String::npos) {
            if(dbAddr.field_type==DBF_LONG||dbAddr.field_type==DBF_DOUBLE) {
                propertyMask |= controlBit;
            }
        }
    } else {
        pvField = pvRequest->getSubField(timeStampString);
        if(pvField!=0) {
             PVString *pvString = pvRequest->getStringField(timeStampString);
             if(pvString!=0) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= timeStampBit;
                  }
             }
        }
        pvField = pvRequest->getSubField(alarmString);
        if(pvField!=0) {
             PVString *pvString = pvRequest->getStringField(alarmString);
             if(pvString!=0) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= alarmBit;
                  }
             }
        }
        pvField = pvRequest->getSubField(displayString);
        if(pvField!=0) {
             PVString *pvString = pvRequest->getStringField(displayString);
             if(pvString!=0) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= displayBit;
                  }
             }
        }
        pvField = pvRequest->getSubField(controlString);
        if(pvField!=0) {
             PVString *pvString = pvRequest->getStringField(controlString);
             if(pvString!=0) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= controlBit;
                  }
             }
        }
    }
    return propertyMask;
}

PVStructure *V3Util::createPVStructure(
    Requester &requester,int propertyMask,DbAddr &dbAddr)
{
    StandardPVField *standardPVField = getStandardPVField();
    String properties;
    if((propertyMask&timeStampBit)!=0) properties+= timeStampString;
    if((propertyMask&alarmBit)!=0) {
        if(!properties.empty()) properties += ",";
        properties += alarmString;
    }
    if((propertyMask&displayBit)!=0) {
        if(!properties.empty()) properties += ",";
        properties += displayString;
    }
    if((propertyMask&controlBit)!=0) {
        if(!properties.empty()) properties += ",";
        properties += controlString;
    }
    if((propertyMask&enumValueBit)!=0) {
        struct dbr_enumStrs enumStrs;
        struct rset *prset = dbGetRset(&dbAddr);
        if(prset && prset->get_enum_strs) {
            get_enum_strs get_strs;
            get_strs = (get_enum_strs)(prset->get_enum_strs);
            get_strs(&dbAddr,&enumStrs);
            int32 length = enumStrs.no_str;
            String choices[length];
            for(int i=0; i<length; i++) {
                 choices[i] = String(enumStrs.strs[i]);
            }
            PVStructure *pvStructure = standardPVField->enumeratedValue(
                     0,choices,length,properties);
            return pvStructure;
        } else {
            requester.message(
               String("bad enum field in V3 record"),errorMessage);
            return 0;
        }
    }
    ScalarType scalarType(pvBoolean);
    // Note that pvBoolean is not a supported type
    switch(dbAddr.field_type) {
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
            throw std::logic_error(String("Should never get here"));
    }
    if((propertyMask&scalarValueBit)!=0) {
        PVStructure *pvStructure =
            standardPVField->scalarValue(0,scalarType,properties);
        return pvStructure;
    }
    if((propertyMask&arrayValueBit)!=0) {
         bool share = (propertyMask&shareArrayBit) ? true : false;
         PVStructure *pvParent = getPVDataCreate()->createPVStructure(
             0,String(),0,0);
         PVFieldPtr pvField = 0;
         ScalarArrayConstPtr scalarArray = getStandardField()->scalarArray(
              valueString,scalarType);
         V3ValueArrayCreate *v3ValueArrayCreate = getV3ValueArrayCreate();
         switch(scalarType) {
         case pvByte:
            pvField = v3ValueArrayCreate->createByteArray(
                pvParent,scalarArray,dbAddr,share);
            break;
         case pvShort:
            pvField = v3ValueArrayCreate->createShortArray(
                pvParent,scalarArray,dbAddr,share);
            break;
         case pvInt:
            pvField = v3ValueArrayCreate->createIntArray(
                pvParent,scalarArray,dbAddr,share);
            break;
         case pvFloat:
            pvField = v3ValueArrayCreate->createFloatArray(
                pvParent,scalarArray,dbAddr,share);
            break;
         case pvDouble:
            pvField = v3ValueArrayCreate->createDoubleArray(
                pvParent,scalarArray,dbAddr,share);
            break;
         case pvString:
            pvField = v3ValueArrayCreate->createStringArray(
                pvParent,scalarArray,dbAddr);
            break;
         default:
            throw std::logic_error(String("Should never get here"));
         }
         pvParent->appendPVField(pvField);
         if((propertyMask&timeStampBit)!=0) {
            pvParent->appendPVField(standardPVField->timeStamp(pvParent));
         }
         if((propertyMask&alarmBit)!=0) {
            pvParent->appendPVField(standardPVField->alarm(pvParent));
         }
         if((propertyMask&displayBit)!=0) {
            pvParent->appendPVField(standardPVField->display(pvParent));
         }
         if((propertyMask&controlBit)!=0) {
            pvParent->appendPVField(standardPVField->control(pvParent));
         }
         return pvParent;
    }
    requester.message(String("did not ask for value"),errorMessage);
    return 0;
}

void  V3Util::getPropertyData(
        Requester &requester,
        int propertyMask,DbAddr &dbAddr,
        PVStructure &pvStructure)
{
    if(propertyMask&displayBit) {
        Display display;
        char units[DB_UNITS_SIZE];
        units[0] = 0;
        long precision = 0;
        struct rset *prset = dbGetRset(&dbAddr);
        if(prset && prset->get_units) {
            get_units gunits;
            gunits = (get_units)(prset->get_units);
            gunits(&dbAddr,units);
            display.setUnits(String(units));
        }
        if(prset && prset->get_precision) {
            get_precision gprec = (get_precision)(prset->get_precision);
            gprec(&dbAddr,&precision);
            String format("%f");
            if(precision>0) {
                format += "." + precision;
                display.setFormat(format);
            }
        }
        struct dbr_grDouble graphics;
        if(prset && prset->get_graphic_double) {
           get_graphic_double gg =
                (get_graphic_double)(prset->get_graphic_double);
           gg(&dbAddr,&graphics);
           display.setHigh(graphics.upper_disp_limit);
           display.setLow(graphics.lower_disp_limit);
        }
        PVDisplay pvDisplay;
        PVField *pvField = pvStructure.getSubField(displayString);
        pvDisplay.attach(pvField);
        pvDisplay.set(display);
    }
    if(propertyMask&controlBit) {
        Control control;
        struct rset *prset = dbGetRset(&dbAddr);
        struct dbr_ctrlDouble graphics;
        if(prset && prset->get_control_double) {
           get_control_double cc =
                (get_control_double)(prset->get_control_double);
           cc(&dbAddr,&graphics);
           control.setHigh(graphics.upper_ctrl_limit);
           control.setLow(graphics.lower_ctrl_limit);
        }
        PVControl pvControl;
        PVField *pvField = pvStructure.getSubField(controlString);
        pvControl.attach(pvField);
        pvControl.set(control);
    }
}

Status  V3Util::get(
        Requester &requester,
        int propertyMask,DbAddr &dbAddr,
        PVStructure &pvStructure,
        BitSet &bitSet,
        CAV3Data *caV3Data)
{
    PVFieldPtrArray pvFields = pvStructure.getPVFields();
    PVFieldPtr pvField = pvFields[0];
    if((propertyMask&V3Util::scalarValueBit)!=0) {
        PVScalar* pvScalar = static_cast<PVScalar *>(pvField);
        ScalarType scalarType = pvScalar->getScalar()->getScalarType();
        bool wasChanged = false;
        switch(scalarType) {
        case pvByte: {
            int8 val = 0;
            if(caV3Data) {
                val = caV3Data->byteValue;
            } else {
                val = *static_cast<int8 *>(dbAddr.pfield);
            }
            PVByte *pv = static_cast<PVByte *>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvShort: {
            int16 val = 0;
            if(caV3Data) {
                val = caV3Data->shortValue;
            } else {
                val = *static_cast<int16 *>(dbAddr.pfield);
            }
            PVShort *pv = static_cast<PVShort *>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvInt: {
            int32 val = 0;
            if(caV3Data) {
                val = caV3Data->intValue;
            } else {
                val = *static_cast<int32 *>(dbAddr.pfield);
            }
            PVInt *pv = static_cast<PVInt *>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvFloat: {
            float val = 0;
            if(caV3Data) {
                val = caV3Data->floatValue;
            } else {
                val = *static_cast<float *>(dbAddr.pfield);
            }
            PVFloat *pv = static_cast<PVFloat *>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvDouble: {
            double val = 0;
            if(caV3Data) {
                val = caV3Data->doubleValue;
            } else {
                val = *static_cast<double *>(dbAddr.pfield);
            }
            PVDouble *pv = static_cast<PVDouble *>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvString: {
            char * val = static_cast<char *>(dbAddr.pfield);
            String sval(val);
            PVString *pvString = static_cast<PVString *>(pvField);
            if((pvString->get().compare(sval))!=0) {
                pvString->put(sval);
                wasChanged = true;
            }
            break;
        }
        default:
             throw std::logic_error(String("Should never get here"));
        }
        if(wasChanged) bitSet.set(pvField->getFieldOffset());
    } else if((propertyMask&arrayValueBit)!=0) {
        // nothing to do serialize takes care of it
        bitSet.set(pvField->getFieldOffset());
    } else if((propertyMask&enumValueBit)!=0) {
        PVStructure *pvEnum = static_cast<PVStructure *>(pvField);
        int32 val = 0;
        if(caV3Data) {
            val = caV3Data->intValue;
        } else {
            if(dbAddr.field_type==DBF_DEVICE) {
                val = static_cast<epicsEnum16>(dbAddr.precord->dtyp);
            } else {
                val = *static_cast<int32 *>(dbAddr.pfield);
            }
        }
        PVInt *pvIndex = pvEnum->getIntField(indexString);
        if(pvIndex->get()!=val) pvIndex->put(val);
        bitSet.set(pvIndex->getFieldOffset());
    }
    if((propertyMask&timeStampBit)!=0) {
        TimeStamp timeStamp;
        PVTimeStamp pvTimeStamp;
        PVField *pvField = pvStructure.getSubField(timeStampString);
        if(!pvTimeStamp.attach(pvField)) {
            throw std::logic_error(String("V3ChannelGet::get logic error"));
        }
        epicsTimeStamp *epicsTimeStamp;
        struct dbCommon *precord = dbAddr.precord;
        if(caV3Data) {
            epicsTimeStamp = &caV3Data->timeStamp;
        } else {
            epicsTimeStamp = &precord->time;     
        }
        epicsUInt32 secPastEpoch = epicsTimeStamp->secPastEpoch;
        epicsUInt32 nsec = epicsTimeStamp->nsec;
        int64 seconds = secPastEpoch;
        seconds += POSIX_TIME_AT_EPICS_EPOCH;
        int32 nanoseconds = nsec;
        pvTimeStamp.get(timeStamp);
        int64 oldSecs = timeStamp.getSecondsPastEpoch();
        int32 oldNano = timeStamp.getNanoSeconds();
        if(oldSecs!=seconds || oldNano!=nanoseconds) {
            timeStamp.put(seconds,nanoseconds);
            pvTimeStamp.set(timeStamp);
            bitSet.set(pvField->getFieldOffset());
        }
    }
     if((propertyMask&alarmBit)!=0) {
        Alarm alarm;
        PVAlarm pvAlarm;
        PVField *pvField = pvStructure.getSubField(alarmString);
        if(!pvAlarm.attach(pvField)) {
            throw std::logic_error(String("V3ChannelGet::get logic error"));
        }
        struct dbCommon *precord = dbAddr.precord;
        const char * stat = "";
        epicsEnum16 sevr;
        if(caV3Data) {
            stat = caV3Data->status;
            sevr = caV3Data->sevr;
        } else {
            stat = epicsAlarmConditionStrings[precord->stat];
            sevr = precord->sevr;
        }
        pvAlarm.get(alarm);
        AlarmSeverity prev = alarm.getSeverity();
        epicsEnum16 prevSevr = static_cast<epicsEnum16>(prev);
        if(prevSevr!=sevr) {
            String message(stat);
            AlarmSeverity severity = static_cast<AlarmSeverity>(sevr);
            alarm.setSeverity(severity);
            alarm.setMessage(message);
            pvAlarm.set(alarm);
            bitSet.set(pvField->getFieldOffset());
        }
    }
    return Status::OK;
}

Status  V3Util::put(
        Requester &requester,
        int propertyMask,DbAddr &dbAddr,
        PVStructure &pvStructure)
{
    PVFieldPtrArray pvFields = pvStructure.getPVFields();
    PVFieldPtr pvField = pvFields[0];
    if((propertyMask&scalarValueBit)!=0) {
        switch(dbAddr.field_type) {
        case DBF_CHAR:
        case DBF_UCHAR: {
            int8 * val = static_cast<int8 *>(dbAddr.pfield);
            PVByte *pv = static_cast<PVByte *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_SHORT:
        case DBF_USHORT: {
            int16 * val = static_cast<int16 *>(dbAddr.pfield);
            PVShort *pv = static_cast<PVShort *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_LONG:
        case DBF_ULONG: {
            int32 * val = static_cast<int32 *>(dbAddr.pfield);
            PVInt *pv = static_cast<PVInt *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_FLOAT: {
            float * val = static_cast<float *>(dbAddr.pfield);
            PVFloat *pv = static_cast<PVFloat *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_DOUBLE: {
            double * val = static_cast<double *>(dbAddr.pfield);
            PVDouble *pv = static_cast<PVDouble *>(pvField);
            *val = pv->get();
            break;
        }
        case DBF_STRING: {
            char * to = static_cast<char *>(dbAddr.pfield);
            PVString *pvString = static_cast<PVString *>(pvField);
            int len = dbAddr.field_size;
            strncpy(to,pvString->get().c_str(),len -1);
            *(to + len -1) = 0;
        }
        }
    } else if((propertyMask&arrayValueBit)!=0) {
         // nothing to do deserialization did it
    } else if((propertyMask&enumValueBit)!=0) {
        PVStructure *pvEnum = static_cast<PVStructure *>(pvField);
        PVInt *pvIndex = pvEnum->getIntField(indexString);
        if(dbAddr.field_type==DBF_MENU) {
            requester.message(
                String("Not allowed to change a menu field"),errorMessage);
        } else if(dbAddr.field_type==DBF_ENUM) {
            epicsEnum16 *value = static_cast<epicsEnum16*>(dbAddr.pfield);
            *value = pvIndex->get();
        } else if(dbAddr.field_type==DBF_DEVICE) {
            requester.message(
                String("Changing the DTYP field not supported"),errorMessage);
        }
    }
    dbFldDes *pfldDes = dbAddr.pfldDes;
    if(dbIsValueField(pfldDes)) {
        dbAddr.precord->udf = 0;
    }
    return Status::OK;
}

ScalarType V3Util::getScalarType(Requester &requester, DbAddr &dbAddr)
{
    switch(dbAddr.field_type) {
        case DBF_CHAR:
        case DBF_UCHAR:
            return pvByte;
        case DBF_SHORT:
        case DBF_USHORT:
            return pvShort;
        case DBF_LONG:
        case DBF_ULONG:
            return pvInt;
        case DBF_FLOAT:
            return pvFloat;
        case DBF_DOUBLE:
            return pvDouble;
        case DBF_STRING:
            return pvString;
        default:
            break;
    }
    // Note that pvBoolean is not a supported type
    return pvBoolean;
}

}}

