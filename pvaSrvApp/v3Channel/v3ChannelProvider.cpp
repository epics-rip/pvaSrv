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

static String providerName("v3Channel");

static Mutex mutex;
static bool isRegistered = false;

static ChannelProvider::shared_pointer channelProvider(new V3ChannelProvider());

ChannelProvider::shared_pointer const &V3ChannelProvider::getChannelProvider()
{
    Lock xx(mutex);
    if(!isRegistered) {
        isRegistered = true;
        registerChannelProvider(channelProvider);
    }
    return channelProvider;
}

V3ChannelProvider::V3ChannelProvider()
: providerName("v3Channel"),channelList()
{
printf("V3ChannelProvider::V3ChannelProvider\n");
}

V3ChannelProvider::~V3ChannelProvider()
{
printf("V3ChannelProvider::~V3ChannelProvider\n");
}

void V3ChannelProvider::destroy()
{
printf("V3ChannelProvider::destroy\n");
    Lock xx(mutex);
    if(!isRegistered) return;
    isRegistered = false;
    unregisterChannelProvider(channelProvider);
    while(true) {
        ChannelListNode *node = channelList.getHead();
        if(node==0) break;
        V3Channel &v3Channel = node->getObject();
        v3Channel.destroy();
    }
}

String V3ChannelProvider::getProviderName()
{
    return providerName;
}


ChannelFind::shared_pointer V3ChannelProvider::channelFind(
    String channelName,
    ChannelFindRequester::shared_pointer const &channelFindRequester)
{
    struct dbAddr dbAddr;
    long result = dbNameToAddr(channelName.c_str(),&dbAddr);
    if(result!=0) {
        Status notFoundStatus(Status::STATUSTYPE_ERROR,String("pv not found"));
        channelFindRequester.get()->channelFindResult(
            notFoundStatus,
            ChannelFind::shared_pointer(),
            false);
        return ChannelFind::shared_pointer();
    }
    channelFindRequester->channelFindResult(
        Status::OK,
        ChannelFind::shared_pointer(),
        true);
    return ChannelFind::shared_pointer();
}

Channel::shared_pointer V3ChannelProvider::createChannel(
    String channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority)
{
    return createChannel(channelName,channelRequester,priority,"");
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
        Status notFoundStatus(Status::STATUSTYPE_ERROR,String("pv not found"));
        channelRequester->channelCreated(
            notFoundStatus,
            Channel::shared_pointer());
        return Channel::shared_pointer();
    }
    std::auto_ptr<DbAddr> addr(new DbAddr());
    memcpy(addr.get(),&dbAddr,sizeof(dbAddr));
    
    V3Channel::shared_pointer v3Channel(
        new V3Channel(
            std::tr1::static_pointer_cast<V3ChannelProvider>(channelProvider),
            channelRequester,channelName,addr));
    ChannelListNode & node = v3Channel->init();
    channelList.addTail(node);
    channelRequester->channelCreated(Status::OK,v3Channel);
    return v3Channel;
}

void V3ChannelProvider::removeChannel(ChannelListNode &node)
{
     channelList.remove(node);   
}

}}
