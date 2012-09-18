/* multiValueChannel.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#ifndef MULTIVALUECHANNEL_H
#define MULTIVALUECHANNEL_H

#include <pv/valueChannel.h>
#include <pv/channelBase.h>

namespace epics { namespace pvIOC { 

class MultiValueChannel;
typedef std::tr1::shared_ptr<MultiValueChannel> MultiValueChannelPtr;

class MultiValueChannel :
   public virtual epics::pvAccess::ChannelBaseProvider
{
public:
    POINTER_DEFINITIONS(MultiValueChannel);
    /**
     * Create a PVStructure and add to channelProviderLocal.
     * @param requester The requester.
     * @param valueChannelProvider Provider for value channels.
     * @param channelName The channelName.
     * @param fieldNames An array of fieldNames for the top level PVStructure.
     * @param valueChannelNames An array of V3 record names.
     */
    static bool create(
         epics::pvData::RequesterPtr const & requester,
         epics::pvAccess::ChannelProvider::shared_pointer const &valueChannelProvider,
         epics::pvData::String const & channelName,
         epics::pvData::StringArrayPtr const & fieldNames,
         epics::pvData::StringArrayPtr const & valueChannelNames);
    virtual ~MultiValueChannel();
    virtual void destroy();
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(
        epics::pvData::String const & channelName,
        epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String const & channelName,
        epics::pvAccess::ChannelRequester::shared_pointer  const & channelRequester,
        short priority,
        epics::pvData::String const & address);
private:
    MultiValueChannel(
         epics::pvData::String const &channelName,
         ValueChannelPtrArrayPtr const &arrayValueChannel);
    bool connect(epics::pvData::RequesterPtr const & requester);
    epics::pvAccess::ChannelProvider::shared_pointer valueChannelProvider;
    epics::pvData::String channelName;
    ValueChannelPtrArrayPtr arrayValueChannel;
    epics::pvData::Mutex mutex;
    epics::pvData::Event event;
};

}}

#endif  /* MULTIVALUECHANNEL_H */
