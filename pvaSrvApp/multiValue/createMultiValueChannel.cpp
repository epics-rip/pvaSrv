/* createMultiValueChannel.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

#include <pv/createMultiValueChannel.h>
#include <pv/multiValueChannel.h>

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;


bool createMultiValueChannel(
     RequesterPtr const & requester,
     String const & channelName,
     StringArrayPtr const & fieldNames,
     StringArrayPtr const & v3RecordNames)
{
     ChannelProvider::shared_pointer valueChannelProvider =
         getChannelAccess()->getProvider("v3ChannelProvider");
     if(valueChannelProvider.get()==NULL) {
         requester->message("provider v3ChannelProvider does not exist",errorMessage);
         return false;
     }
     MultiValueChannel::create(
         requester,
         valueChannelProvider,
         channelName,
         fieldNames,
         v3RecordNames);
    return true;
}

}}
