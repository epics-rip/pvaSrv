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

namespace epics { namespace pvIOC { 

class V3ChannelProvider;
class V3Channel;
class V3ChannelProcess;
class V3ChannelGet;
class V3ChannelPut;
class V3ChannelPutGet;
class V3ChannelMonitor;
class V3ChannelArray;
class V3ChannelRPC;

typedef struct dbAddr DbAddr;
typedef epics::pvData::LinkedListNode<V3Channel> ChannelListNode;
typedef epics::pvData::LinkedList<V3Channel> ChannelList;

class V3ChannelProvider : public epics::pvAccess::ChannelProvider {
public:
    static V3ChannelProvider &getChannelProvider();
    virtual epics::pvData::String getProviderName();
    virtual void destroy();
    virtual epics::pvAccess::ChannelFind *channelFind(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelFindRequester *channelFindRequester);
    virtual epics::pvAccess::Channel *createChannel(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelRequester *channelRequester,
        short priority);
    virtual epics::pvAccess::Channel *createChannel(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelRequester *channelRequester,
        short priority,
        epics::pvData::String address);
private:
    V3ChannelProvider();
    ~V3ChannelProvider();
    epics::pvData::String providerName;
    ChannelList channelList;
};

typedef epics::pvData::LinkedListNode<V3ChannelProcess> ChannelProcessListNode;
typedef epics::pvData::LinkedList<V3ChannelProcess> ChannelProcessList;
typedef epics::pvData::LinkedListNode<V3ChannelGet> ChannelGetListNode;
typedef epics::pvData::LinkedList<V3ChannelGet> ChannelGetList;
typedef epics::pvData::LinkedListNode<V3ChannelPut> ChannelPutListNode;
typedef epics::pvData::LinkedList<V3ChannelPut> ChannelPutList;
typedef epics::pvData::LinkedListNode<V3ChannelPutGet> ChannelPutGetListNode;
typedef epics::pvData::LinkedList<V3ChannelPutGet> ChannelPutGetList;
typedef epics::pvData::LinkedListNode<V3ChannelMonitor> ChannelMonitorListNode;
typedef epics::pvData::LinkedList<V3ChannelMonitor> ChannelMonitorList;
typedef epics::pvData::LinkedListNode<V3ChannelArray> ChannelArrayListNode;
typedef epics::pvData::LinkedList<V3ChannelArray> ChannelArrayList;

class V3Channel : public epics::pvAccess::Channel {
public:
    V3Channel(
        epics::pvAccess::ChannelRequester &requester,
        epics::pvData::String name,
        std::auto_ptr<DbAddr> addr
        );
    virtual void destroy();
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual epics::pvAccess::ChannelProvider *getProvider();
    virtual epics::pvData::String getRemoteAddress();
    virtual epics::pvAccess::Channel::ConnectionState getConnectionState();
    virtual epics::pvData::String getChannelName();
    virtual epics::pvAccess::ChannelRequester *getChannelRequester();
    virtual bool isConnected();
    virtual void getField(epics::pvAccess::GetFieldRequester *requester,
        epics::pvData::String subField);
    virtual epics::pvAccess::AccessRights getAccessRights(
        epics::pvData::PVField *pvField);
    virtual epics::pvAccess::ChannelProcess *createChannelProcess(
        epics::pvAccess::ChannelProcessRequester *channelProcessRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelGet *createChannelGet(
        epics::pvAccess::ChannelGetRequester *channelGetRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelPut *createChannelPut(
        epics::pvAccess::ChannelPutRequester *channelPutRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelPutGet *createChannelPutGet(
        epics::pvAccess::ChannelPutGetRequester *channelPutGetRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelRPC *createChannelRPC(
        epics::pvAccess::ChannelRPCRequester *channelRPCRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvData::Monitor *createMonitor(
        epics::pvData::MonitorRequester *monitorRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelArray *createChannelArray(
        epics::pvAccess::ChannelArrayRequester *channelArrayRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual void printInfo();
    virtual void printInfo(epics::pvData::StringBuilder out);
    // following only called by other V3ChannelXXX coxde
    void removeChannelProcess(ChannelProcessListNode &);
    void removeChannelGet(ChannelGetListNode &);
    void removeChannelPut(ChannelPutListNode &);
    void removeChannelPutGet(ChannelPutGetListNode &);
    void removeChannelMonitor(ChannelMonitorListNode &);
    void removeChannelArray(ChannelArrayListNode &);
private:
    ~V3Channel();
    epics::pvAccess::ChannelRequester &requester;
    epics::pvData::String name;
    std::auto_ptr<DbAddr> addr;
    ChannelProcessList channelProcessList;
    ChannelGetList channelGetList;
    ChannelPutList channelPutList;
    ChannelPutGetList channelPutGetList;
    ChannelMonitorList channelMonitorList;
    ChannelArrayList channelArrayList;
};

class V3ChannelProcess : public epics::pvAccess::ChannelProcess {
public:
    V3ChannelProcess(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelProcessRequester &channelProcessRequester,
        DbAddr &dbaddr);
    ~V3ChannelProcess();
    ChannelProcessListNode * init(epics::pvData::PVStructure & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void process(bool lastRequest);
private:
    V3Channel &v3Channel;
    epics::pvAccess::ChannelProcessRequester &channelProcessRequester;
    DbAddr &dbaddr;
    ChannelProcessListNode processListNode;
};

class V3ChannelGet : public epics::pvAccess::ChannelGet {
public:
    V3ChannelGet(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelGetRequester &channelGetRequester,
        DbAddr &dbaddr);
    virtual ~V3ChannelGet();
    ChannelGetListNode * init(epics::pvData::PVStructure & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void get(bool lastRequest);
private:
    V3Channel &v3Channel;
    epics::pvAccess::ChannelGetRequester &channelGetRequester;
    DbAddr &dbaddr;
    ChannelGetListNode getListNode;
    bool process;
    int whatMask;
    std::auto_ptr<epics::pvData::PVStructure> pvStructure;
    std::auto_ptr<epics::pvData::BitSet> bitSet;
};

class V3ChannelPut : public epics::pvAccess::ChannelPut {
public:
    V3ChannelPut(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelPutRequester &channelPutRequester,
        DbAddr &dbaddr);
    ~V3ChannelPut();
    ChannelPutListNode * init(epics::pvData::PVStructure & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void put(bool lastRequest);
    virtual void get();
private:
    static void notifyCallback(struct putNotify *);
    V3Channel &v3Channel;
    epics::pvAccess::ChannelPutRequester &channelPutRequester;
    DbAddr &dbaddr;
    ChannelPutListNode putListNode;
    bool process;
    int whatMask;
    std::auto_ptr<epics::pvData::PVStructure> pvStructure;
    std::auto_ptr<epics::pvData::BitSet> bitSet;
    std::auto_ptr<struct putNotify> pNotify;
    std::auto_ptr<DbAddr> notifyAddr;
    epics::pvData::Event event;
};

class V3ChannelPutGet : public epics::pvAccess::ChannelPutGet {
public:
    V3ChannelPutGet(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelPutGetRequester &channelPutGetRequester,
        DbAddr &dbaddr);
    ~V3ChannelPutGet();
    ChannelPutGetListNode * init(epics::pvData::PVStructure & pvRequest);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual void destroy();
    virtual void putGet(bool lastRequest);
    virtual void getPut();
    virtual void getGet();
private:
    V3Channel &v3Channel;
    epics::pvAccess::ChannelPutGetRequester &channelPutGetRequester;
    DbAddr &dbaddr;
    ChannelPutGetListNode putGetListNode;
    bool process;
    std::auto_ptr<epics::pvData::PVStructure> pvStructure;
    std::auto_ptr<epics::pvData::BitSet> bitSet;
};

class V3ChannelMonitor : public epics::pvData::Monitor {
public:
    virtual void destroy();
    // TBD
private:
    V3ChannelMonitor(
        V3Channel &v3Channel,
        epics::pvData::MonitorRequester *monitorRequester,
        epics::pvData::PVStructure *pvRequest);
    ~V3ChannelMonitor();
     friend class V3Channel;
   //TBD
};

class V3ChannelArray : public epics::pvAccess::ChannelArray {
public:
    virtual void destroy();
    virtual void putArray(bool lastRequest, int offset, int count);
    virtual void getArray(bool lastRequest, int offset, int count);
    virtual void setLength(bool lastRequest, int length, int capacity);
private:
    V3ChannelArray(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelArrayRequester *channelArrayRequester,
        epics::pvData::PVStructure *pvRequest);
    ~V3ChannelArray();
     friend class V3Channel;
   //TBD
};

class V3ChannelRPC : public epics::pvAccess::ChannelRPC {
public:
    virtual void destroy();
    virtual void request(bool lastRequest);
private:
    V3ChannelRPC(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelRPCRequester *channelRPCRequester,
        epics::pvData::PVStructure *pvRequest);
    ~V3ChannelRPC();
     friend class V3Channel;
   //TBD
};

   

}}

#endif  /* V3CHANNEL_H */
