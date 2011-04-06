/* v3CAMonitor.cpp */
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

#include <cadef.h>
#include <db_access.h>
#include <dbDefs.h>
#include <alarmString.h>

#include "v3CAMonitor.h"
#include "CAV3Context.h"

using namespace epics::pvData;

CAV3Data::CAV3Data()
: doubleValue(0.0),timeStamp(),
  sevr(0),status(0)
{}

CAV3Data::~CAV3Data() { }

class CAV3MonitorPvt {
public:
    CAV3MonitorPvt(CAV3MonitorRequester &requester,
        String pvName,V3Type v3Type);
    ~CAV3MonitorPvt();
    CAV3Data & getData();
    void connect();
    void start();
    void stop();

    CAV3MonitorRequester &requester;
    String pvName;
    V3Type v3Type;
    CAV3Data data;
    chanId chid;
    evid myevid;
    CAV3Context &context;
};

extern "C" {

static void connectionCallback(struct connection_handler_args args)
{
    CAV3MonitorPvt *pvt = static_cast<CAV3MonitorPvt *>(ca_puser(args.chid));   
    pvt->requester.connectionCallback();
}

static void accessRightsCallback(struct access_rights_handler_args args)
{
    CAV3MonitorPvt *pvt = static_cast<CAV3MonitorPvt *>(ca_puser(args.chid));   
    pvt->requester.accessRightsCallback();
}

static void eventCallback(struct event_handler_args eha)
{
    CAV3MonitorPvt *pvt = static_cast<CAV3MonitorPvt *>(ca_puser(eha.chid));   
    if(eha.status!=ECA_NORMAL) {
        pvt->requester.eventCallback(eha.status);
        return;
    }
    switch(pvt->v3Type) {
        case v3Enum: {
            const struct dbr_time_enum *from =
                static_cast<const struct dbr_time_enum*>(eha.dbr);
            pvt->data.sevr = from->severity;
            pvt->data.stat = from->status;
            pvt->data.status = epicsAlarmConditionStrings[from->status];
            pvt->data.intValue = from->value;
            pvt->data.timeStamp = from->stamp;
            break;
        }
        case v3Byte: {
            const struct dbr_time_char *from =
                static_cast<const struct dbr_time_char*>(eha.dbr);
            pvt->data.sevr = from->severity;
            pvt->data.stat = from->status;
            pvt->data.status = epicsAlarmConditionStrings[from->status];
            pvt->data.byteValue = from->value;
            pvt->data.timeStamp = from->stamp;
            break;
        }
        case v3Short: {
            const struct dbr_time_short *from =
                static_cast<const struct dbr_time_short*>(eha.dbr);
            pvt->data.sevr = from->severity;
            pvt->data.stat = from->status;
            pvt->data.status = epicsAlarmConditionStrings[from->status];
            pvt->data.shortValue = from->value;
            pvt->data.timeStamp = from->stamp;
            break;
        }
        case v3Int: {
            const struct dbr_time_long *from =
                static_cast<const struct dbr_time_long*>(eha.dbr);
            pvt->data.sevr = from->severity;
            pvt->data.stat = from->status;
            pvt->data.status = epicsAlarmConditionStrings[from->status];
            pvt->data.intValue = from->value;
            pvt->data.timeStamp = from->stamp;
            break;
        }
        case v3Float: {
            const struct dbr_time_float *from =
                static_cast<const struct dbr_time_float*>(eha.dbr);
            pvt->data.sevr = from->severity;
            pvt->data.stat = from->status;
            pvt->data.status = epicsAlarmConditionStrings[from->status];
            pvt->data.floatValue = from->value;
            pvt->data.timeStamp = from->stamp;
            break;
        }
        case v3Double: {
            const struct dbr_time_double *from =
                static_cast<const struct dbr_time_double*>(eha.dbr);
            pvt->data.sevr = from->severity;
            pvt->data.stat = from->status;
            pvt->data.status = epicsAlarmConditionStrings[from->status];
            pvt->data.doubleValue = from->value;
            pvt->data.timeStamp = from->stamp;
            break;
        }
        case v3String: {
            const struct dbr_time_string *from =
                static_cast<const struct dbr_time_string*>(eha.dbr);
            pvt->data.sevr = from->severity;
            pvt->data.stat = from->status;
            pvt->data.status = epicsAlarmConditionStrings[from->status];
            pvt->data.timeStamp = from->stamp;
            // client will get value from record
            break;
        }
    }
   pvt->requester.eventCallback(0);
}

} //extern "C"


