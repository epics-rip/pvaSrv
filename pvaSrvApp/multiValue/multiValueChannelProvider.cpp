/* multiValueChannel.cpp */
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

#include <pv/channelProviderLocal.h>
#include <pv/service.h>
#include <pv/serverContext.h>

#include "multiValue.h"

namespace epics { namespace pvaSrv { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvIOC;
using std::tr1::static_pointer_cast;

static ChannelProviderLocalPtr localProvider;
static ChannelFind::shared_pointer noChannelFind;
static Channel::shared_pointer noChannel;

static String multiValueProvider("multiValue");

MultiValueChannelProvider::~MultiValueChannelProvider() {}

MultiValueChannelProvider::MultiValueChannelProvider(
     RequesterPtr const & requester,
     ChannelProvider::shared_pointer const &valueChannelProvider,
     String const &channelName,
     StringArrayPtr const & fieldNames,
     StringArrayPtr const & valueChannelNames)
: ChannelBaseProvider(multiValueProvider),
  requester(requester),
  valueChannelProvider(valueChannelProvider),
  channelName(channelName),
  fieldNames(fieldNames),
  valueChannelNames(valueChannelNames)
{}

void MultiValueChannelProvider::dump()
{
    printf("MultiValueChannelProvider channelName %s\n",channelName.c_str());
    printf("channelName %s\n",channelName.c_str());
    size_t n = fieldNames->size();
    for(size_t i=0; i<n; i++) {
    String fieldName = (*fieldNames.get())[i];
    String valueChannelName = (*valueChannelNames.get())[i];
    printf("fieldName %s valueChannelName %s\n",
    fieldName.c_str(),valueChannelName.c_str());
    }
}

void MultiValueChannelProvider::destroy()
{
}

ChannelFind::shared_pointer MultiValueChannelProvider::channelFind(
    String const & channelName,
    ChannelFindRequester::shared_pointer const & channelFindRequester)
{
    bool result =
        ((channelName.compare(this->channelName)==0) ? true : false);
    channelFound(result,channelFindRequester);
    return ChannelFind::shared_pointer();
}

Channel::shared_pointer MultiValueChannelProvider::createChannel(
    String const & channelName,
    ChannelRequester::shared_pointer  const & channelRequester,
    short priority,
    String const & address)
{
    if(!(channelName.compare(this->channelName)==0) ? true : false) {
        channelNotCreated(channelRequester);
        return noChannel;
    }
    MultiValueChannelProviderPtr xxx =
        static_pointer_cast<MultiValueChannelProvider>(getPtrSelf());
    MultiValueChannel::shared_pointer channel(
        new MultiValueChannel(xxx,channelRequester));
    if(!channel->create()) {
        channelNotCreated(channelRequester);
        return noChannel;
    } 
    channelCreated(channel);
    return channel;
}

}}

