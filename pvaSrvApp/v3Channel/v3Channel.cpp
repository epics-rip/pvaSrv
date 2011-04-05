/* v3Channel.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#include <string>
#include <stdexcept>
#include <memory>

#include "dbFldTypes.h"
#include "dbDefs.h"

#include <pvIntrospect.h>
#include <pvData.h>
#include <noDefaultMethods.h>

#include "support.h"
#include "pvDatabase.h"
#include "standardField.h"
#include "v3Channel.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

V3Channel::V3Channel(
    V3ChannelProvider &provider,
    ChannelRequester &requester,
    String name,
    std::auto_ptr<DbAddr> addr)
:   provider(provider),
    requester(requester),name(name),
    addr(addr),
    recordField(0),
    channelProcessList(),
    channelGetList(),
    channelPutList(),
    channelMonitorList(),
    channelArrayList(),
    context(0)
{
}

void V3Channel::init()
{
    ScalarType scalarType = pvBoolean;
    DbAddr *dbAddr = addr.get();
    switch(dbAddr->field_type) {
    case DBF_CHAR:
        case DBF_UCHAR:
            scalarType = pvByte; break;
        case DBF_SHORT:
        case DBF_USHORT:
            scalarType = pvShort; break;
        case DBF_LONG:
        case DBF_ULONG:
            scalarType = pvInt; break;
        case DBF_FLOAT:
            scalarType = pvFloat; break;
        case DBF_DOUBLE:
            scalarType = pvDouble; break;
        case DBF_STRING:
            scalarType = pvString; break;
        default:
          break;
    }
    if(scalarType!=pvBoolean) {
        StandardField *standardField = getStandardField();
        bool isArray = (dbAddr->no_elements>1) ? true : false;
        if(isArray) {
            recordField = standardField->scalarArrayValue(scalarType,
                String("value,timeStamp,alarm,display"));
        } else {
            recordField = standardField->scalarValue(scalarType,
                String("value,timeStamp,alarm,display,control"));
        }
    }
}

V3Channel::~V3Channel()
{
    delete context;
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
    return requester.getRequesterName();
}

void V3Channel::message(
        String message,
        MessageType messageType)
{
    requester.message(message,messageType);
}

ChannelProvider *V3Channel::getProvider()
{
    return &provider;
}

String V3Channel::getRemoteAddress()
{
    return provider.getProviderName();
}

Channel::ConnectionState V3Channel::getConnectionState()
{
    return Channel::CONNECTED;
}

String V3Channel::getChannelName()
{
    return name;
}

ChannelRequester *V3Channel::getChannelRequester()
{
    return &requester;
}

bool V3Channel::isConnected()
{
    return true;
}

void V3Channel::getField(GetFieldRequester *requester,
        String subField)
{
    if(recordField!=0) {
        requester->getDone(Status::OK,recordField);
        return;
    }
    Status status(Status::STATUSTYPE_ERROR,
        String("client asked for illegal V3 field"));
    requester->getDone(status,0);
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
    V3ChannelProcess *v3ChannelProcess = new V3ChannelProcess(
        *this,*channelProcessRequester,*(addr.get()));
    ChannelProcessListNode * node = v3ChannelProcess->init();
    if(node!=0) channelProcessList.addTail(*node);
    return v3ChannelProcess;
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
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelPutGet not supported for V3 Records"));
    channelPutGetRequester->channelPutGetConnect(status,0,0,0);
    return 0;
}

ChannelRPC *V3Channel::createChannelRPC(
        ChannelRPCRequester *channelRPCRequester,
        PVStructure *pvRequest)
{
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelRPC not supported for V3 Records"));
    channelRPCRequester->channelRPCConnect(status,0,0,0);
    return 0;
}

Monitor *V3Channel::createMonitor(
        MonitorRequester *monitorRequester,
        PVStructure *pvRequest)
{
    V3ChannelMonitor *v3ChannelMonitor = new V3ChannelMonitor(
        *this,*monitorRequester,*(addr.get()));
    if(context==0) context = new CAV3Context(requester);
    ChannelMonitorListNode * node = v3ChannelMonitor->init(
        *pvRequest,*context);
    if(node!=0) channelMonitorList.addTail(*node);
    return v3ChannelMonitor;
}

ChannelArray *V3Channel::createChannelArray(
        ChannelArrayRequester *channelArrayRequester,
        PVStructure *pvRequest)
{
    V3ChannelArray *v3ChannelArray = new V3ChannelArray(
        *this,*channelArrayRequester,*(addr.get()));
    ChannelArrayListNode * node = v3ChannelArray->init(*pvRequest);
    if(node!=0) channelArrayList.addTail(*node);
    return v3ChannelArray;
}

void V3Channel::printInfo()
{
    printf("V3Channel provides access to V3 records\n");
}

void V3Channel::printInfo(StringBuilder out)
{
    *out += "V3Channel provides access to V3 records";
}



}}
