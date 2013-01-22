/* v3Channel.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#include <string>
#include <stdexcept>
#include <memory>

#include "dbFldTypes.h"
#include "dbDefs.h"

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/noDefaultMethods.h>

#include <pv/standardField.h>
#include <v3Channel.h>

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

static int v3ChannelDebugLevel = 0;
void V3ChannelDebug::setLevel(int level)
{
    v3ChannelDebugLevel = level;
}

int V3ChannelDebug::getLevel()
{
    return v3ChannelDebugLevel;
}

V3Channel::V3Channel(
    ChannelBaseProvider::shared_pointer const & provider,
        ChannelRequester::shared_pointer const & requester,
        String const &name,
        std::tr1::shared_ptr<DbAddr> dbAddr
)
:  ChannelBase(provider,requester,name),
    dbAddr(dbAddr),
    recordField()
{
//printf("V3Channel::V3Channel\n");
}

void V3Channel::init()
{
    // this requires valid existance of V3Channel::shared_pointer instance
    ScalarType scalarType = pvBoolean;
    DbAddr *paddr = dbAddr.get();
    switch(paddr->field_type) {
        case DBF_CHAR:
            scalarType = pvByte; break;
        case DBF_UCHAR:
            scalarType = pvUByte; break;
        case DBF_SHORT:
            scalarType = pvShort; break;
        case DBF_USHORT:
            scalarType = pvUShort; break;
        case DBF_LONG:
            scalarType = pvInt; break;
        case DBF_ULONG:
            scalarType = pvUInt; break;
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
        StandardFieldPtr standardField = getStandardField();
        bool isArray = (paddr->no_elements>1) ? true : false;
        if(isArray) {
            recordField = standardField->scalarArray(scalarType,
                String("value,timeStamp,alarm,display"));
        } else {
            recordField = standardField->scalar(scalarType,
                String("value,timeStamp,alarm,display,control"));
        }
    }
}

V3Channel::~V3Channel()
{
//printf("V3Channel::~V3Channel\n");
}

void V3Channel::getField(GetFieldRequester::shared_pointer const &requester,
        String const &subField)
{
    if(recordField!=0) {
        requester->getDone(Status::Ok,recordField);
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
    V3ChannelProcess::shared_pointer v3ChannelProcess(
          new V3ChannelProcess(getPtrSelf(),channelProcessRequester,*(dbAddr.get())));
    v3ChannelProcess->init();
    addChannelProcess(v3ChannelProcess);
    return v3ChannelProcess;
}

ChannelGet::shared_pointer V3Channel::createChannelGet(
        ChannelGetRequester::shared_pointer const &channelGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    PVFieldPtr pvField = pvRequest->getSubField("getField");
    if(pvField.get()!=NULL) {
        V3ChannelMultiGet::shared_pointer v3ChannelMultiGet(
            new V3ChannelMultiGet(
                getPtrSelf(),channelGetRequester,*(dbAddr.get())));
        if(v3ChannelMultiGet->init(pvRequest)) {
            addChannelGet(v3ChannelMultiGet);
        } else {
            Status createFailed(Status::STATUSTYPE_ERROR, "create v3ChannelMultiGet failed");
            channelGetRequester->channelGetConnect(
                createFailed,
                v3ChannelMultiGet,
                pvNullStructure,
                emptyBitSet);
        }
        return v3ChannelMultiGet;
    }
    V3ChannelGet::shared_pointer v3ChannelGet(
        new V3ChannelGet(getPtrSelf(),channelGetRequester,*(dbAddr.get())));
    if(v3ChannelGet->init(pvRequest)) {
        addChannelGet(v3ChannelGet);
    } else {
        Status createFailed(Status::STATUSTYPE_ERROR, "create v3ChannelGet failed");
        channelGetRequester->channelGetConnect(
            createFailed,
            v3ChannelGet,
            pvNullStructure,
            emptyBitSet);
    }
    return v3ChannelGet;
}

ChannelPut::shared_pointer V3Channel::createChannelPut(
        ChannelPutRequester::shared_pointer const &channelPutRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    PVFieldPtr pvField = pvRequest->getSubField("putField");
    if(pvField.get()!=NULL) {
        V3ChannelMultiPut::shared_pointer v3ChannelMultiPut(
            new V3ChannelMultiPut(
                getPtrSelf(),channelPutRequester,*(dbAddr.get())));
        if(v3ChannelMultiPut->init(pvRequest)) {
            addChannelPut(v3ChannelMultiPut);
        } else {
            Status createFailed(Status::STATUSTYPE_ERROR, "create v3ChannelMultiPut failed");
            channelPutRequester->channelPutConnect(
                createFailed,
                v3ChannelMultiPut,
                pvNullStructure,
                emptyBitSet);
        }
        return v3ChannelMultiPut;
    }
    V3ChannelPut::shared_pointer v3ChannelPut(
          new V3ChannelPut(getPtrSelf(),channelPutRequester,*(dbAddr.get())));
    if(v3ChannelPut->init(pvRequest)) {
        addChannelPut(v3ChannelPut);
    } else {
        Status createFailed(Status::STATUSTYPE_ERROR, "create v3ChannelPut failed");
        channelPutRequester->channelPutConnect(
            createFailed,
            v3ChannelPut,
            pvNullStructure,
            emptyBitSet);
    }
    return v3ChannelPut;
}


Monitor::shared_pointer V3Channel::createMonitor(
        MonitorRequester::shared_pointer const &monitorRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelMonitor::shared_pointer v3ChannelMonitor(
         new V3ChannelMonitor(getPtrSelf(),monitorRequester,*(dbAddr.get())));
    if(v3ChannelMonitor->init(pvRequest)) {
        addChannelMonitor(v3ChannelMonitor);
    } else {
        Status createFailed(Status::STATUSTYPE_ERROR, "create v3ChannelMonitor failed");
        StructureConstPtr xxx;
        monitorRequester->monitorConnect(
            createFailed,
            v3ChannelMonitor,
            xxx);
    }
    return v3ChannelMonitor;
}

ChannelArray::shared_pointer V3Channel::createChannelArray(
        ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    V3ChannelArray::shared_pointer v3ChannelArray(
         new V3ChannelArray(getPtrSelf(),channelArrayRequester,*(dbAddr.get())));
    if(v3ChannelArray->init(pvRequest)) {
        addChannelArray(v3ChannelArray);
    } else {
        Status createFailed(Status::STATUSTYPE_ERROR, "create v3ChannelArray failed");
        PVScalarArrayPtr xxx;
        channelArrayRequester->channelArrayConnect(
            createFailed,
            v3ChannelArray,
            xxx);
    }
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
