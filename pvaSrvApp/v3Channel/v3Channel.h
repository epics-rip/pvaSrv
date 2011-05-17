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

#include <status.h>
#include <monitor.h>
#include <monitorQueue.h>
#include <linkedList.h>
#include <pvIntrospect.h>
#include <pvData.h>
#include <noDefaultMethods.h>
#include <pvEnumerated.h>
#include <thread.h>
#include <event.h>
#include <pvAccess.h>

#include "pvDatabase.h"
#include "support.h"
#include "v3CAMonitor.h"

namespace epics { namespace pvIOC { 

class V3ChannelProvider;
class V3Channel;
class V3ChannelProcess;
class V3ChannelGet;
class V3ChannelPut;
class V3ChannelMonitor;
class V3ChannelArray;

typedef struct dbAddr DbAddr;
typedef epics::pvData::LinkedListNode<V3Channel> ChannelListNode;
typedef epics::pvData::LinkedList<V3Channel> ChannelList;

typedef epics::pvData::LinkedListNode<V3ChannelProcess> ChannelProcessListNode;
typedef epics::pvData::LinkedList<V3ChannelProcess> ChannelProcessList;
typedef epics::pvData::LinkedListNode<V3ChannelGet> ChannelGetListNode;
typedef epics::pvData::LinkedList<V3ChannelGet> ChannelGetList;
typedef epics::pvData::LinkedListNode<V3ChannelPut> ChannelPutListNode;
typedef epics::pvData::LinkedList<V3ChannelPut> ChannelPutList;
typedef epics::pvData::LinkedListNode<V3ChannelMonitor> ChannelMonitorListNode;
typedef epics::pvData::LinkedList<V3ChannelMonitor> ChannelMonitorList;
typedef epics::pvData::LinkedListNode<V3ChannelArray> ChannelArrayListNode;
typedef epics::pvData::LinkedList<V3ChannelArray> ChannelArrayList;

class V3ChannelProvider : public epics::pvAccess::ChannelProvider {
public:
     POINTER_DEFINITIONS(V3ChannelProvider);
    V3ChannelProvider();
    ~V3ChannelProvider();
    static epics::pvAccess::ChannelProvider::shared_pointer const & getChannelProvider();
    virtual epics::pvData::String getProviderName();
    virtual void destroy();
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelRequester::shared_pointer  const &channelRequester,
        short priority);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelRequester::shared_pointer  const &channelRequester,
        short priority,
        epics::pvData::String address);
    void removeChannel(ChannelListNode &node);
private:
    static epics::pvAccess::ChannelProvider::shared_pointer channelProviderPtr;
    epics::pvData::String providerName;
    ChannelList channelList;
};

