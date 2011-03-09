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

static Status notFoundStatus(Status::STATUSTYPE_ERROR,String("pv not found"));

static V3ChannelProvider *channelProvider = 0;
static Mutex mutex;
static bool isRegistered = false;

static void myDestroy(void*)
{
    Lock xx(mutex);
    if(channelProvider==0) return;
    channelProvider->destroy();
    channelProvider = 0;
}

V3ChannelProvider &V3ChannelProvider::getChannelProvider()
{
    Lock xx(mutex);
    if(channelProvider==0){
        channelProvider = new V3ChannelProvider();
        epicsAtExit(&myDestroy,0);
    }
    if(!isRegistered) {
        isRegistered = true;
        registerChannelProvider(channelProvider);
    }
    return *channelProvider;

}

V3ChannelProvider::V3ChannelProvider()
: providerName("v3Local"),channelList()
{
}

V3ChannelProvider::~V3ChannelProvider()
{
    while(true) {
        ChannelListNode *node = channelList.removeHead();
        if(node==0) break;
        V3Channel &v3Channel = node->getObject();
        v3Channel.destroy();
        delete node;
    }
}

void V3ChannelProvider::destroy()
{
    Lock xx(mutex);
    isRegistered = false;
    unregisterChannelProvider(this);
    // do not destroy since is singleton
}

String V3ChannelProvider::getProviderName()
{
    return providerName;
}


ChannelFind *V3ChannelProvider::channelFind(
    String channelName,
    ChannelFindRequester *channelFindRequester)
{
    struct dbAddr dbaddr;
    long result = dbNameToAddr(channelName.c_str(),&dbaddr);
    if(result!=0) {
        channelFindRequester->channelFindResult(notFoundStatus,0,false);
        return 0;
    }
    channelFindRequester->channelFindResult(Status::OK,0,true);
    return 0;
}

Channel *V3ChannelProvider::createChannel(
    String channelName,
    ChannelRequester *channelRequester,
    short priority)
{
    return createChannel(channelName,channelRequester,priority,"");
}

Channel *V3ChannelProvider::createChannel(
    String channelName,
    ChannelRequester *channelRequester,
    short priority,
    String address)
{
    struct dbAddr dbaddr;
    long result = dbNameToAddr(channelName.c_str(),&dbaddr);
    if(result!=0) {
        channelRequester->channelCreated(notFoundStatus,0);
        return 0;
    }
    std::auto_ptr<DbAddr> addr(new DbAddr());
    memcpy(addr.get(),&dbaddr,sizeof(dbaddr));
    V3Channel *v3Channel = new V3Channel(*channelRequester,channelName,addr);
    ChannelListNode *node = new ChannelListNode(*v3Channel);
    channelList.addTail(*node);
    channelRequester->channelCreated(Status::OK,v3Channel);
    return v3Channel;
}

}}
