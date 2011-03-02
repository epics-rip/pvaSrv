/* v3Channel.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#ifndef V3CHANNEL_H
#define V3CHANNEL_H
#include <string>
#include <stdexcept>
#include <memory>

#include <pvIntrospect.h>
#include <pvData.h>
#include <noDefaultMethods.h>
#include <pvEnumerated.h>
#include <thread.h>
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

class V3ChannelProvider : public epics::pvAccess::ChannelProvider {
public:
    V3ChannelProvider();
    ~V3ChannelProvider();
    virtual epics::pvData::String getProviderName();
    virtual void destroy();
    virtual epics::pvAccess::ChannelFind* channelFind(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelFindRequester *channelFindRequester);
    virtual epics::pvAccess::Channel* createChannel(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelRequester *channelRequester,
        short priority);
    virtual epics::pvAccess::Channel* createChannel(
        epics::pvData::String channelName,
        epics::pvAccess::ChannelRequester *channelRequester,
        short priority,
        epics::pvData::String address);
private:
    // TBD
};


class V3Channel : public epics::pvAccess::Channel {
public:
    V3Channel(epics::pvAccess::ChannelProvider &provider,
        epics::pvAccess::ChannelRequester &requester,
        epics::pvData::String name,
        epics::pvData::String remoteAddress);
    virtual void destroy();
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String message,
        epics::pvData::MessageType messageType);
    virtual epics::pvAccess::ChannelProvider* getProvider();
    virtual epics::pvData::String getRemoteAddress();
    virtual epics::pvAccess::Channel::ConnectionState getConnectionState();
    virtual epics::pvData::String getChannelName();
    virtual epics::pvAccess::ChannelRequester* getChannelRequester();
    virtual bool isConnected();
    virtual void getField(epics::pvAccess::GetFieldRequester *requester,
        epics::pvData::String subField);
    virtual epics::pvAccess::AccessRights getAccessRights(
        epics::pvData::PVField *pvField);
    virtual epics::pvAccess::ChannelProcess* createChannelProcess(
        epics::pvAccess::ChannelProcessRequester *channelProcessRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelGet* createChannelGet(
        epics::pvAccess::ChannelGetRequester *channelGetRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelPut* createChannelPut(
        epics::pvAccess::ChannelPutRequester *channelPutRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelPutGet* createChannelPutGet(
        epics::pvAccess::ChannelPutGetRequester *channelPutGetRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelRPC* createChannelRPC(
        epics::pvAccess::ChannelRPCRequester *channelRPCRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvData::Monitor* createMonitor(
        epics::pvData::MonitorRequester *monitorRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual epics::pvAccess::ChannelArray* createChannelArray(
        epics::pvAccess::ChannelArrayRequester *channelArrayRequester,
        epics::pvData::PVStructure *pvRequest);
    virtual void printInfo();
    virtual void printInfo(epics::pvData::StringBuilder out);
private:
    ~V3Channel();
    // TBD
};

class V3ChannelProcess : public epics::pvAccess::ChannelProcess {
public:
    virtual void destroy();
private:
    V3ChannelProcess(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelProcessRequester *channelProcessRequester,
        epics::pvData::PVStructure *pvRequest);
    ~V3ChannelProcess();
     friend class V3Channel;
   //TBD
};

class V3ChannelGet : public epics::pvAccess::ChannelGet {
public:
    virtual void destroy();
    virtual void get(bool lastRequest);
private:
    V3ChannelGet(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelGetRequester *channelGetRequester,
        epics::pvData::PVStructure *pvRequest);
    ~V3ChannelGet();
     friend class V3Channel;
   //TBD
};

class V3ChannelPut : public epics::pvAccess::ChannelPut {
public:
    virtual void destroy();
    virtual void put(bool lastRequest);
    virtual void get();
private:
    V3ChannelPut(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelPutRequester *channelPutRequester,
        epics::pvData::PVStructure *pvRequest);
    ~V3ChannelPut();
     friend class V3Channel;
   //TBD
};

class V3ChannelPutGet : public epics::pvAccess::ChannelPutGet {
public:
    virtual void destroy();
    virtual void putGet(bool lastRequest);
    virtual void getPut();
    virtual void getGet();
private:
    V3ChannelPutGet(
        V3Channel &v3Channel,
        epics::pvAccess::ChannelPutGetRequester *channelPutGetRequester,
        epics::pvData::PVStructure *pvRequest);
    ~V3ChannelPutGet();
     friend class V3Channel;
   //TBD
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
