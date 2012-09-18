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

#include <pv/multiValueChannel.h>
#include <pv/channelProviderLocal.h>
#include <pv/service.h>
#include <pv/serverContext.h>


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;

static ChannelProviderLocalPtr localProvider;
static ChannelFind::shared_pointer noChannelFind;
static Channel::shared_pointer noChannel;

static String multiValueProvider("multiValue");

MultiValueChannel::~MultiValueChannel() {}

MultiValueChannel::MultiValueChannel(
     String const &channelName,
     ValueChannelPtrArrayPtr const &arrayValueChannel)
: ChannelBaseProvider(multiValueProvider),
  channelName(channelName),
  arrayValueChannel(arrayValueChannel)
{
}

bool MultiValueChannel::connect(RequesterPtr const & requester)
{
    return false;
}

void MultiValueChannel::destroy()
{
}

ChannelFind::shared_pointer MultiValueChannel::channelFind(
    String const & channelName,
    ChannelFindRequester::shared_pointer const & channelFindRequester)
{
    return noChannelFind;
}

Channel::shared_pointer MultiValueChannel::createChannel(
    String const & channelName,
    ChannelRequester::shared_pointer  const & channelRequester,
    short priority,
    String const & address)
{
    return noChannel;
}




static void createMessage(RequesterPtr const & requester,String const &message)
{
    String mess("V3RecordVALS::create error ");
    mess += message;
    requester->message(mess,errorMessage);
}


bool MultiValueChannel::create(
    RequesterPtr const & requester,
    ChannelProvider::shared_pointer const &valueChannelProvider,
    String const & channelName,
    StringArrayPtr const & fieldNames,
    StringArrayPtr const & valueChannelNames)
{
    static Mutex mutex;
    Lock lock(mutex);
    ChannelProvider::shared_pointer provider = getChannelAccess()->getProvider("local");
    if(provider.get()==NULL) {
       createMessage(requester,"provider local does not exist");
       return false;
    }
    localProvider = static_pointer_cast<ChannelProviderLocal>(provider);
    ValueChannelPtrArrayPtr accessPtrArrayPtr(new ValueChannelPtrArray());
    size_t numberChannels = valueChannelNames->size();
    accessPtrArrayPtr->reserve(numberChannels);
    for(size_t i=0; i<numberChannels; i++)
    {
        String recordName = (*valueChannelNames)[i];
        accessPtrArrayPtr->push_back(ValueChannelPtr(
            new ValueChannel(requester,valueChannelProvider,recordName)));
    }
    MultiValueChannelPtr multiValueChannels(
        new MultiValueChannel(channelName,accessPtrArrayPtr));
    bool result = multiValueChannels->connect(requester);
    if(!result) return false;
    localProvider->registerProvider(channelName,multiValueChannels);
    return true;
}


}}

