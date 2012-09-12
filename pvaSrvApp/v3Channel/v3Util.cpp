/* v3Util.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#include <string>
#include <cstring>
#include <stdexcept>

#include <dbAccess.h>
#include <dbEvent.h>
#include <dbNotify.h>
#include <special.h>
#include <link.h>
#include <alarm.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/convert.h>
#include <pv/pvEnumerated.h>
#include <pv/pvAccess.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/control.h>
#include <pv/display.h>
#include <pv/timeStamp.h>
#include <pv/pvAlarm.h>
#include <pv/pvTimeStamp.h>
#include <pv/pvControl.h>
#include <pv/pvDisplay.h>

#include "v3Util.h"
#include "v3Array.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using std::tr1::static_pointer_cast;

// client request bits
int V3Util::processBit       = 0x0001;
int V3Util::shareArrayBit    = 0x0002;
int V3Util::timeStampBit     = 0x0004;
int V3Util::alarmBit         = 0x0008;
int V3Util::displayBit       = 0x0010;
int V3Util::controlBit       = 0x0020;
int V3Util::valueAlarmBit    = 0x0040;
// V3 data characteristics
int V3Util::scalarValueBit   = 0x0080;
int V3Util::arrayValueBit    = 0x0100;
int V3Util::enumValueBit     = 0x0200;
int V3Util::noAccessBit      = 0x0400;
int V3Util::noModBit         = 0x0800;
int V3Util::dbPutBit         = 0x1000;
int V3Util::isLinkBit        = 0x2000;

static String recordString("record");
static String processString("record._options.process");
static String queueSizeString("record._options.queueSize");
static String recordShareString("record._options.shareData");
static String fieldString("field");
static String valueString("value");
static String valueShareArrayString("value._options.shareData");
static String timeStampString("timeStamp");
static String alarmString("alarm");
static String displayString("display");
static String controlString("control");
static String valueAlarmString("valueAlarm");
static String lowAlarmLimitString("lowAlarmLimit");
static String lowWarningLimitString("lowWarningLimit");
static String highWarningLimitString("highWarningLimit");
static String highAlarmLimitString("highAlarmLimit");
static String allString("value,timeStamp,alarm,display,control,valueAlarm");
static String indexString("index");

static PVStructurePtr  nullPVStructure;


int V3Util::getProperties(
    Requester::shared_pointer const &requester,PVStructure::shared_pointer const &pvr,DbAddr &dbAddr)
{
    PVStructure *pvRequest = pvr.get();
    int propertyMask = 0;
    PVFieldPtr pvField = pvRequest->getSubField(processString);
    if(pvField.get()!=NULL) {
        PVStringPtr pvString = pvRequest->getStringField(processString);
        if(pvString.get()!=NULL) {
            String value = pvString->get();
            if(value.compare("true")==0) propertyMask |= processBit;
        }
    }
    PVStructurePtr fieldPV;
    PVFieldPtr pvTemp = pvRequest->getSubField(fieldString);
    if(pvTemp.get()!=NULL) fieldPV = static_pointer_cast<PVStructure>(pvTemp);
    PVStringPtr pvShareString;
    pvField = pvRequest->getSubField(recordShareString);
    if(pvField.get()!=NULL) {
         pvShareString = pvRequest->getStringField(recordShareString);
    } else {
         pvField = fieldPV->getSubField(valueShareArrayString);
         if(pvField.get()!=NULL) {
             pvShareString = fieldPV->getStringField(valueShareArrayString);
         }
    }
    if(pvShareString.get()!=NULL) {
        String value = pvShareString->get();
        if(value.compare("true")==0) propertyMask |= shareArrayBit;
    }
    bool getValue;
    if(fieldPV.get()!=NULL) pvRequest = fieldPV.get();
    String fieldList;
    if(pvRequest->getStructure()->getNumberFields()==0) {
        getValue = true;
        fieldList = allString;
    } else {
        pvField = pvRequest->getSubField(valueString);
        if(pvField.get()!=NULL) {
            if(fieldList.size()>0) fieldList += ',';
            fieldList += valueString;
            getValue = true;
        }
        pvField = pvRequest->getSubField(alarmString);
        if(pvField.get()!=NULL) {
            if(fieldList.size()>0) fieldList += ',';
            fieldList += alarmString;
        }
        pvField = pvRequest->getSubField(timeStampString);
        if(pvField.get()!=NULL) {
            if(fieldList.size()>0) fieldList += ',';
            fieldList += timeStampString;
        }
        pvField = pvRequest->getSubField(displayString);
        if(pvField.get()!=NULL) {
            if(fieldList.size()>0) fieldList += ',';
            fieldList += displayString;
        }
        pvField = pvRequest->getSubField(controlString);
        if(pvField.get()!=NULL) {
            if(fieldList.size()>0) fieldList += ',';
            fieldList += controlString;
        }
        pvField = pvRequest->getSubField(valueAlarmString);
        if(pvField.get()!=NULL) {
            if(fieldList.size()>0) fieldList += ',';
            fieldList += valueAlarmString;
        }
    }
    if(fieldList.length()>0) {
        if(fieldList.find(valueString)!=String::npos) getValue = true;
    }
    if(getValue) {
        Type type = (dbAddr.special==SPC_DBADDR) ? scalarArray : scalar;
        ScalarType scalarType(pvBoolean);
        // Note that pvBoolean is not a supported type
        switch(dbAddr.field_type) {
        case DBF_STRING:
            scalarType = pvString; break;
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
        case DBF_ENUM:
            propertyMask |= enumValueBit; break;
        case DBF_MENU:
            propertyMask |= (enumValueBit|dbPutBit); break;
        case DBF_DEVICE:
            propertyMask |= enumValueBit; break;
        case DBF_INLINK:
        case DBF_OUTLINK:
        case DBF_FWDLINK:
            scalarType = pvString;
            propertyMask |= (isLinkBit|dbPutBit); break;
        case DBF_NOACCESS:
            requester->message(String("access is not allowed"),errorMessage);
            propertyMask = noAccessBit; return propertyMask;
        default:
            requester->message(String(
                "logic error unknown DBF type"),errorMessage);
            propertyMask = noAccessBit; return propertyMask;
        }
        if(type==scalar&&scalarType!=pvBoolean) {
           propertyMask |= scalarValueBit;
        }
        if(type==scalarArray&&scalarType!=pvBoolean) {
           propertyMask |= arrayValueBit;
        }
    }
    if(dbAddr.special!=0) {
        switch(dbAddr.special) {
        case SPC_NOMOD:
            propertyMask |= noModBit; break;
        case SPC_DBADDR: // already used
            break;
        case SPC_SCAN:
        case SPC_ALARMACK:
        case SPC_AS:
        case SPC_ATTRIBUTE:
        case SPC_MOD:
        case SPC_RESET:
        case SPC_LINCONV:
        case SPC_CALC:
            propertyMask |= dbPutBit; break;
        default:
            requester->message(String(
                "logic error unknown special type"),errorMessage);
            propertyMask = noAccessBit; return propertyMask;
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
        if(fieldList.find(valueAlarmString)!=String::npos) {
            if(dbAddr.field_type==DBF_LONG||dbAddr.field_type==DBF_DOUBLE) {
                propertyMask |= valueAlarmBit;
            }
        }
    } else {
        pvField = pvRequest->getSubField(timeStampString);
        if(pvField.get()!=NULL) {
             PVStringPtr pvString = pvRequest->getStringField(timeStampString);
             if(pvString.get()!=NULL) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= timeStampBit;
                  }
             }
        }
        pvField = pvRequest->getSubField(alarmString);
        if(pvField.get()!=NULL) {
             PVStringPtr pvString = pvRequest->getStringField(alarmString);
             if(pvString.get()!=NULL) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= alarmBit;
                  }
             }
        }
        pvField = pvRequest->getSubField(displayString);
        if(pvField.get()!=NULL) {
             PVStringPtr pvString = pvRequest->getStringField(displayString);
             if(pvString.get()!=NULL) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= displayBit;
                  }
             }
        }
        pvField = pvRequest->getSubField(controlString);
        if(pvField.get()!=NULL) {
             PVStringPtr pvString = pvRequest->getStringField(controlString);
             if(pvString.get()!=NULL) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= controlBit;
                  }
             }
        }
        pvField = pvRequest->getSubField(valueAlarmString);
        if(pvField.get()!=NULL) {
             PVStringPtr pvString = pvRequest->getStringField(valueAlarmString);
             if(pvString.get()!=NULL) {
                  if(pvString->get().compare("true")==0) {
                      propertyMask |= valueAlarmBit;
                  }
             }
        }
    }
    return propertyMask;
}

PVStructurePtr V3Util::createPVStructure(
    Requester::shared_pointer const &requester,int propertyMask,DbAddr &dbAddr)
{
    StandardPVFieldPtr standardPVField = getStandardPVField();
    StandardFieldPtr standardField = getStandardField();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    FieldCreatePtr fieldCreate = getFieldCreate();
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
    if((propertyMask&valueAlarmBit)!=0) {
        if(!properties.empty()) properties += ",";
        properties += valueAlarmString;
    }
    if((propertyMask&enumValueBit)!=0) {
        struct dbr_enumStrs enumStrs;
        struct rset *prset = dbGetRset(&dbAddr);
        if(prset && prset->get_enum_strs) {
            get_enum_strs get_strs;
            get_strs = (get_enum_strs)(prset->get_enum_strs);
            get_strs(&dbAddr,&enumStrs);
            size_t length = enumStrs.no_str;
            StringArray choices;
            choices.reserve(length);
            for(size_t i=0; i<length; i++) {
                 choices.push_back(enumStrs.strs[i]);
            }
            PVStructurePtr pvStructure = standardPVField->enumerated(
                 choices,properties);
            return pvStructure;
        } else if(dbAddr.field_type==DBF_DEVICE) {
            dbFldDes *pdbFldDes = dbAddr.pfldDes;
            dbDeviceMenu *pdbDeviceMenu
                = static_cast<dbDeviceMenu *>(pdbFldDes->ftPvt);
            size_t length = pdbDeviceMenu->nChoice;
            char **papChoice = pdbDeviceMenu->papChoice;
            StringArray choices;
            choices.reserve(length);
            for(size_t i=0; i<length; i++) {
                 choices.push_back(papChoice[i]);
            }
            PVStructurePtr pvStructure = standardPVField->enumerated(
                choices,properties);
            return pvStructure;
        } else if(dbAddr.field_type==DBF_MENU) {
            dbFldDes *pdbFldDes = dbAddr.pfldDes;
            dbMenu *pdbMenu = static_cast<dbMenu *>(pdbFldDes->ftPvt);
            size_t length = pdbMenu->nChoice;
            char **papChoice = pdbMenu->papChoiceValue;
            StringArray choices;
            choices.reserve(length);
            for(size_t i=0; i<length; i++) {
                 choices.push_back(papChoice[i]);
            }
            PVStructurePtr pvStructure = standardPVField->enumerated(
                choices,properties);
            return pvStructure;
        } else {
            requester->message(
               String("bad enum field in V3 record"),errorMessage);
            return nullPVStructure;
        }
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
             if(propertyMask&isLinkBit) {
                  scalarType = pvString; break;
             } else {
                throw std::logic_error("Should never get here");
             }
    }
    if((propertyMask&scalarValueBit)!=0) {
        return standardPVField->scalar(scalarType,properties);
    }
    if((propertyMask&arrayValueBit)==0) {
        requester->message("did not ask for value",errorMessage);
        return nullPVStructure;
    }
    // the value is an array. Must use implementation that "wraps" V3 array 
    bool share = (propertyMask&shareArrayBit) ? true : false;
    PVFieldPtr pvField;
    ScalarArrayConstPtr scalarArray = getFieldCreate()->createScalarArray(
         scalarType);
    V3ValueArrayCreatePtr v3ValueArrayCreate = getV3ValueArrayCreate();
    switch(scalarType) {
    case pvByte:
       pvField = v3ValueArrayCreate->createByteArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvUByte:
       pvField = v3ValueArrayCreate->createUByteArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvShort:
       pvField = v3ValueArrayCreate->createShortArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvUShort:
       pvField = v3ValueArrayCreate->createUShortArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvInt:
       pvField = v3ValueArrayCreate->createIntArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvUInt:
       pvField = v3ValueArrayCreate->createUIntArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvFloat:
       pvField = v3ValueArrayCreate->createFloatArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvDouble:
       pvField = v3ValueArrayCreate->createDoubleArray(
           nullPVStructure,scalarArray,dbAddr,share);
       break;
    case pvString:
       pvField = v3ValueArrayCreate->createStringArray(
           nullPVStructure,scalarArray,dbAddr);
       break;
    default:
       throw std::logic_error(String("Should never get here"));
    }
    int numberFields = 1;
    if((propertyMask&timeStampBit)!=0) numberFields++;
    if((propertyMask&alarmBit)!=0) numberFields++;
    if((propertyMask&displayBit)!=0) numberFields++;
    if((propertyMask&controlBit)!=0) numberFields++;
    if((propertyMask&valueAlarmBit)!=0) numberFields++;
    StringArray fieldNames;
    FieldConstPtrArray fields;
    fieldNames.reserve(numberFields);
    fields.reserve(numberFields);
    fieldNames.push_back("value");
    fields.push_back(pvField->getField());
    if((propertyMask&timeStampBit)!=0) {
        fieldNames.push_back("timeStamp");
        fields.push_back(standardField->timeStamp());
    }
    if((propertyMask&alarmBit)!=0) {
        fieldNames.push_back("alarm");
        fields.push_back(standardField->alarm());
    }
    if((propertyMask&displayBit)!=0) {
        fieldNames.push_back("display");
        fields.push_back(standardField->display());
    }
    if((propertyMask&controlBit)!=0) {
        fieldNames.push_back("control");
        fields.push_back(standardField->control());
    }
    if((propertyMask&valueAlarmBit)!=0) {
       fieldNames.push_back("valueAlarm");
       switch(scalarType) {
       case pvByte:
          fields.push_back(standardField->byteAlarm());
          break;
       case pvUByte:
          fields.push_back(standardField->ubyteAlarm());
          break;
       case pvShort:
          fields.push_back(standardField->shortAlarm());
          break;
       case pvUShort:
          fields.push_back(standardField->ushortAlarm());
          break;
       case pvInt:
          fields.push_back(standardField->intAlarm());
          break;
       case pvUInt:
          fields.push_back(standardField->uintAlarm());
          break;
       case pvFloat:
          fields.push_back(standardField->floatAlarm());
          break;
       case pvDouble:
          fields.push_back(standardField->doubleAlarm());
          break;
       default:
          throw std::logic_error(String("Should never get here"));
       }
    }
    StructureConstPtr structure = fieldCreate->createStructure(fieldNames,fields);
    PVStructurePtr pvParent = getPVDataCreate()->createPVStructure(structure);
    // TODO a hack
    const_cast<PVFieldPtrArray&>(pvParent->getPVFields())[0] = pvField;
    return pvParent;
}

void  V3Util::getPropertyData(
        Requester::shared_pointer const &requester,
        int propertyMask,DbAddr &dbAddr,
        PVStructurePtr const &pvStructure)
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
        PVFieldPtr pvField = pvStructure->getSubField(displayString);
        pvDisplay.attach(pvField);
        pvDisplay.set(display);
    }
    if(propertyMask&controlBit) {
        Control control;
        struct rset *prset = dbGetRset(&dbAddr);
        struct dbr_ctrlDouble graphics;
        memset(&graphics,0,sizeof(graphics));
        if(prset && prset->get_control_double) {
           get_control_double cc =
                (get_control_double)(prset->get_control_double);
           cc(&dbAddr,&graphics);
           control.setHigh(graphics.upper_ctrl_limit);
           control.setLow(graphics.lower_ctrl_limit);
        }
        PVControl pvControl;
        PVFieldPtr pvField = pvStructure->getSubField(controlString);
        pvControl.attach(pvField);
        pvControl.set(control);
    }
    if(propertyMask&valueAlarmBit) {
        struct rset *prset = dbGetRset(&dbAddr);
        struct dbr_alDouble ald;
        memset(&ald,0,sizeof(ald));
        if(prset && prset->get_alarm_double) {
           get_alarm_double cc =
               (get_alarm_double)(prset->get_alarm_double);
           cc(&dbAddr,&ald);
        }
        PVStructurePtr pvAlarmLimits =
            pvStructure->getStructureField(valueAlarmString);
        PVFieldPtr pvf = pvAlarmLimits->getSubField(lowAlarmLimitString);
        if(pvf.get()!=NULL && pvf->getField()->getType()==scalar) {
            PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvf);
            getConvert()->fromDouble(pvScalar,ald.lower_alarm_limit);
        }
        pvf = pvAlarmLimits->getSubField(lowWarningLimitString);
        if(pvf.get()!=NULL && pvf->getField()->getType()==scalar) {
            PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvf);
            getConvert()->fromDouble(pvScalar,ald.lower_warning_limit);
        }
        pvf = pvAlarmLimits->getSubField(highWarningLimitString);
        if(pvf.get()!=NULL && pvf->getField()->getType()==scalar) {
            PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvf);
            getConvert()->fromDouble(pvScalar,ald.upper_warning_limit);
        }
        pvf = pvAlarmLimits->getSubField(highAlarmLimitString);
        if(pvf.get()!=NULL && pvf->getField()->getType()==scalar) {
            PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvf);
            getConvert()->fromDouble(pvScalar,ald.upper_alarm_limit);
        }
    }
}

Status  V3Util::get(
        Requester::shared_pointer const &requester,
        int propertyMask,DbAddr &dbAddr,
        PVStructurePtr const &pvStructure,
        BitSet::shared_pointer const &bitSet,
        CAV3Data *caV3Data)
{
    PVFieldPtrArray pvFields = pvStructure->getPVFields();
    PVFieldPtr pvField = pvFields[0];
    if((propertyMask&scalarValueBit)!=0) {
        PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvField);
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
            PVBytePtr pv = static_pointer_cast<PVByte>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvUByte: {
            uint8 val = 0;
            if(caV3Data) {
                val = caV3Data->ubyteValue;
            } else {
                val = *static_cast<uint8 *>(dbAddr.pfield);
            }
            PVUBytePtr pv = static_pointer_cast<PVUByte>(pvField);
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
            PVShortPtr pv = static_pointer_cast<PVShort>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvUShort: {
            uint16 val = 0;
            if(caV3Data) {
                val = caV3Data->ushortValue;
            } else {
                val = *static_cast<uint16 *>(dbAddr.pfield);
            }
            PVUShortPtr pv = static_pointer_cast<PVUShort>(pvField);
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
            PVIntPtr pv = static_pointer_cast<PVInt>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvUInt: {
            uint32 val = 0;
            if(caV3Data) {
                val = caV3Data->uintValue;
            } else {
                val = *static_cast<uint32 *>(dbAddr.pfield);
            }
            PVUIntPtr pv = static_pointer_cast<PVUInt>(pvField);
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
            PVFloatPtr pv = static_pointer_cast<PVFloat>(pvField);
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
            PVDoublePtr pv = static_pointer_cast<PVDouble>(pvField);
            if(pv->get()!=val) {
                pv->put(val);
                wasChanged = true;
            }
            break;
        }
        case pvString: {
            char * val = 0;
            if(propertyMask&isLinkBit) {
                char buffer[100];
                for(int i=0; i<100; i++) buffer[i]  = 0;
                long result = dbGetField(&dbAddr,DBR_STRING,
                    buffer,0,0,0);
                if(result!=0) {
                    requester->message(String("dbGetField error"),errorMessage);
                }
                val = buffer;
            } else {
                val = static_cast<char *>(dbAddr.pfield);
            }
            String sval(val);
            PVStringPtr pvString = static_pointer_cast<PVString>(pvField);
            if((pvString->get().compare(sval))!=0) {
                pvString->put(sval);
                wasChanged = true;
            }
            break;
        }
        default:
             throw std::logic_error(String("Should never get here"));
        }
        if(wasChanged) bitSet->set(pvField->getFieldOffset());
    } else if((propertyMask&arrayValueBit)!=0) {
        PVScalarArrayPtr pvArray = static_pointer_cast<PVScalarArray>(pvField);
        ScalarType scalarType = pvArray->getScalarArray()->getElementType();
        switch(scalarType) {
        case pvByte: {
            PVByteArrayPtr pva = static_pointer_cast<PVByteArray>(pvArray);
            ByteArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvUByte: {
            PVUByteArrayPtr pva = static_pointer_cast<PVUByteArray>(pvArray);
            UByteArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvShort: {
            PVShortArrayPtr pva = static_pointer_cast<PVShortArray>(pvArray);
            ShortArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvUShort: {
            PVUShortArrayPtr pva = static_pointer_cast<PVUShortArray>(pvArray);
            UShortArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvInt: {
            PVIntArrayPtr pva = static_pointer_cast<PVIntArray>(pvArray);
            IntArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvUInt: {
            PVUIntArrayPtr pva = static_pointer_cast<PVUIntArray>(pvArray);
            UIntArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvFloat: {
            PVFloatArrayPtr pva = static_pointer_cast<PVFloatArray>(pvArray);
            FloatArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvDouble: {
            PVDoubleArrayPtr pva = static_pointer_cast<PVDoubleArray>(pvArray);
            DoubleArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        case pvString: {
            PVStringArrayPtr pva = static_pointer_cast<PVStringArray>(pvArray);
            StringArrayData data;
            int length = pva->getLength();
            pva->get(0,length,data);
            break;
        }
        default:
             throw std::logic_error(String("Should never get here"));
        }
        bitSet->set(pvField->getFieldOffset());
    } else if((propertyMask&enumValueBit)!=0) {
        PVStructurePtr pvEnum = static_pointer_cast<PVStructure>(pvField);
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
        PVIntPtr pvIndex = pvEnum->getIntField(indexString);
        if(pvIndex->get()!=val) {
            pvIndex->put(val);
            bitSet->set(pvIndex->getFieldOffset());
        }
    }
    if((propertyMask&timeStampBit)!=0) {
        TimeStamp timeStamp;
        PVTimeStamp pvTimeStamp;
        PVFieldPtr pvField = pvStructure->getSubField(timeStampString);
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
            bitSet->set(pvField->getFieldOffset());
        }
    }
     if((propertyMask&alarmBit)!=0) {
        Alarm alarm;
        PVAlarm pvAlarm;
        PVFieldPtr pvField = pvStructure->getSubField(alarmString);
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
            bitSet->set(pvField->getFieldOffset());
        }
    }
    return Status::Ok;
}

Status  V3Util::put(
        Requester::shared_pointer const &requester,
        int propertyMask,DbAddr &dbAddr,
        PVFieldPtr const &pvField)
{
    if((propertyMask&scalarValueBit)!=0) {
        PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvField);
        ScalarType scalarType = pvScalar->getScalar()->getScalarType();
        switch(scalarType) {
        case pvByte: {
            int8 * val = static_cast<int8 *>(dbAddr.pfield);
            PVBytePtr pv = static_pointer_cast<PVByte>(pvField);
            *val = pv->get();
            break;
        }
        case pvUByte: {
            uint8 * val = static_cast<uint8 *>(dbAddr.pfield);
            PVUBytePtr pv = static_pointer_cast<PVUByte>(pvField);
            *val = pv->get();
            break;
        }
        case pvShort: {
            int16 * val = static_cast<int16 *>(dbAddr.pfield);
            PVShortPtr pv = static_pointer_cast<PVShort>(pvField);
            *val = pv->get();
            break;
        }
        case pvUShort: {
            uint16 * val = static_cast<uint16 *>(dbAddr.pfield);
            PVUShortPtr pv = static_pointer_cast<PVUShort>(pvField);
            *val = pv->get();
            break;
        }
        case pvInt: {
            int32 * val = static_cast<int32 *>(dbAddr.pfield);
            PVIntPtr pv = static_pointer_cast<PVInt>(pvField);
            *val = pv->get();
            break;
        }
        case pvUInt: {
            uint32 * val = static_cast<uint32 *>(dbAddr.pfield);
            PVUIntPtr pv = static_pointer_cast<PVUInt>(pvField);
            *val = pv->get();
            break;
        }
        case pvFloat: {
            float * val = static_cast<float *>(dbAddr.pfield);
            PVFloatPtr pv = static_pointer_cast<PVFloat>(pvField);
            *val = pv->get();
            break;
        }
        case pvDouble: {
            double * val = static_cast<double *>(dbAddr.pfield);
            PVDoublePtr pv = static_pointer_cast<PVDouble>(pvField);
            *val = pv->get();
            break;
        }
        case pvString: {
            char * to = static_cast<char *>(dbAddr.pfield);
            PVStringPtr pvString = static_pointer_cast<PVString>(pvField);
            int len = dbAddr.field_size;
            strncpy(to,pvString->get().c_str(),len -1);
            *(to + len -1) = 0;
        }
        default:
            requester->message(
                String("Logic Error did not handle scalarType"),errorMessage);
            return Status::Ok;
        }
    } else if((propertyMask&arrayValueBit)!=0) {
        // client or deserialize already handled this.
    } else if((propertyMask&enumValueBit)!=0) {
        PVStructurePtr pvEnum = static_pointer_cast<PVStructure>(pvField);
        PVIntPtr pvIndex = pvEnum->getIntField(indexString);
        if(dbAddr.field_type==DBF_MENU) {
            requester->message(
                String("Not allowed to change a menu field"),errorMessage);
        } else if(dbAddr.field_type==DBF_ENUM||dbAddr.field_type==DBF_DEVICE) {
            epicsEnum16 *value = static_cast<epicsEnum16*>(dbAddr.pfield);
            *value = pvIndex->get();
        } else {
            requester->message(
                String("Logic Error unknown enum field"),errorMessage);
            return Status::Ok;
        }
    } else {
        requester->message(
            String("Logic Error unknown field to put"),errorMessage);
            return Status::Ok;
    }
    dbCommon *precord = dbAddr.precord;
    dbFldDes *pfldDes = dbAddr.pfldDes;
    int isValueField = dbIsValueField(pfldDes);
    if(isValueField) precord->udf = 0;
    bool post = false;
    if(!(propertyMask&processBit)) post = true;
    if(precord->mlis.count && !(isValueField && pfldDes->process_passive)) post = true;
    if(post) {
        db_post_events(precord, dbAddr.pfield, DBE_VALUE | DBE_LOG);
    }
    return Status::Ok;
}

Status  V3Util::putField(
        Requester::shared_pointer const &requester,
        int propertyMask,DbAddr &dbAddr,
        PVFieldPtr const &pvField)
{
    const void *pbuffer = 0;
    short dbrType = 0;
    int8 bvalue;
    uint8 ubvalue;
    int16 svalue;
    uint16 usvalue;
    int32 ivalue;
    uint32 uivalue;
    float fvalue;
    double dvalue;
    String string;
    if((propertyMask&scalarValueBit)!=0) {
        PVScalarPtr pvScalar = static_pointer_cast<PVScalar>(pvField);
        ScalarType scalarType = pvScalar->getScalar()->getScalarType();
        switch(scalarType) {
        case pvByte: {
            PVBytePtr pv = static_pointer_cast<PVByte>(pvField);
            bvalue = pv->get(); pbuffer = &bvalue;
            dbrType = DBF_CHAR;
            break;
        }
        case pvUByte: {
            PVUBytePtr pv = static_pointer_cast<PVUByte>(pvField);
            ubvalue = pv->get(); pbuffer = &ubvalue;
            dbrType = DBF_UCHAR;
            break;
        }
        case pvShort: {
            PVShortPtr pv = static_pointer_cast<PVShort>(pvField);
            svalue = pv->get(); pbuffer = &svalue;
            dbrType = DBF_SHORT;
            break;
        }
        case pvUShort: {
            PVUShortPtr pv = static_pointer_cast<PVUShort>(pvField);
            usvalue = pv->get(); pbuffer = &usvalue;
            dbrType = DBF_USHORT;
            break;
        }
        case pvInt: {
            PVIntPtr pv = static_pointer_cast<PVInt>(pvField);
            ivalue = pv->get(); pbuffer = &ivalue;
            dbrType = DBF_LONG;
            break;
        }
        case pvUInt: {
            PVUIntPtr pv = static_pointer_cast<PVUInt>(pvField);
            uivalue = pv->get(); pbuffer = &uivalue;
            dbrType = DBF_ULONG;
            break;
        }
        case pvFloat: {
            PVFloatPtr pv = static_pointer_cast<PVFloat>(pvField);
            fvalue = pv->get(); pbuffer = &fvalue;
            dbrType = DBF_FLOAT;
            break;
        }
        case pvDouble: {
            PVDoublePtr pv = static_pointer_cast<PVDouble>(pvField);
            dvalue = pv->get(); pbuffer = &dvalue;
            dbrType = DBF_DOUBLE;
            break;
        }
        case pvString: {
            PVStringPtr pvString = static_pointer_cast<PVString>(pvField);
            string = pvString->get();
            pbuffer = string.c_str();
            dbrType = DBF_STRING;
            break;
        }
        default:
            requester->message(
                String("Logic Error did not handle scalarType"),errorMessage);
            return Status::Ok;
        }
    } else if((propertyMask&enumValueBit)!=0) {
        PVStructurePtr pvEnum = static_pointer_cast<PVStructure>(pvField);
        PVIntPtr pvIndex = pvEnum->getIntField(indexString);
        svalue = pvIndex->get(); pbuffer = &svalue;
        dbrType = DBF_ENUM;
    } else {
        requester->message(
                String("Logic Error unknown field to put"),errorMessage);
        return Status::Ok;
    }
    long status = dbPutField(&dbAddr,dbrType,pbuffer,1);
    if(status!=0) {
        char buf[30];
        sprintf(buf,"dbPutField returned error 0x%lx",status);
        String message(buf);
        requester->message(message,warningMessage);
    }
    return Status::Ok;
}

ScalarType V3Util::getScalarType(Requester::shared_pointer const &requester, DbAddr &dbAddr)
{
    switch(dbAddr.field_type) {
        case DBF_CHAR:
            return pvByte;
        case DBF_UCHAR:
            return pvUByte;
        case DBF_SHORT:
            return pvShort;
        case DBF_USHORT:
            return pvUShort;
        case DBF_LONG:
            return pvInt;
        case DBF_ULONG:
            return pvUInt;
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

