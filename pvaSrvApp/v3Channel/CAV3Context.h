/* CAV3Context.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
/* Marty Kraimer 2011.03 
 * This creates a CAV3 context the first time a thread calls 
 * CAV3ContextCreate::create and then calls ca_attach_context
 * from checkContext if the caller is a thread that is not the
 * thread that called CAV3ContextCreate,create.
 */

#ifndef CAV3CONTEXT_H
#define CAV3CONTEXT_H

#include <map>
#include <list>

#include <epicsThread.h>

#include <pv/lock.h>
#include <pv/requester.h>


class CAV3Context;
typedef std::tr1::shared_ptr<CAV3Context> CAV3ContextPtr;
class CAV3ContextCreate;

class CAV3Context {
public:
    POINTER_DEFINITIONS(CAV3Context);
    ~CAV3Context();
    void release();
    void stop();
    void exception(epics::pvData::String const &message);
    void checkContext();
private:
    std::list<epicsThreadId> auxThreadList ;
    epics::pvData::Mutex mutex;
    CAV3Context(
       epics::pvData::RequesterPtr const & requester);
    epics::pvData::RequesterPtr requester;
    epicsThreadId threadId;
    struct ca_client_context *context;
    int referenceCount;
    friend class CAV3ContextCreate;
};

class CAV3ContextCreate {
public:
    static CAV3ContextPtr get(epics::pvData::RequesterPtr const & requester);
private:
    static void erase(epicsThreadId id);
    static std::map<epicsThreadId,CAV3ContextPtr> contextMap;
    static epics::pvData::Mutex mutex;
    friend class CAV3Context;
};

#endif  /* CAV3CONTEXT_H */
