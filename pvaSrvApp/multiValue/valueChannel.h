/* valueChannel.h*/
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#ifndef VALUECHANNEL_H
#define VALUECHANNEL_H

#include <pv/lock.h>
#include <pv/event.h>

#include <pv/pvData.h>
#include <pv/alarm.h>
#include <pv/timeStamp.h>
#include <pv/pvAccess.h>

namespace epics { namespace pvIOC { 

class ValueChannel;
typedef std::tr1::shared_ptr<ValueChannel> ValueChannelPtr;
typedef std::vector<ValueChannelPtr> ValueChannelPtrArray;
typedef std::tr1::shared_ptr<ValueChannelPtrArray> ValueChannelPtrArrayPtr;

class ValueChannel :
    public epics::pvAccess::ChannelRequester,
    public epics::pvAccess::ChannelGetRequester,
    public std::tr1::enable_shared_from_this<ValueChannel>
{
public:
    POINTER_DEFINITIONS(ValueChannel);
    virtual ~ValueChannel();
    ValueChannel(
         epics::pvData::RequesterPtr const &requester,
         epics::pvAccess::ChannelProvider::shared_pointer const &channelProvider,
         epics::pvData::String const &channelName);
    void connect();
    bool waitConnect();
    epics::pvData::PVFieldPtr getValue();
    void get();
    bool waitGet();
    void getTimeStamp(epics::pvData::TimeStamp const &timeStamp);
    void getAlarm(epics::pvData::Alarm const &alarm);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    virtual void channelCreated(
        const epics::pvData::Status& status,
        epics::pvAccess::Channel::shared_pointer const & channel);
    virtual void channelStateChange(
        epics::pvAccess::Channel::shared_pointer const & channel, 
        epics::pvAccess::Channel::ConnectionState connectionState);
    virtual void channelGetConnect(
        const epics::pvData::Status& status,
        epics::pvAccess::ChannelGet::shared_pointer const & channelGet,
        epics::pvData::PVStructure::shared_pointer const & pvStructure,
        epics::pvData::BitSet::shared_pointer const & bitSet);
    virtual void getDone(const epics::pvData::Status& status);
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    epics::pvData::RequesterPtr requester;
    epics::pvAccess::ChannelProvider::shared_pointer channelProvider;
    epics::pvData::String channelName;
    epics::pvAccess::Channel::shared_pointer channel;
    epics::pvAccess::ChannelGet::shared_pointer channelGet;
    epics::pvAccess::ChannelPut::shared_pointer channelPut;
    epics::pvData::Mutex mutex;
    epics::pvData::Event event;
};


}}

#endif  /* VALUECHANNEL_H */