class V3Channel : public virtual epics::pvAccess::Channel {
public:
    POINTER_DEFINITIONS(V3Channel);
    V3Channel(
        V3ChannelProvider::shared_pointer const & channelProvider,
        epics::pvAccess::ChannelRequester::shared_pointer const & requester,
        epics::pvData::String name,
        std::auto_ptr<DbAddr> addr

        );
    ~V3Channel();
    ChannelListNode& init();
    virtual void destroy();
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual epics::pvAccess::ChannelProvider::shared_pointer const & getProvider();
    virtual epics::pvData::String getRemoteAddress();
    virtual epics::pvAccess::Channel::ConnectionState getConnectionState();
    virtual epics::pvData::String getChannelName();
    virtual epics::pvAccess::ChannelRequester::shared_pointer const & getChannelRequester();
    virtual bool isConnected();
    virtual void getField(
        epics::pvAccess::GetFieldRequester::shared_pointer const &requester,
        epics::pvData::String subField);
    virtual epics::pvAccess::AccessRights getAccessRights(
        epics::pvData::PVField::shared_pointer const &pvField);
    virtual epics::pvAccess::ChannelProcess::shared_pointer createChannelProcess(
        epics::pvAccess::ChannelProcessRequester::shared_pointer const &channelProcessRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelGet::shared_pointer createChannelGet(
        epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelPut::shared_pointer createChannelPut(
        epics::pvAccess::ChannelPutRequester::shared_pointer const &channelPutRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelPutGet::shared_pointer createChannelPutGet(
        epics::pvAccess::ChannelPutGetRequester::shared_pointer const &channelPutGetRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelRPC::shared_pointer createChannelRPC(
        epics::pvAccess::ChannelRPCRequester::shared_pointer const &channelRPCRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvData::Monitor::shared_pointer createMonitor(
        epics::pvData::MonitorRequester::shared_pointer const &monitorRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual epics::pvAccess::ChannelArray::shared_pointer createChannelArray(
        epics::pvAccess::ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        epics::pvData::PVStructure::shared_pointer const &pvRequest);
    virtual void printInfo();
    virtual void printInfo(epics::pvData::StringBuilder out);
    // following only called by other V3ChannelXXX coxde
    void removeChannelProcess(ChannelProcessListNode &);
    void removeChannelGet(ChannelGetListNode &);
    void removeChannelPut(ChannelPutListNode &);
    void removeChannelMonitor(ChannelMonitorListNode &);
    void removeChannelArray(ChannelArrayListNode &);
private:
    V3ChannelProvider::shared_pointer  provider;
    epics::pvAccess::ChannelRequester::shared_pointer requester;
    epics::pvData::String name;
    std::auto_ptr<DbAddr> dbAddr;
    epics::pvData::FieldConstPtr recordField; 
    ChannelProcessList channelProcessList;
    ChannelGetList channelGetList;
    ChannelPutList channelPutList;
    ChannelMonitorList channelMonitorList;
    ChannelArrayList channelArrayList;
    ChannelListNode channelListNode;
    V3Channel::shared_pointer v3ChannelPtr;
};

class V3ChannelProcess : public virtual epics::pvAccess::ChannelProcess {
public:
    POINTER_DEFINITIONS(V3ChannelProcess);
    V3ChannelProcess(
        V3Channel::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelProcess();
    ChannelProcessListNode * init();
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void process(bool lastRequest);
private:
    static void notifyCallback(struct putNotify *);
    V3Channel::shared_pointer v3Channel;
    epics::pvAccess::ChannelProcessRequester::shared_pointer channelProcessRequester;
    DbAddr &dbAddr;
    ChannelProcessListNode processListNode;
    std::auto_ptr<struct putNotify> pNotify;
    std::auto_ptr<DbAddr> notifyAddr;
    epics::pvData::Event event;
    V3ChannelProcess::shared_pointer v3ChannelProcessPtr;
};

class V3ChannelGet : public virtual epics::pvAccess::ChannelGet {
public:
    POINTER_DEFINITIONS(V3ChannelGet);
    V3ChannelGet(
        V3Channel::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelGetRequester::shared_pointer const &channelGetRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelGet();
    ChannelGetListNode * init(epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void get(bool lastRequest);
private:
    static void notifyCallback(struct putNotify *);
    V3Channel::shared_pointer v3Channel;
    epics::pvAccess::ChannelGetRequester::shared_pointer channelGetRequester;
    DbAddr &dbAddr;
    ChannelGetListNode getListNode;
    bool process;
    bool firstTime;
    int propertyMask;
    epics::pvData::PVStructure::shared_pointer pvStructure;
    epics::pvData::BitSet::shared_pointer bitSet;
    std::auto_ptr<struct putNotify> pNotify;
    std::auto_ptr<DbAddr> notifyAddr;
    epics::pvData::Event event;
    V3ChannelGet::shared_pointer v3ChannelGetPtr;
};

class V3ChannelPut : public virtual epics::pvAccess::ChannelPut {
public:
    POINTER_DEFINITIONS(V3ChannelPut);
    V3ChannelPut(
        V3Channel::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelPutRequester::shared_pointer const &channelPutRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelPut();
    ChannelPutListNode * init(epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void put(bool lastRequest);
    virtual void get();
private:
    static void notifyCallback(struct putNotify *);
    V3Channel::shared_pointer v3Channel;
    epics::pvAccess::ChannelPutRequester::shared_pointer channelPutRequester;
    DbAddr &dbAddr;
    ChannelPutListNode putListNode;
    int propertyMask;
    bool process;
    bool firstTime;
    epics::pvData::PVStructure::shared_pointer pvStructure;
    epics::pvData::BitSet::shared_pointer bitSet;
    std::auto_ptr<struct putNotify> pNotify;
    std::auto_ptr<DbAddr> notifyAddr;
    epics::pvData::Event event;
    V3ChannelPut::shared_pointer v3ChannelPutPtr;
};

class V3ChannelMonitor
: public virtual epics::pvData::Monitor,
  public virtual CAV3MonitorRequester
{
public:
    POINTER_DEFINITIONS(V3ChannelMonitor);
    V3ChannelMonitor(
        V3Channel::shared_pointer const & v3Channel,
        epics::pvData::MonitorRequester::shared_pointer const & monitorRequester,
        DbAddr &dbAddr
    );
    virtual ~V3ChannelMonitor();
    ChannelMonitorListNode * init(
        epics::pvData::PVStructure::shared_pointer const &  pvRequest);
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
private:
    V3Channel::shared_pointer v3Channel;
    epics::pvData::MonitorRequester::shared_pointer  monitorRequester;
    DbAddr &dbAddr;
    ChannelMonitorListNode monitorListNode;
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
    V3ChannelMonitor::shared_pointer v3ChannelMonitorPtr;
};

class V3ChannelArray : public epics::pvAccess::ChannelArray {
public:
    POINTER_DEFINITIONS(V3ChannelArray);
    V3ChannelArray(
        V3Channel::shared_pointer const & v3Channel,
        epics::pvAccess::ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        DbAddr &dbAddr);
    virtual ~V3ChannelArray();
    ChannelArrayListNode * init(epics::pvData::PVStructure::shared_pointer const & pvRequest);
    virtual void destroy();
    virtual void putArray(bool lastRequest, int offset, int count);
    virtual void getArray(bool lastRequest, int offset, int count);
    virtual void setLength(bool lastRequest, int length, int capacity);
private:
    V3Channel::shared_pointer v3Channel;
    epics::pvAccess::ChannelArrayRequester::shared_pointer channelArrayRequester;
    DbAddr &dbAddr;
    ChannelArrayListNode arrayListNode;
    epics::pvData::PVScalarArray::shared_pointer pvScalarArray;
    V3ChannelArray::shared_pointer v3ChannelArrayPtr;
};

}}

#endif  /* V3CHANNEL_H */
