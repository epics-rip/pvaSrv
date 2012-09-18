/* valueChannel.cpp*/
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

#include <pv/valueChannel.h>

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;

ValueChannel::~ValueChannel() {}

ValueChannel::ValueChannel(
         RequesterPtr const &requester,
         ChannelProvider::shared_pointer const &channelProvider,
         String const &channelName)
: requester(requester),
  channelProvider(channelProvider),
  channelName(channelName)
{}

void ValueChannel::connect()
{
}

bool ValueChannel::waitConnect()
{
return false;
}
PVFieldPtr ValueChannel::getValue()
{
PVFieldPtr xxx;
return xxx;
}

void ValueChannel::get()
{
}

bool ValueChannel::waitGet()
{
return false;
}

void ValueChannel::getTimeStamp(TimeStamp const &timeStamp)
{
}

void ValueChannel::getAlarm(Alarm const &alarm)
{
}

String ValueChannel::getRequesterName()
{
return NULL;
}

void ValueChannel::message(
    String const & message,
    MessageType messageType)
{
}

void ValueChannel::channelCreated(
    const Status& status,
    Channel::shared_pointer const & channel)
{
}

void ValueChannel::channelStateChange(
    Channel::shared_pointer const & channel, 
    Channel::ConnectionState connectionState)
{
}

void ValueChannel::channelGetConnect(
    const Status& status,
    ChannelGet::shared_pointer const & channelGet,
    PVStructure::shared_pointer const & pvStructure,
    BitSet::shared_pointer const & bitSet)
{
}

void ValueChannel::getDone(const Status& status)
{
}



}}
