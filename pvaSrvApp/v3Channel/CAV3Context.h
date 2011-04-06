/* v3CAContext.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/* Marty Kraimer 2011.03 */
/* This connects to a V3 record and calls V3CA to monitor data.
 * Only numeric scalar, timeStamp, and alarm are provided.
 */

#ifndef V3CACONTEXT_H
#define V3CACONTEXT_H

#include <map>
#include <list>

#include <epicsThread.h>

#include <lock.h>
#include <requester.h>


class CAV3ContextCreate;

class CAV3Context {
public:
    ~CAV3Context();
    void release();
    void stop();
    void exception(epics::pvData::String message);
    void checkContext();
private:
    std::list<epicsThreadId> auxThreadList ;
    epics::pvData::Mutex mutex;
    CAV3Context(
       epics::pvData::Requester &requester);
    epics::pvData::Requester &requester;
    epicsThreadId threadId;
    struct ca_client_context *context;
    int referenceCount;
    friend class CAV3ContextCreate;
};

class CAV3ContextCreate {
public:
    static CAV3Context &get(epics::pvData::Requester &requester);
private:
    static void erase(epicsThreadId id);
    static std::map<epicsThreadId,CAV3Context*> contextMap;
    static epics::pvData::Mutex mutex;
    friend class CAV3Context;
};

#endif  /* V3CACONTEXT_H */
