/* v3CAMonitor.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/* Marty Kraimer 2011.03 */
/* This connects to a V3 record and calls V3CA to monitor data.
 * Only numeric scalar, timeStamp, and alarm are provided.
 */

#ifndef V3CAMONITOR_H
#define V3CAMONITOR_H

#include <epicsTime.h>

#include <noDefaultMethods.h>
#include <pvIntrospect.h>
#include <requester.h>
#include <lock.h>

class CAV3Context {
public:
    CAV3Context(epics::pvData::Requester &requester);
    ~CAV3Context();
    void start();

    epics::pvData::Requester &requester;
private:
    epics::pvData::Mutex mutex;
    struct ca_client_context *context;
};

enum V3Type {
    v3Enum,
    v3Byte,
    v3Short,
    v3Int,
    v3Float,
    v3Double,
    v3String
};

struct CAV3Data {
    CAV3Data();
    ~CAV3Data();
    /* The following have new values after each data event*/
    union { //only used for scalar values
        epics::pvData::int8  byteValue;
        epics::pvData::int16 shortValue;
        epics::pvData::int32 intValue;
        float                floatValue;
        double               doubleValue;
    };
    epicsTimeStamp  timeStamp;
    int             sevr;
    int             stat;
    const char *    status;
};

class CAV3MonitorRequester : public virtual epics::pvData::Requester {
public:
    virtual ~CAV3MonitorRequester(){}
    virtual void exceptionCallback(long status,long op) = 0;
    virtual void connectionCallback() = 0;
    virtual void accessRightsCallback() = 0;
    virtual void eventCallback(long status) = 0;
};

class CAV3Monitor : private epics::pvData::NoDefaultMethods {
public:
    CAV3Monitor(
        CAV3MonitorRequester &requester,
        epics::pvData::String pvName,
        V3Type v3Type,
        CAV3Context &context);
    ~CAV3Monitor();
    CAV3Data & getData();
    void connect();
    void start();
    void disconnect();
    const char * getStatusString(long status);
    bool hasReadAccess();
    bool hasWriteAccess();
    bool isConnected();
private:
    class CAV3MonitorPvt *pImpl;
};

#endif  /* V3CAMONITOR_H */
