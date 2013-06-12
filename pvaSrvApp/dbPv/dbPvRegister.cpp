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
#include <pv/channelBase.h>
#include <pv/serverContext.h>

#include "dbPv.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaSrv;

class DbPvCTX;
typedef std::tr1::shared_ptr<DbPvCTX> DbPvCTXPtr;

class DbPvCTX :
    public Runnable,
    public std::tr1::enable_shared_from_this<DbPvCTX>
{
public:
    POINTER_DEFINITIONS(DbPvCTX);
    static DbPvCTXPtr getDbPvCTX();
    DbPvProviderPtr getDbPvProvider() {return dbPvProvider;}
    ChannelBaseProviderFactory::shared_pointer getChannelProviderFactory() {return factory;}
    virtual ~DbPvCTX();
    virtual void run();
private:
    DbPvCTX();
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    DbPvProviderPtr dbPvProvider;
    ChannelBaseProviderFactory::shared_pointer factory;
    Event event;
    ServerContextImpl::shared_pointer ctx;
    Thread *thread;
};

DbPvCTXPtr DbPvCTX::getDbPvCTX()
{
    static DbPvCTXPtr dbPvCTX;
    static Mutex mutex;
    Lock xx(mutex);

   if(dbPvCTX.get()==0) {
      dbPvCTX = DbPvCTXPtr(new DbPvCTX());
   }
   return dbPvCTX;
}

DbPvCTX::DbPvCTX()
: dbPvProvider(DbPvProvider::getDbPvProvider()),
  factory(new ChannelBaseProviderFactory(dbPvProvider)),
  event(),
  ctx(ServerContextImpl::create()),
  thread(new Thread(String("dbPvServer"),lowerPriority,this,epicsThreadStackBig))
{}

DbPvCTX::~DbPvCTX()
{
    ctx->shutdown();
    // we need thead.waitForCompletion()
    event.wait();
    epicsThreadSleep(1.0);
    delete thread;
}

void DbPvCTX::run()
{
    factory->registerSelf();
    String providerName = dbPvProvider->getProviderName();
    ctx->setChannelProviderName(providerName);
    ctx->initialize(getChannelAccess());
    ctx->printInfo();
    ctx->run(0);
    ctx->destroy();
    event.signal();
}

static const iocshArg setDbPvDebugLevelArg0 = { "level", iocshArgInt };
static const iocshArg *setDbPvDebugLevelArgs[] =
     {&setDbPvDebugLevelArg0};
static const iocshFuncDef setDbPvDebugLevelFuncDef = {
    "pvaSrvSetDebugLevel", 1, setDbPvDebugLevelArgs };

extern "C" void setDbPvDebugLevel(const iocshArgBuf *args)
{
    int level = args[0].ival;
    DbPvDebug::setLevel(level);
    printf("new level %d\n",level);
}

static const iocshFuncDef pvaSrvStartFuncDef = {
    "pvaSrvStart", 0, 0
};

extern "C" void pvaSrvStart(const iocshArgBuf *args)
{
    DbPvCTXPtr dbPvCTX = DbPvCTX::getDbPvCTX();
    epicsThreadSleep(.1);
    dbPvCTX->getChannelProviderFactory()->registerSelf();
}

static const iocshFuncDef pvaSrvStopFuncDef = {
    "pvaSrvStop", 0, 0
};

extern "C" void pvaSrvStop(const iocshArgBuf *args)
{
    DbPvCTXPtr dbPvCTX = DbPvCTX::getDbPvCTX();
    dbPvCTX->getChannelProviderFactory()->unregisterSelf();
}

static void dbPvRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&setDbPvDebugLevelFuncDef, setDbPvDebugLevel);
        iocshRegister(&pvaSrvStartFuncDef, pvaSrvStart);
        iocshRegister(&pvaSrvStopFuncDef, pvaSrvStop);
    }
}

epicsExportRegistrar(dbPvRegister);
