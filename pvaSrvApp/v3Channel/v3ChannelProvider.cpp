/* v3ChannelProvider.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#include <string>
#include <stdexcept>
#include <memory>

#include <pvIntrospect.h>
#include <pvData.h>
#include <noDefaultMethods.h>
#include <lock.h>
#include <epicsExit.h>
#include <dbAccess.h>

#include "support.h"
#include "pvDatabase.h"
#include "v3Channel.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelProvider::V3ChannelProvider()
: PVServiceBaseProvider("v3Channel")
{
printf("V3ChannelProvider::V3ChannelProvider\n");
}

V3ChannelProvider::~V3ChannelProvider()
{
printf("V3ChannelProvider::~V3ChannelProvider\n");
}

ChannelFind::shared_pointer V3ChannelProvider::channelFind(
    String channelName,
    ChannelFindRequester::shared_pointer const &channelFindRequester)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    channelFound(((result==0) ? true: false),channelFindRequester);
    return ChannelFind::shared_pointer();
}

Channel::shared_pointer V3ChannelProvider::createChannel(
    String channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority,
    String address)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    if(result!=0) {
        channelNotCreated(channelRequester);
        return Channel::shared_pointer();
    }
    std::auto_ptr<DbAddr> addr(new DbAddr());
    memcpy(addr.get(),&dbAddr,sizeof(dbAddr));
    V3Channel *v3Channel = new V3Channel(
            getChannelProvider(),
            channelRequester,channelName,addr);
    PVServiceBase::shared_pointer channel(v3Channel);
    v3Channel->init();
    channelCreated(channel);
    return channel;
}

}}
