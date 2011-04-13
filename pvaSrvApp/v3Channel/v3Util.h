/* v3Util.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#ifndef V3UTIL_H
#define V3UTIL_H
#include <string>
#include <cstring>

#include <dbAccess.h>
#include <dbCommon.h>
#include <recSup.h>
#include <dbBase.h>

#include <status.h>
#include <bitSet.h>
#include <requester.h>
#include <pvIntrospect.h>
#include <pvData.h>

#include "v3Channel.h"

namespace epics { namespace pvIOC { 

extern "C" {
typedef long (*get_array_info) (DBADDR *,long *,long *);
typedef long (*put_array_info) (void *,long);
typedef long (*get_enum_strs) (DBADDR *, struct dbr_enumStrs  *);
typedef long (*get_units) (DBADDR *, char *);
typedef long (*get_precision) (DBADDR *, long  *);
typedef long (*get_graphic_double) (DBADDR *, struct dbr_grDouble  *);
typedef long (*get_control_double) (DBADDR *, struct dbr_ctrlDouble  *);
}

class V3Util : private epics::pvData::NoDefaultMethods {
public:
    static int processBit;       // is processing requested
    static int shareArrayBit;    // share V3array instead of copy
    static int scalarValueBit;   // value is a scalar
    static int arrayValueBit;    // value is an array
    static int enumValueBit;     // value is an enum
    static int timeStampBit;     // get timeStamp;
    static int alarmBit;         // get alarm
    static int displayBit;       // get display info
    static int controlBit;       // get control info
    // MUST ALSO IMPLEMENT alarmLimit
    static int noAccessBit;      // fields can not be accessed
    static int noModBit;         // fields can not be modified
    static int dbPutBit;         // Must call dbPutField
    static int isLinkBit;        // field is a DBF_XXLINK field

    static epics::pvData::String recordString;
    static epics::pvData::String processString;
    static epics::pvData::String queueSizeString;
    static epics::pvData::String recordShareString;
    static epics::pvData::String fieldString;
    static epics::pvData::String fieldListString;
    static epics::pvData::String valueString;
    static epics::pvData::String valueShareArrayString;
    static epics::pvData::String timeStampString;
    static epics::pvData::String alarmString;
    static epics::pvData::String displayString;
    static epics::pvData::String controlString;
    static epics::pvData::String allString;
    static epics::pvData::String indexString;



    static int getProperties(epics::pvData::Requester &requester,
        epics::pvData::PVStructure &pvRequest,DbAddr &dbAddr);
    static epics::pvData::PVStructure *createPVStructure(
        epics::pvData::Requester &requester,
        int mask,DbAddr &dbAddr);
    static void getPropertyData(
        epics::pvData::Requester &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVStructure &pvStructure);
    static epics::pvData::Status get(
        epics::pvData::Requester &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVStructure &pvStructure,
        epics::pvData::BitSet &bitSet,
        CAV3Data *caV3Data);
    static epics::pvData::Status put(
        epics::pvData::Requester &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVField *pvField);
    static epics::pvData::Status putField(
        epics::pvData::Requester &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVField *pvField);
    static epics::pvData::ScalarType getScalarType(
        epics::pvData::Requester &requester,
        DbAddr &dbAddr);
};

}}

#endif  /* V3UTIL_H */
