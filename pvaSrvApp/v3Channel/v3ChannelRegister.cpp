/*v3ChannelRegister.cpp */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>

#include <cantProceed.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <iocsh.h>

#include <epicsExport.h>

#include <pvIntrospect.h>
#include <pvData.h>
#include <pvAccess.h>
#include <serverContext.h>

#include "pvDatabase.h"
#include "v3Channel.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvIOC;

class V3ChannelRun : public Runnable {
public:
    V3ChannelRun();
    ~V3ChannelRun();
    virtual void run();
private:
    Event event;
    ServerContextImpl::shared_pointer ctx;
    Thread *thread;
};

V3ChannelRun::V3ChannelRun()
: event(),
  ctx(ServerContextImpl::create()),
  thread(new Thread(String("v3ChannelServer"),lowerPriority,this))
{}

V3ChannelRun::~V3ChannelRun()
{
    ctx->shutdown();
    // we need thead.waitForCompletion()
    event.wait();
    epicsThreadSleep(1.0);
    delete thread;
}

void V3ChannelRun::run()
{
    ChannelProvider::shared_pointer channelProvider(new V3ChannelProvider());
    registerChannelProvider(channelProvider);
    ctx->setChannelProviderName(channelProvider->getProviderName());
    ctx->initialize(getChannelAccess());
    ctx->printInfo();
    ctx->run(0);
    ctx->destroy();
    event.signal();
}

static V3ChannelRun *myRun = 0;

static const iocshFuncDef startV3ChannelFuncDef = {
    "startV3Channel", 0, 0
};
static void startV3ChannelCallFunc(const iocshArgBuf *args)
{
    if(myRun!=0) {
        printf("server already started\n");
        return;
    }
    myRun = new V3ChannelRun();
}

static const iocshFuncDef stopV3ChannelFuncDef = {
    "stopV3Channel", 0, 0
};
static void stopV3ChannelCallFunc(const iocshArgBuf *args)
{
   printf("stopPVAccessServer\n");
   if(myRun!=0) delete myRun;
   myRun = 0;
}

static void startV3ChannelRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&startV3ChannelFuncDef, startV3ChannelCallFunc);
    }
}

static void stopV3ChannelRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&stopV3ChannelFuncDef, stopV3ChannelCallFunc);
    }
}

epicsExportRegistrar(startV3ChannelRegister);
epicsExportRegistrar(stopV3ChannelRegister);