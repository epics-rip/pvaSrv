/* v3ChannelProvider.cpp */
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

#include "v3Channel.h"

namespace epics { namespace pvaSrv { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelProvider::V3ChannelProvider()
: ChannelBaseProvider("v3Channel")
{
//printf("V3ChannelProvider::V3ChannelProvider\n");
}

V3ChannelProviderPtr V3ChannelProvider::getV3ChannelProvider()
{
    static V3ChannelProviderPtr v3ChannelProvider;
    static Mutex mutex;
    Lock xx(mutex);

    if(v3ChannelProvider.get()==0) {
        v3ChannelProvider = V3ChannelProviderPtr(new V3ChannelProvider());
    }
    return v3ChannelProvider;
}

V3ChannelProvider::~V3ChannelProvider()
{
//printf("V3ChannelProvider::~V3ChannelProvider\n");
}

ChannelFind::shared_pointer V3ChannelProvider::channelFind(
    String const & channelName,
    ChannelFindRequester::shared_pointer const &channelFindRequester)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    channelFound(((result==0) ? true: false),channelFindRequester);
    return ChannelFind::shared_pointer();
}

Channel::shared_pointer V3ChannelProvider::createChannel(
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
    V3Channel *v3Channel = new V3Channel(
            getPtrSelf(),
            channelRequester,channelName,addr);
    ChannelBase::shared_pointer channel(v3Channel);
    v3Channel->init();
    channelCreated(channel);
    return channel;
}

}}
