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
    PVServiceBaseProvider::shared_pointer const & provider,
        ChannelRequester::shared_pointer const & requester,
        String name,
        std::auto_ptr<DbAddr> dbAddr
)
:  PVServiceBase(provider,requester,name),
    dbAddr(dbAddr),
    recordField()
{
printf("V3Channel::V3Channel\n");
}

void V3Channel::init()
{
    // this requires valid existance of V3Channel::shared_pointer instance
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
}

V3Channel::~V3Channel()
{
printf("V3Channel::~V3Channel\n");
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

ChannelProcess::shared_pointer V3Channel::createChannelProcess(
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructure::shared_pointer const & pvRequest)
{
    V3ChannelProcess * v3ChannelProcess = new V3ChannelProcess(
           getPtrSelf(),channelProcessRequester,*(dbAddr.get()));
    ChannelProcess::shared_pointer channelProcess(v3ChannelProcess);
    v3ChannelProcess->init();
    addChannelProcess(*v3ChannelProcess);
    return channelProcess;
}

ChannelGet::shared_pointer V3Channel::createChannelGet(
        ChannelGetRequester::shared_pointer const &channelGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelGet * v3ChannelGet = new V3ChannelGet(
            getPtrSelf(),channelGetRequester,*(dbAddr.get()));
    ChannelGet::shared_pointer channelGet(v3ChannelGet);
    if(v3ChannelGet->init(pvRequest)) addChannelGet(*v3ChannelGet);
    return channelGet;
}

ChannelPut::shared_pointer V3Channel::createChannelPut(
        ChannelPutRequester::shared_pointer const &channelPutRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelPut * v3ChannelPut = new V3ChannelPut(
            getPtrSelf(),channelPutRequester,*(dbAddr.get()));
    ChannelPut::shared_pointer channelPut(v3ChannelPut);
    if(v3ChannelPut->init(pvRequest)) addChannelPut(*v3ChannelPut);
    return channelPut;
}


Monitor::shared_pointer V3Channel::createMonitor(
        MonitorRequester::shared_pointer const &monitorRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelMonitor *v3ChannelMonitor = new V3ChannelMonitor(
           getPtrSelf(),monitorRequester,*(dbAddr.get()));
    Monitor::shared_pointer channelMonitor(v3ChannelMonitor);
    if(v3ChannelMonitor->init(pvRequest)) addChannelMonitor(*v3ChannelMonitor);
    return channelMonitor;
}

ChannelArray::shared_pointer V3Channel::createChannelArray(
        ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelArray *v3ChannelArray = new V3ChannelArray(
        getPtrSelf(),channelArrayRequester,*(dbAddr.get()));
    ChannelArray::shared_pointer channelArray(v3ChannelArray);
    if(v3ChannelArray->init(pvRequest)) addChannelArray(*v3ChannelArray);
    return channelArray;
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
