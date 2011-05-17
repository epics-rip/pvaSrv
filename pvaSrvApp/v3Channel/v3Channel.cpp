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
    V3ChannelProvider::shared_pointer const & provider,
        epics::pvAccess::ChannelRequester::shared_pointer const & requester,
        epics::pvData::String name,
        std::auto_ptr<DbAddr> dbAddr
    )
:   provider(provider),
    requester(requester),
    name(name),
    dbAddr(dbAddr),
    recordField(std::tr1::shared_ptr<const Field>()),
    channelProcessList(),
    channelGetList(),
    channelPutList(),
    channelMonitorList(),
    channelArrayList(),
    channelListNode(*this),
    v3ChannelPtr(V3Channel::shared_pointer(this))
{
}

ChannelListNode &V3Channel::init()
{
    ScalarType scalarType = pvBoolean;
    DbAddr *paddr = dbAddr.get();
    switch(paddr->field_type) {
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
        bool isArray = (paddr->no_elements>1) ? true : false;
        if(isArray) {
            recordField = standardField->scalarArrayValue(scalarType,
                String("value,timeStamp,alarm,display"));
        } else {
            recordField = standardField->scalarValue(scalarType,
                String("value,timeStamp,alarm,display,control"));
        }
    }
    return channelListNode;
}

V3Channel::~V3Channel()
{
}

void V3Channel::destroy()
{
printf("V3Channel::destroy\n");
    while(true) {
        ChannelProcessListNode *node = channelProcessList.getHead();
        if(node==0) break;
        V3ChannelProcess &channelProcess = node->getObject();
        channelProcess.destroy();
    }
    while(true) {
        ChannelGetListNode *node = channelGetList.getHead();
        if(node==0) break;
        V3ChannelGet &channelGet = node->getObject();
        channelGet.destroy();
    }
    while(true) {
        ChannelPutListNode *node = channelPutList.getHead();
        if(node==0) break;
        V3ChannelPut &channelPut = node->getObject();
        channelPut.destroy();
    }
    while(true) {
        ChannelMonitorListNode *node = channelMonitorList.getHead();
        if(node==0) break;
        V3ChannelMonitor &channelMonitor = node->getObject();
        channelMonitor.destroy();
    }
    while(true) {
        ChannelArrayListNode *node = channelArrayList.getHead();
        if(node==0) break;
        V3ChannelArray &channelArray = node->getObject();
        channelArray.destroy();
    }
    provider->removeChannel(channelListNode);
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
    return requester->getRequesterName();
}

void V3Channel::message(
        String message,
        MessageType messageType)
{
    requester->message(message,messageType);
}

epics::pvAccess::ChannelProvider::shared_pointer const & V3Channel::getProvider()
{
    ChannelProvider::shared_pointer const & xxx = 
        static_cast<ChannelProvider::shared_pointer const &>(provider);
    return xxx;
}

String V3Channel::getRemoteAddress()
{
    return String("local");
}

Channel::ConnectionState V3Channel::getConnectionState()
{
    return Channel::CONNECTED;
}

String V3Channel::getChannelName()
{
    return name;
}

ChannelRequester::shared_pointer const & V3Channel::getChannelRequester()
{
    return requester;
}

bool V3Channel::isConnected()
{
    return true;
}

void V3Channel::getField(GetFieldRequester::shared_pointer const &requester,
        String subField)
{
    if(recordField!=0) {
        requester->getDone(Status::OK,recordField);
        return;
    }
    Status status(Status::STATUSTYPE_ERROR,
        String("client asked for illegal V3 field"));
    requester->getDone(status,FieldConstPtr());
}

AccessRights V3Channel::getAccessRights(
        PVField::shared_pointer const &pvField)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelProcess::shared_pointer V3Channel::createChannelProcess(
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructure::shared_pointer const & pvRequest)
{
    V3ChannelProcess::shared_pointer v3ChannelProcess =
        V3ChannelProcess::shared_pointer(new V3ChannelProcess(
           v3ChannelPtr,channelProcessRequester,*(dbAddr.get())));
    ChannelProcessListNode * node = v3ChannelProcess->init();
    if(node!=0) channelProcessList.addTail(*node);
    return v3ChannelProcess;
}

ChannelGet::shared_pointer V3Channel::createChannelGet(
        ChannelGetRequester::shared_pointer const &channelGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelGet::shared_pointer v3ChannelGet =
        V3ChannelGet::shared_pointer(new V3ChannelGet(
            v3ChannelPtr,channelGetRequester,*(dbAddr.get())));
    ChannelGetListNode * node = v3ChannelGet->init(pvRequest);
    if(node!=0) channelGetList.addTail(*node);
    return v3ChannelGet;
}

ChannelPut::shared_pointer V3Channel::createChannelPut(
        ChannelPutRequester::shared_pointer const &channelPutRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelPut::shared_pointer v3ChannelPut =
       V3ChannelPut::shared_pointer(new V3ChannelPut(
        v3ChannelPtr,channelPutRequester,*(dbAddr.get())));
    ChannelPutListNode * node = v3ChannelPut->init(pvRequest);
    if(node!=0) channelPutList.addTail(*node);
    return v3ChannelPut;
}

ChannelPutGet::shared_pointer V3Channel::createChannelPutGet(
        ChannelPutGetRequester::shared_pointer const &channelPutGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelPutGet not supported for V3 Records"));
    channelPutGetRequester->channelPutGetConnect(
        status,
        ChannelPutGet::shared_pointer(),
        PVStructure::shared_pointer(),
        PVStructure::shared_pointer());
    return ChannelPutGet::shared_pointer();
}

ChannelRPC::shared_pointer V3Channel::createChannelRPC(
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        PVStructure::shared_pointer const & pvRequest)
{
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelRPC not supported for V3 Records"));
    channelRPCRequester->channelRPCConnect(status,
     ChannelRPC::shared_pointer(),
     PVStructure::shared_pointer(),
     BitSet::shared_pointer());
    return ChannelRPC::shared_pointer();
}

Monitor::shared_pointer V3Channel::createMonitor(
        MonitorRequester::shared_pointer const &monitorRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelMonitor::shared_pointer v3ChannelMonitor =
        V3ChannelMonitor::shared_pointer(
            new V3ChannelMonitor(
            v3ChannelPtr,monitorRequester,*(dbAddr.get())));
    ChannelMonitorListNode * node = v3ChannelMonitor->init(pvRequest);
    if(node!=0) channelMonitorList.addTail(*node);
    return v3ChannelMonitor;
}

ChannelArray::shared_pointer V3Channel::createChannelArray(
        ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelArray::shared_pointer v3ChannelArray =
        V3ChannelArray::shared_pointer(
            new V3ChannelArray(
            v3ChannelPtr,channelArrayRequester,*(dbAddr.get())));
    ChannelArrayListNode * node = v3ChannelArray->init(pvRequest);
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
