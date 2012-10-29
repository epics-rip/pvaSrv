/*v3ChannelRegister.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

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

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/serverContext.h>

#include <pv/v3Channel.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvIOC;

class V3ChannelCTX;
typedef std::tr1::shared_ptr<V3ChannelCTX> V3ChannelCTXPtr;

class V3ChannelCTX :
    public Runnable,
    public std::tr1::enable_shared_from_this<V3ChannelCTX>
{
public:
    POINTER_DEFINITIONS(V3ChannelCTX);
    static V3ChannelCTXPtr getV3ChannelCTX();
    V3ChannelProviderPtr getV3ChannelProvider() {return v3ChannelProvider;}
    virtual ~V3ChannelCTX();
    virtual void run();
private:
    V3ChannelCTX();
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    V3ChannelProviderPtr v3ChannelProvider;
    Event event;
    ServerContextImpl::shared_pointer ctx;
    Thread *thread;
};

V3ChannelCTXPtr V3ChannelCTX::getV3ChannelCTX()
{
    static V3ChannelCTXPtr pvV3ChannelCTX;
    static Mutex mutex;
    Lock xx(mutex);

   if(pvV3ChannelCTX.get()==0) {
      pvV3ChannelCTX = V3ChannelCTXPtr(new V3ChannelCTX());
   }
   return pvV3ChannelCTX;
}

V3ChannelCTX::V3ChannelCTX()
: v3ChannelProvider(V3ChannelProvider::getV3ChannelProvider()),
  event(),
  ctx(ServerContextImpl::create()),
  thread(new Thread(String("v3ChannelServer"),lowerPriority,this,epicsThreadStackBig))
{}

V3ChannelCTX::~V3ChannelCTX()
{
    ctx->shutdown();
    // we need thead.waitForCompletion()
    event.wait();
    epicsThreadSleep(1.0);
    delete thread;
}

void V3ChannelCTX::run()
{
    v3ChannelProvider->registerSelf();
    String providerName = v3ChannelProvider->getProviderName();
    ctx->setChannelProviderName(providerName);
    ctx->initialize(getChannelAccess());
    ctx->printInfo();
    ctx->run(0);
    ctx->destroy();
    event.signal();
}

static const iocshArg setV3ChannelDebugLevelArg0 = { "level",iocshArgInt};
static const iocshArg *setV3ChannelDebugLevelArgs[] =
     {&setV3ChannelDebugLevelArg0};
static const iocshFuncDef setV3ChannelDebugLevelFuncDef = {
    "setV3ChannelDebugLevel", 1, setV3ChannelDebugLevelArgs };
extern "C" void setV3ChannelDebugLevel(const iocshArgBuf *args)
{
    int level = args[0].ival;
    V3ChannelDebug::setLevel(level);
    printf("new level %d\n",level);
}

static const iocshFuncDef startV3ChannelFuncDef = {
    "startV3Channel", 0, 0
};
extern "C" void startV3Channel(const iocshArgBuf *args)
{
    V3ChannelCTXPtr v3ChannelCTX = V3ChannelCTX::getV3ChannelCTX();
    epicsThreadSleep(.1);
    v3ChannelCTX->getV3ChannelProvider()->registerSelf();
}

static const iocshFuncDef stopV3ChannelFuncDef = {
    "stopV3Channel", 0, 0
};
extern "C" void stopV3Channel(const iocshArgBuf *args)
{
    V3ChannelCTXPtr v3ChannelCTX = V3ChannelCTX::getV3ChannelCTX();
    v3ChannelCTX->getV3ChannelProvider()->unregisterSelf();
}

static void setV3ChannelDebugLevelRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&setV3ChannelDebugLevelFuncDef, setV3ChannelDebugLevel);
    }
}

static void startV3ChannelRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&startV3ChannelFuncDef, startV3Channel);
    }
}

static void stopV3ChannelRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&stopV3ChannelFuncDef, stopV3Channel);
    }
}

epicsExportRegistrar(setV3ChannelDebugLevelRegister);
epicsExportRegistrar(startV3ChannelRegister);
epicsExportRegistrar(stopV3ChannelRegister);
