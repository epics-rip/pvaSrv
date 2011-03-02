/* v3Channel.cpp */
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

#include "support.h"
#include "pvDatabase.h"
#include "v3Channel.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3ChannelProvider::V3ChannelProvider()
{
    throw std::logic_error(String("Not Implemented"));
}

V3ChannelProvider::~V3ChannelProvider()
{
    throw std::logic_error(String("Not Implemented"));
}

String V3ChannelProvider::getProviderName()
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelFind *V3ChannelProvider::channelFind(
    String channelName,
    ChannelFindRequester *channelFindRequester)
{
    throw std::logic_error(String("Not Implemented"));
}

Channel *V3ChannelProvider::createChannel(
    String channelName,
    ChannelRequester *channelRequester,
    short priority)
{
    throw std::logic_error(String("Not Implemented"));
}

Channel *V3ChannelProvider::createChannel(
    String channelName,
    ChannelRequester *channelRequester,
    short priority,
    String address)
{
    throw std::logic_error(String("Not Implemented"));
}


V3Channel::V3Channel(ChannelProvider &provider,
    ChannelRequester &requester,
    String name,
    String remoteAddress)
{
    throw std::logic_error(String("Not Implemented"));
}


}}
