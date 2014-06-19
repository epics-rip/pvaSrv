/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

#include <string>
#include <stdexcept>
#include <memory>

#include <epicsExit.h>
#include <dbAccess.h>

#include <pv/serverContext.h>
#include <pv/syncChannelFind.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/noDefaultMethods.h>
#include <pv/lock.h>

#include "dbPv.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::dynamic_pointer_cast;
using std::string;

namespace epics { namespace pvaSrv { 

static string providerName("dbPv");

DbPvProvider::DbPvProvider()
{
//printf("dbPvProvider::dbPvProvider\n");
}

string DbPvProvider::getProviderName()
{
    return providerName;
}

class DbPvProviderFactory;
typedef std::tr1::shared_ptr<DbPvProviderFactory> DbPvProviderFactoryPtr;

class DbPvProviderFactory : public ChannelProviderFactory
{

public:
    POINTER_DEFINITIONS(DbPvProviderFactory);
    virtual string getFactoryName() { return providerName;}
    static DbPvProviderFactoryPtr create(
        DbPvProviderPtr const &channelProvider)
    {
        DbPvProviderFactoryPtr xxx(
            new DbPvProviderFactory(channelProvider));
        registerChannelProviderFactory(xxx);
        return xxx;
    }
    virtual  ChannelProvider::shared_pointer sharedInstance()
    {
        return channelProvider;
    }
    virtual  ChannelProvider::shared_pointer newInstance()
    {
        return channelProvider;
    }
private:
    DbPvProviderFactory(
        DbPvProviderPtr const &channelProvider)
    : channelProvider(channelProvider)
    {}
    DbPvProviderPtr channelProvider;
};


DbPvProviderPtr getDbPvProvider()
{
    static DbPvProviderPtr dbPvProvider;
    static Mutex mutex;
    Lock xx(mutex);

    if(dbPvProvider.get()==0) {
        dbPvProvider = DbPvProviderPtr(new DbPvProvider());
        ChannelProvider::shared_pointer xxx = dynamic_pointer_cast<ChannelProvider>(dbPvProvider);
        dbPvProvider->channelFinder = SyncChannelFind::shared_pointer(new SyncChannelFind(xxx));
        DbPvProviderFactory::create(dbPvProvider);
    }
    return dbPvProvider;
}

DbPvProvider::~DbPvProvider()
{
//printf("dbPvProvider::~dbPvProvider\n");
}

ChannelFind::shared_pointer DbPvProvider::channelFind(
    string const & channelName,
    ChannelFindRequester::shared_pointer const &channelFindRequester)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    if(result==0) {
        channelFindRequester->channelFindResult(
            Status::Ok,
            channelFinder,
            true);
    } else {
        Status notFoundStatus(Status::STATUSTYPE_ERROR,"pv not found");
        channelFindRequester.get()->channelFindResult(
            notFoundStatus,
            channelFinder,
            false);
    }
    return channelFinder;
}

ChannelFind::shared_pointer DbPvProvider::channelList(
    ChannelListRequester::shared_pointer const & channelListRequester)
{
    Status errorStatus(Status::STATUSTYPE_ERROR, "not implemented");
    ChannelFind::shared_pointer nullChannelFind;
    PVStringArray::const_svector none;    
    channelListRequester->channelListResult(errorStatus, nullChannelFind, none, false);
    return nullChannelFind;
}

Channel::shared_pointer DbPvProvider::createChannel(
    string const & channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority,
    string const & address)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    if(result!=0) {
        Status notFoundStatus(Status::STATUSTYPE_ERROR,"pv not found");
        channelRequester->channelCreated(
            notFoundStatus,
            Channel::shared_pointer());
        return Channel::shared_pointer();
    }
    std::tr1::shared_ptr<DbAddr> addr(new DbAddr());
    memcpy(addr.get(),&dbAddr,sizeof(dbAddr));
    DbPvPtr v3Channel(new DbPv(
            getPtrSelf(),
            channelRequester,channelName,addr));
    v3Channel->init();
    channelRequester->channelCreated(Status::Ok,v3Channel);
    return v3Channel;
}

}}
