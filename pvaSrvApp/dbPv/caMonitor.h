/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
/* Marty Kraimer 2011.03 */
/* This connects to a V3 record and calls V3CA to monitor data.
 * Only numeric scalar, timeStamp, and alarm are provided.
 */

#ifndef CAMONITOR_H
#define CAMONITOR_H

#include <epicsTime.h>

#include <pv/pvIntrospect.h>
#include <pv/requester.h>
#include <pv/lock.h>

enum V3Type {
    v3Enum,
    v3Byte,
    v3UByte,
    v3Short,
    v3UShort,
    v3Int,
    v3UInt,
    v3Float,
    v3Double,
    v3String
};

struct CAV3Data {
    CAV3Data();
    ~CAV3Data();
    /* The following have new values after each data event*/
    union { //only used for scalar values
        epics::pvData::int8   byteValue;
        epics::pvData::uint8  ubyteValue;
        epics::pvData::uint16 ushortValue;
        epics::pvData::int16  shortValue;
        epics::pvData::int32  intValue;
        epics::pvData::uint32 uintValue;
        float                 floatValue;
        double                doubleValue;
    };
    epicsTimeStamp  timeStamp;
    int             sevr;
    int             stat;
    const char *    status;
};

class CAV3MonitorRequester : public virtual epics::pvData::Requester {
public:
    POINTER_DEFINITIONS(CAV3MonitorRequester);
    virtual ~CAV3MonitorRequester(){}
    virtual void exceptionCallback(long status,long op) = 0;
    virtual void connectionCallback() = 0;
    virtual void accessRightsCallback() = 0;
    virtual void eventCallback(const char *status) = 0;
};

typedef std::tr1::shared_ptr<CAV3MonitorRequester> CAV3MonitorRequesterPtr;

class CAV3Monitor : private epics::pvData::NoDefaultMethods {
public:
    CAV3Monitor(
        CAV3MonitorRequesterPtr const &requester,
        epics::pvData::String const &pvName,
        V3Type v3Type);
    ~CAV3Monitor();
    CAV3Data & getData();
    void connect();
    void start();
    void stop();
    const char * getStatusString(long status);
    bool hasReadAccess();
    bool hasWriteAccess();
    bool isConnected();
private:
    class CAV3MonitorPvt *pImpl;
};

#endif  /* CAMONITOR_H */
