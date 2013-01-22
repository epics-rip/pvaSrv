/* CAV3Context.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
/* Marty Kraimer 2011.03 
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <algorithm>

#include <epicsExit.h>
#include <cadef.h>
#include <db_access.h>
#include <dbDefs.h>

#include "CAV3Context.h"
#include "v3ChannelDebug.h"

using namespace epics::pvData;
using namespace epics::pvaSrv;


extern "C" {

static void exceptionCallback(struct exception_handler_args args)
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::exceptionCallback\n");
    CAV3Context *context = static_cast<CAV3Context *>(args.usr);   
    String message(ca_message(args.stat));
    context->exception(message);
}

static void threadExitFunc(void *arg)
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::threadExitFunc\n");
    CAV3Context * context = static_cast<CAV3Context * >(arg);
    context->stop();
}

} //extern "C"

CAV3Context::CAV3Context(RequesterPtr const & requester)
: 
  requester(requester),
  threadId(epicsThreadGetIdSelf()),
  context(0),
  referenceCount(0)
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::CAV3Context\n");
    SEVCHK(ca_context_create(ca_enable_preemptive_callback),
        "CAV3Context::CAV3Context calling ca_context_create");
    int status = ca_add_exception_event(exceptionCallback,this);
    if(status!=ECA_NORMAL) {
        requester->message("ca_add_exception_event failed",warningMessage);
    }
    context = ca_current_context();
    epicsAtThreadExit(threadExitFunc,this);
}

CAV3Context::~CAV3Context()
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::~CAV3Context\n");
}

// TODO commented out exceptions to avoid SIGSEGs (to reproduce: pvget -m counter01 and then CTRL+C the pvget)
void CAV3Context::stop()
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::stop\n");
    epicsThreadId id = epicsThreadGetIdSelf();
    if(id!=threadId) {
    	printf("CAV3Context::stop not same thread\n");
    	return;
        //throw std::logic_error(String(
        //   "CAV3Context::stop not same thread"));
    }
    if(referenceCount!=0) {
    	printf("CAV3Context::stop referenceCount != 0 value %d\n",
            referenceCount);
    	return;
        //throw std::logic_error(String(
        //   "CAV3Context::stop referenceCount != 0"));
    }
    else
    {
        CAV3ContextCreate::erase(threadId);
        ca_context_destroy();
    }
}

typedef std::list<epicsThreadId>::iterator threadListIter;

void CAV3Context::checkContext()
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::checkContext\n");
    epicsThreadId id = epicsThreadGetIdSelf();
    if(id==threadId) return;
    Lock xx(mutex);
    threadListIter iter = std::find(
       auxThreadList.begin(),auxThreadList.end(),id);
    if(iter!=auxThreadList.end()) return;
    auxThreadList.push_front(id);
    SEVCHK(ca_attach_context(context),
        "CAV3Context::checkContext calling ca_context_create");
}

void CAV3Context::release()
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::release referenceCount %d\n",referenceCount);
    Lock xx(mutex);
    referenceCount--;
}

void CAV3Context::exception(String const &message)
{
    Lock xx(mutex);
    requester->message(message,errorMessage);
}

typedef std::map<epicsThreadId,CAV3ContextPtr>::iterator contextMapIter;

std::map<epicsThreadId,CAV3ContextPtr>CAV3ContextCreate::contextMap;
Mutex CAV3ContextCreate::mutex;

CAV3ContextPtr CAV3ContextCreate::get(RequesterPtr const &requester)
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::get\n");
    Lock xx(mutex);
    epicsThreadId id = epicsThreadGetIdSelf();
    contextMapIter iter = contextMap.find(id);
    if(iter!=contextMap.end()) {
        CAV3ContextPtr context = iter->second;
        context->referenceCount++;
        return context;
    }
    CAV3ContextPtr context(new CAV3Context(requester));
    contextMap[id] = context;
    context->referenceCount++;
    return context;
}

void CAV3ContextCreate::erase(epicsThreadId threadId)
{
    if(V3ChannelDebug::getLevel()>0) printf("CAV3Context::erase\n");
    Lock xx(mutex);
    contextMap.erase(threadId);
}
