/* multiValue.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#ifndef MULTIVALUE_H
#define MULTIVALUE_H

#include <pv/channelBase.h>
#include <pv/convert.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>

#include <valueChannel.h>

namespace epics { namespace pvIOC { 

class MultiValueChannel;
class MultiValueChannelProvider;
class MultiValueChannelGet;
typedef std::tr1::shared_ptr<MultiValueChannel> MultiValueChannelPtr;
typedef std::tr1::shared_ptr<MultiValueChannelProvider> MultiValueChannelProviderPtr;
typedef std::tr1::shared_ptr<MultiValueChannelGet> MultiValueChannelGetPtr;

class MultiValueChannel :
    public epics::pvAccess::ChannelBase
{
public:
    POINTER_DEFINITIONS(MultiValueChannel);
    MultiValueChannel(MultiValueChannelProviderPtr const & channelProvider,
        epics::pvAccess::ChannelRequester::shared_pointer const & requester);
    bool create();
    virtual ~MultiValueChannel() {}
    virtual void destroy();
    virtual void getField(
        epics::pvAccess::GetFieldRequester::shared_pointer const &requester,
        epics::pvData::String const &subField);
    virtual epics::pvAccess::ChannelGet::shared_pointer createChannelGet(
        epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
private:
    MultiValueChannelProviderPtr multiValueProvider;
    epics::pvAccess::ChannelRequester::shared_pointer const & requester;
    ValueChannelPtrArrayPtr arrayValueChannel;
    epics::pvData::StructureConstPtr structure;
    epics::pvData::Mutex mutex;
    epics::pvData::Event event;
    friend class MultiValueChannelGet;
};

class MultiValueChannelGet :
  public virtual epics::pvAccess::ChannelGet,
  public std::tr1::enable_shared_from_this<MultiValueChannelGet>
{
public:
    POINTER_DEFINITIONS(MultiValueChannelGet);
    MultiValueChannelGet(
        MultiValueChannelPtr const & channel,
        epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester);
    virtual ~MultiValueChannelGet();
    bool init(epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String const &message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void get(bool lastRequest);
    virtual void lock();
    virtual void unlock();
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    MultiValueChannelPtr multiValueChannel;
    epics::pvAccess::ChannelGetRequester::shared_pointer  channelGetRequester;
    ValueChannelPtrArrayPtr arrayValueChannel;
    epics::pvData::PVStructurePtr pvTop;
    epics::pvData::BitSetPtr bitSet;
    epics::pvData::Alarm alarm;
    epics::pvData::PVAlarm pvAlarm;
    epics::pvData::TimeStamp timeStamp;
    epics::pvData::PVTimeStamp pvTimeStamp;
};


class MultiValueChannelProvider :
   public epics::pvAccess::ChannelBaseProvider
{
public:
    POINTER_DEFINITIONS(MultiValueChannelProvider);
    /**
     * Create a PVStructure and add to channelProviderLocal.
     * @param requester The requester.
     * @param valueChannelProvider Provider for value channels.
     * @param channelName The channelName.
     * @param fieldNames An array of fieldNames for the top level PVStructure.
     * @param valueChannelNames An array of V3 record names.
     */
    MultiValueChannelProvider(
         epics::pvData::RequesterPtr const & requester,
         epics::pvAccess::ChannelProvider::shared_pointer const &valueChannelProvider,
         epics::pvData::String const & channelName,
         epics::pvData::StringArrayPtr const & fieldNames,
         epics::pvData::StringArrayPtr const & valueChannelNames);
    virtual ~MultiValueChannelProvider();
    virtual void destroy();
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(
        epics::pvData::String const & channelName,
        epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String const & channelName,
        epics::pvAccess::ChannelRequester::shared_pointer  const & channelRequester,
        short priority,
        epics::pvData::String const & address);
    virtual std::tr1::shared_ptr<epics::pvAccess::ChannelProvider> getChannelProvider()
        {return getPtrSelf();}
    virtual void cancelChannelFind() {}
    void dump();
private:
    epics::pvData::RequesterPtr requester;
    epics::pvAccess::ChannelProvider::shared_pointer valueChannelProvider;
    epics::pvData::String channelName;
    epics::pvData::StringArrayPtr fieldNames;
    epics::pvData::StringArrayPtr valueChannelNames;
    epics::pvData::Mutex mutex;
    epics::pvData::Event event;
    friend class MultiValueChannel;
};

}}

#endif  /* MULTIVALUE_H */
