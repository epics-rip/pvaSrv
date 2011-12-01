/* v3Channel.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#ifndef V3CHANNEL_H
#define V3CHANNEL_H
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>

#include <dbAccess.h>
#include <dbNotify.h>

#include <pv/status.h>
#include <pv/monitor.h>
#include <pv/monitorQueue.h>
#include <pv/linkedList.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/noDefaultMethods.h>
#include <pv/pvEnumerated.h>
#include <pv/thread.h>
#include <pv/event.h>
#include <pv/pvAccess.h>
#include <pv/lock.h>

#include <pv/channelBase.h>
#include <pv/v3CAMonitor.h>

namespace epics { namespace pvIOC { 

class V3ChannelProvider;
class V3Channel;
class V3ChannelProcess;
class V3ChannelGet;
class V3ChannelPut;
class V3ChannelMonitor;
class V3ChannelArray;

typedef struct dbAddr DbAddr;

class V3ChannelProvider :
    public epics::pvAccess::ChannelBaseProvider
{
public:
     POINTER_DEFINITIONS(V3ChannelProvider);
    V3ChannelProvider();
    virtual ~V3ChannelProvider();
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelRequester::shared_pointer  const &channelRequester,
        short priority,
        epics::pvData::String address);
};

class V3Channel :
  public virtual epics::pvAccess::ChannelBase
{
public:
    POINTER_DEFINITIONS(V3Channel);
    V3Channel(
        epics::pvAccess::ChannelBaseProvider::shared_pointer const & channelProvider,
        epics::pvAccess::ChannelRequester::shared_pointer const & requester,
        epics::pvData::String name,
        std::auto_ptr<DbAddr> addr
        );
    virtual ~V3Channel();
    void init();
    virtual void getField(
        epics::pvAccess::GetFieldRequester::shared_pointer const &requester,
        epics::pvData::String subField);
    virtual epics::pvAccess::ChannelProcess::shared_pointer createChannelProcess(
        epics::pvAccess::ChannelProcessRequester::shared_pointer const &channelProcessRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelGet::shared_pointer createChannelGet(
        epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelPut::shared_pointer createChannelPut(
        epics::pvAccess::ChannelPutRequester::shared_pointer const &channelPutRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvData::Monitor::shared_pointer createMonitor(
        epics::pvData::MonitorRequester::shared_pointer const &monitorRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelArray::shared_pointer createChannelArray(
        epics::pvAccess::ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual void printInfo();
    virtual void printInfo(epics::pvData::StringBuilder out);
private:
    std::auto_ptr<DbAddr> dbAddr;
    epics::pvData::FieldConstPtr recordField; 
};

class V3ChannelProcess :
  public virtual epics::pvAccess::ChannelProcess,
  public std::tr1::enable_shared_from_this<V3ChannelProcess>
{
public:
    POINTER_DEFINITIONS(V3ChannelProcess);
    V3ChannelProcess(
        epics::pvAccess::ChannelBase::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelProcess();
    bool init();
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void process(bool lastRequest);
    virtual void lock();
    virtual void unlock();
private:
    V3ChannelProcess::shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    static void notifyCallback(struct putNotify *);
    epics::pvAccess::ChannelBase::shared_pointer v3Channel;
    epics::pvAccess::ChannelProcessRequester::shared_pointer channelProcessRequester;
    DbAddr &dbAddr;
    std::auto_ptr<struct putNotify> pNotify;
    std::auto_ptr<DbAddr> notifyAddr;
    epics::pvData::Event event;
};

class V3ChannelGet :
  public virtual epics::pvAccess::ChannelGet,
  public std::tr1::enable_shared_from_this<V3ChannelGet>
{
public:
    POINTER_DEFINITIONS(V3ChannelGet);
    V3ChannelGet(
        epics::pvAccess::ChannelBase::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelGet();
    bool init(epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void get(bool lastRequest);
    virtual void lock();
    virtual void unlock();
private:
    V3ChannelGet::shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    static void notifyCallback(struct putNotify *);
    epics::pvAccess::ChannelBase::shared_pointer v3Channel;
    epics::pvAccess::ChannelGetRequester::shared_pointer channelGetRequester;
    DbAddr &dbAddr;
    bool process;
    bool firstTime;
    int propertyMask;
    epics::pvData::PVStructure::shared_pointer pvStructure;
    epics::pvData::BitSet::shared_pointer bitSet;
    std::auto_ptr<struct putNotify> pNotify;
    std::auto_ptr<DbAddr> notifyAddr;
    epics::pvData::Event event;
    epics::pvData::Mutex dataMutex;
};

class V3ChannelPut :
  public virtual epics::pvAccess::ChannelPut,
  public std::tr1::enable_shared_from_this<V3ChannelPut>
{
public:
    POINTER_DEFINITIONS(V3ChannelPut);
    V3ChannelPut(
        epics::pvAccess::ChannelBase::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelPutRequester::shared_pointer const &channelPutRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelPut();
    bool init(epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void put(bool lastRequest);
    virtual void get();
    virtual void lock();
    virtual void unlock();
private:
    V3ChannelPut::shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    static void notifyCallback(struct putNotify *);
    epics::pvAccess::ChannelBase::shared_pointer v3Channel;
    epics::pvAccess::ChannelPutRequester::shared_pointer channelPutRequester;
    DbAddr &dbAddr;
    int propertyMask;
    bool process;
    bool firstTime;
    epics::pvData::PVStructure::shared_pointer pvStructure;
    epics::pvData::BitSet::shared_pointer bitSet;
    std::auto_ptr<struct putNotify> pNotify;
    std::auto_ptr<DbAddr> notifyAddr;
    epics::pvData::Event event;
    epics::pvData::Mutex dataMutex;
};

class V3ChannelMonitor
: public virtual epics::pvData::Monitor,
  public virtual CAV3MonitorRequester,
  public std::tr1::enable_shared_from_this<V3ChannelMonitor>
{
public:
    POINTER_DEFINITIONS(V3ChannelMonitor);
    V3ChannelMonitor(
        epics::pvAccess::ChannelBase::shared_pointer const & v3Channel,
        epics::pvData::MonitorRequester::shared_pointer const & monitorRequester,
        DbAddr &dbAddr
    );
    virtual ~V3ChannelMonitor();
    bool init(epics::pvData::PVStructure::shared_pointer const &  pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual epics::pvData::Status start();
    virtual epics::pvData::Status stop();
    virtual epics::pvData::MonitorElement::shared_pointer const & poll();
    virtual void release(
        epics::pvData::MonitorElement::shared_pointer const & monitorElement);
    virtual void exceptionCallback(long status,long op);
    virtual void connectionCallback();
    virtual void accessRightsCallback();
    virtual void eventCallback(const char *);
    virtual void lock();
    virtual void unlock();
private:
    V3ChannelMonitor::shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    epics::pvAccess::ChannelBase::shared_pointer v3Channel;
    epics::pvData::MonitorRequester::shared_pointer  monitorRequester;
    DbAddr &dbAddr;
    epics::pvData::Event event;
    int propertyMask;
    bool firstTime;
    bool gotEvent;
    V3Type v3Type;
    int queueSize;
    std::auto_ptr<epics::pvData::MonitorQueue> monitorQueue;
    std::auto_ptr<CAV3Monitor> caV3Monitor;
    epics::pvData::MonitorElement::shared_pointer currentElement;
    epics::pvData::MonitorElement::shared_pointer nextElement;
};

class V3ChannelArray :
  public epics::pvAccess::ChannelArray,
  public std::tr1::enable_shared_from_this<V3ChannelArray>
{
public:
    POINTER_DEFINITIONS(V3ChannelArray);
    V3ChannelArray(
        epics::pvAccess::ChannelBase::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelArray();
    bool init(epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual void destroy();
    virtual void putArray(bool lastRequest, int offset, int count);
    virtual void getArray(bool lastRequest, int offset, int count);
    virtual void setLength(bool lastRequest, int length, int capacity);
    virtual void lock();
    virtual void unlock();
private:
    V3ChannelArray::shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    epics::pvAccess::ChannelBase::shared_pointer v3Channel;
    epics::pvAccess::ChannelArrayRequester::shared_pointer channelArrayRequester;
    DbAddr &dbAddr;
    epics::pvData::PVScalarArray::shared_pointer pvScalarArray;
    epics::pvData::Mutex dataMutex;
};

}}

#endif  /* V3CHANNEL_H */