CAV3MonitorPvt::CAV3MonitorPvt(
    CAV3MonitorRequester &requester,
    String pvName,V3Type v3Type)
: requester(requester),pvName(pvName),v3Type(v3Type),
  data(),chid(0),myevid(0),context(CAV3ContextCreate::get(requester))
{ }

CAV3MonitorPvt::~CAV3MonitorPvt()
{
    if(chid!=0) {
        context.checkContext();
        ca_clear_channel(chid);
        chid = 0;
    }
    context.release();
}

void CAV3MonitorPvt::connect()
{
    int status = 0;
    context.checkContext();
    status = ca_create_channel(
        pvName.c_str(),connectionCallback,this,20,&chid);
    if(status!=ECA_NORMAL) {
        requester.message(String(
            "ca_create_channel failed"),errorMessage);
        if(chid!=0) ca_clear_channel(chid);
        return;
    }
    status = ca_replace_access_rights_event(chid,accessRightsCallback);
    if(status!=ECA_NORMAL) {
        requester.message(String(
            "ca_replace_access_rights_event failed"),warningMessage);
    }
}

void CAV3MonitorPvt::start()
{
    chtype type = DBR_STRING;
    switch(v3Type) {
        case v3Enum: type = DBR_TIME_ENUM; break;
        case v3Byte: type = DBR_TIME_CHAR; break;
        case v3Short: type = DBR_TIME_SHORT; break;
        case v3Int: type = DBR_TIME_LONG; break;
        case v3Float: type = DBR_TIME_FLOAT; break;
        case v3Double: type = DBR_TIME_DOUBLE; break;
        case v3String: type = DBR_TIME_STRING; break;
    }
    context.checkContext();
    int status = ca_create_subscription(
        type, 1, chid, DBE_VALUE|DBE_ALARM,
        eventCallback, this, &myevid);
    if(status!=ECA_NORMAL) {
        requester.message(String(
            "ca_create_subscription failed"),warningMessage);
    }
}

void CAV3MonitorPvt::stop()
{
    context.checkContext();
    ca_clear_subscription(myevid);
}

CAV3Monitor::CAV3Monitor(
    CAV3MonitorRequester &requester,
    String pvName,V3Type v3Type)
: pImpl(new CAV3MonitorPvt(requester,pvName,v3Type))
{}

CAV3Monitor::~CAV3Monitor()
{
    delete pImpl;
}

CAV3Data & CAV3Monitor::getData()
{
    return pImpl->data;
}

void CAV3Monitor::connect()
{
    pImpl->connect();
}

void CAV3Monitor::start()
{
    pImpl->start();
}

void CAV3Monitor::stop()
{
    pImpl->stop();
}

const char *CAV3Monitor::getStatusString(long status)
{
    return ca_message(status);
}


bool CAV3Monitor::hasReadAccess()
{
    if(pImpl->chid==0) return false;
    if(ca_read_access(pImpl->chid)) return true;
    return false;
}

bool CAV3Monitor::hasWriteAccess()
{
    if(pImpl->chid==0) return false;
    if(ca_write_access(pImpl->chid)) return true;
    return false;
}

bool CAV3Monitor::isConnected()
{
    if(pImpl->chid==0) return false;
    enum channel_state state = ca_state(pImpl->chid);
    if(state==cs_conn) return true;
    return false;
}

