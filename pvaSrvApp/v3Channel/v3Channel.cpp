/* v3Channel.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#include <string>
#include <stdexcept>
#include <memory>

#include <pvIntrospect.h>
#include <pvData.h>
#include <noDefaultMethods.h>

#include "support.h"
#include "pvDatabase.h"
#include "v3Channel.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3Channel::V3Channel(
    ChannelRequester &requester,
    String name,
    std::auto_ptr<DbAddr> addr)
: requester(requester),name(name),
    addr(addr),
    channelProcessList(),
    channelGetList(),
    channelPutList(),
    channelPutGetList(),
    channelMonitorList(),
    channelArrayList()
{
}

V3Channel::~V3Channel()
{
}

void V3Channel::destroy()
{
    delete this;
}

void V3Channel::removeChannelProcess(ChannelProcessListNode &node)
{
    if(node.isOnList()) channelProcessList.remove(node);
}

void V3Channel::removeChannelGet(ChannelGetListNode &node)
{
    if(node.isOnList()) channelGetList.remove(node);
}

void V3Channel::removeChannelPut(ChannelPutListNode &node)
{
    if(node.isOnList()) channelPutList.remove(node);
}

void V3Channel::removeChannelPutGet(ChannelPutGetListNode &node)
{
    if(node.isOnList()) channelPutGetList.remove(node);
}

void V3Channel::removeChannelMonitor(ChannelMonitorListNode &node)
{
    if(node.isOnList()) channelMonitorList.remove(node);
}

void V3Channel::removeChannelArray(ChannelArrayListNode &node)
{
    if(node.isOnList()) channelArrayList.remove(node);
}

String V3Channel::getRequesterName()
{
    throw std::logic_error(String("Not Implemented"));
}

void V3Channel::message(
        String message,
        MessageType messageType)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelProvider *V3Channel::getProvider()
{
    throw std::logic_error(String("Not Implemented"));
}

String V3Channel::getRemoteAddress()
{
    throw std::logic_error(String("Not Implemented"));
}

Channel::ConnectionState V3Channel::getConnectionState()
{
    throw std::logic_error(String("Not Implemented"));
}

String V3Channel::getChannelName()
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelRequester *V3Channel::getChannelRequester()
{
    throw std::logic_error(String("Not Implemented"));
}

bool V3Channel::isConnected()
{
    throw std::logic_error(String("Not Implemented"));
}

void V3Channel::getField(GetFieldRequester *requester,
        String subField)
{
    throw std::logic_error(String("Not Implemented"));
}

AccessRights V3Channel::getAccessRights(
        PVField *pvField)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelProcess *V3Channel::createChannelProcess(
        ChannelProcessRequester *channelProcessRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelGet *V3Channel::createChannelGet(
        ChannelGetRequester *channelGetRequester,
        PVStructure *pvRequest)
{
    V3ChannelGet *v3ChannelGet = new V3ChannelGet(
        *this,*channelGetRequester,*(addr.get()));
    ChannelGetListNode * node = v3ChannelGet->init(*pvRequest);
    if(node!=0) channelGetList.addTail(*node);
    return v3ChannelGet;
}

ChannelPut *V3Channel::createChannelPut(
        ChannelPutRequester *channelPutRequester,
        PVStructure *pvRequest)
{
    V3ChannelPut *v3ChannelPut = new V3ChannelPut(
        *this,*channelPutRequester,*(addr.get()));
    ChannelPutListNode * node = v3ChannelPut->init(*pvRequest);
    if(node!=0) channelPutList.addTail(*node);
    return v3ChannelPut;
}

ChannelPutGet *V3Channel::createChannelPutGet(
        ChannelPutGetRequester *channelPutGetRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelRPC *V3Channel::createChannelRPC(
        ChannelRPCRequester *channelRPCRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

Monitor *V3Channel::createMonitor(
        MonitorRequester *monitorRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelArray *V3Channel::createChannelArray(
        ChannelArrayRequester *channelArrayRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

void V3Channel::printInfo()
{
    throw std::logic_error(String("Not Implemented"));
}

void V3Channel::printInfo(StringBuilder out)
{
    throw std::logic_error(String("Not Implemented"));
}



}}
