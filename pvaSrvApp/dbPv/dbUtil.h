/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

#ifndef DBUTIL_H
#define DBUTIL_H

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

#include "dbPv.h"

namespace epics { namespace pvaSrv { 

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

class DbUtil;
typedef std::tr1::shared_ptr<DbUtil> DbUtilPtr;

class DbUtil {
public:
    POINTER_DEFINITIONS(DbUtil);
    virtual ~DbUtil() {}
    static DbUtilPtr getDbUtil();
    // client request bits
    int processBit;       // is processing requested
    int timeStampBit;     // get timeStamp;
    int alarmBit;         // get alarm
    int displayBit;       // get display info
    int controlBit;       // get control info
    int valueAlarmBit;    // get value alarm info
    // DB data characteristics
    int getValueBit;      // client requested value
    int scalarValueBit;   // value is a scalar
    int arrayValueBit;    // value is an array
    int enumValueBit;     // value is an enum
    int enumIndexBit;     // pvRequest specified value.index
    int noAccessBit;      // fields can not be accessed
    int noModBit;         // fields can not be modified
    int dbPutBit;         // Must call dbPutField
    int isLinkBit;        // field is a DBF_XXLINK field

    int getProperties(
        epics::pvData::Requester::shared_pointer const &requester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest,
        DbAddr &dbAddr,
        bool processDefault);
    epics::pvData::PVStructurePtr createPVStructure(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr);
    void getPropertyData(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVStructurePtr const &pvStructure);
    epics::pvData::Status get(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVStructurePtr const &pvStructure,
        epics::pvData::BitSet::shared_pointer const &bitSet,
        CaData *caV3Data);
    epics::pvData::Status put(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVFieldPtr const &pvField);
    epics::pvData::Status putField(
        epics::pvData::Requester::shared_pointer const &requester,
        int mask,DbAddr &dbAddr,
        epics::pvData::PVFieldPtr const &pvField);
    epics::pvData::ScalarType getScalarType(
        epics::pvData::Requester::shared_pointer const &requester,
        DbAddr &dbAddr);
private:
    DbUtil();
    epics::pvData::PVStructurePtr  nullPVStructure;
    epics::pvData::String recordString;
    epics::pvData::String processString;
    epics::pvData::String queueSizeString;
    epics::pvData::String fieldString;
    epics::pvData::String valueString;
    epics::pvData::String valueIndexString;
    epics::pvData::String timeStampString;
    epics::pvData::String alarmString;
    epics::pvData::String displayString;
    epics::pvData::String controlString;
    epics::pvData::String valueAlarmString;
    epics::pvData::String lowAlarmLimitString;
    epics::pvData::String lowWarningLimitString;
    epics::pvData::String highWarningLimitString;
    epics::pvData::String highAlarmLimitString;
    epics::pvData::String allString;
    epics::pvData::String indexString;
};

}}

#endif  /* DBUTIL_H */
