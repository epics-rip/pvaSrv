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

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/noDefaultMethods.h>
#include <pv/lock.h>

#include "dbPv.h"

namespace epics { namespace pvaSrv { 

using namespace epics::pvData;
using namespace epics::pvAccess;

DbPvProvider::DbPvProvider()
: ChannelBaseProvider("dbPv")
{
//printf("dbPvProvider::dbPvProvider\n");
}

DbPvProviderPtr DbPvProvider::getDbPvProvider()
{
    static DbPvProviderPtr dbPvProvider;
    static Mutex mutex;
    Lock xx(mutex);

    if(dbPvProvider.get()==0) {
        dbPvProvider = DbPvProviderPtr(new DbPvProvider());
    }
    return dbPvProvider;
}

DbPvProvider::~DbPvProvider()
{
//printf("dbPvProvider::~dbPvProvider\n");
}

ChannelFind::shared_pointer DbPvProvider::channelFind(
    String const & channelName,
    ChannelFindRequester::shared_pointer const &channelFindRequester)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    channelFound(((result==0) ? true: false),channelFindRequester);
    return ChannelFind::shared_pointer();
}

Channel::shared_pointer DbPvProvider::createChannel(
    String const & channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority,
    String const & address)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    if(result!=0) {
        channelNotCreated(channelRequester);
        return Channel::shared_pointer();
    }
    std::tr1::shared_ptr<DbAddr> addr(new DbAddr());
    memcpy(addr.get(),&dbAddr,sizeof(dbAddr));
    DbPv *v3Channel = new DbPv(
            getPtrSelf(),
            channelRequester,channelName,addr);
    ChannelBase::shared_pointer channel(v3Channel);
    v3Channel->init();
    channelCreated(channel);
    return channel;
}

}}
