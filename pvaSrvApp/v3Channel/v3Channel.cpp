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
{
}

V3Channel::~V3Channel()
{
}

void V3Channel::destroy()
{
    delete this;
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

void V3Channel::getField(epics::pvAccess::GetFieldRequester *requester,
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
        epics::pvAccess::ChannelProcessRequester *channelProcessRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelGet *V3Channel::createChannelGet(
        epics::pvAccess::ChannelGetRequester *channelGetRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelPut *V3Channel::createChannelPut(
        epics::pvAccess::ChannelPutRequester *channelPutRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelPutGet *V3Channel::createChannelPutGet(
        epics::pvAccess::ChannelPutGetRequester *channelPutGetRequester,
        PVStructure *pvRequest)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelRPC *V3Channel::createChannelRPC(
        epics::pvAccess::ChannelRPCRequester *channelRPCRequester,
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
        epics::pvAccess::ChannelArrayRequester *channelArrayRequester,
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
