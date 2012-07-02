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

#include <pv/status.h>
#include <pv/bitSet.h>
#include <pv/requester.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>

#include <pv/v3Channel.h>

namespace epics { namespace pvIOC { 

extern "C" {
typedef long (*get_array_info) (DBADDR *,long *,long *);
typedef long (*put_array_info) (void *,long);
typedef long (*get_enum_strs) (DBADDR *, struct dbr_enumStrs  *);
typedef long (*get_units) (DBADDR *, char *);
typedef long (*get_precision) (DBADDR *, long  *);
typedef long (*get_graphic_double) (DBADDR *, struct dbr_grDouble  *);
typedef long (*get_control_double) (DBADDR *, struct dbr_ctrlDouble  *);
typedef long (*get_alarm_double) (DBADDR *, struct dbr_alDouble  *);
}

class V3Util : private epics::pvData::NoDefaultMethods {
public:

    // client request bits
    static int processBit;       // is processing requested
    static int shareArrayBit;    // share V3array instead of copy
    static int timeStampBit;     // get timeStamp;
    static int alarmBit;         // get alarm
    static int displayBit;       // get display info
    static int controlBit;       // get control info
    static int valueAlarmBit;    // get value alarm info
    // V3 data characteristics
    static int scalarValueBit;   // value is a scalar
    static int arrayValueBit;    // value is an array
    static int enumValueBit;     // value is an enum
    static int noAccessBit;      // fields can not be accessed
    static int noModBit;         // fields can not be modified
    static int dbPutBit;         // Must call dbPutField
    static int isLinkBit;        // field is a DBF_XXLINK field

    static int getProperties(
        epics::pvData::Requester::shared_pointer const &requester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest,
        DbAddr &dbAddr);
    static epics::pvData::PVStructurePtr createPVStructure(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr);
    static void getPropertyData(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVStructurePtr const &pvStructure);
    static epics::pvData::Status get(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVStructurePtr const &pvStructure,
        epics::pvData::BitSet::shared_pointer const &bitSet,
        CAV3Data *caV3Data);
    static epics::pvData::Status put(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVFieldPtr const &pvField);
    static epics::pvData::Status putField(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVFieldPtr const &pvField);
    static epics::pvData::ScalarType getScalarType(
        epics::pvData::Requester::shared_pointer const &requester,
        DbAddr &dbAddr);
};

}}

#endif  /* V3UTIL_H */
