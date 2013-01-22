/* multiValueChannel.cpp */
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

#include <pv/channelProviderLocal.h>
#include <pv/service.h>
#include <pv/serverContext.h>

#include "multiValue.h"


namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;

static FieldCreatePtr fieldCreate = getFieldCreate();
static StandardFieldPtr standardField = getStandardField();
static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static StandardPVFieldPtr standardPVField = getStandardPVField();

MultiValueChannel::MultiValueChannel(
    MultiValueChannelProviderPtr const & channelProvider,
    ChannelRequester::shared_pointer const & requester)
: ChannelBase(channelProvider,requester,channelProvider->channelName),
  multiValueProvider(channelProvider),
  requester(requester),
  arrayValueChannel(new ValueChannelPtrArray())
{}

bool MultiValueChannel::create()
{
    size_t n = multiValueProvider->fieldNames->size();
    arrayValueChannel->reserve(n);
    for(size_t i=0; i<n; i++) {
        ValueChannelPtr valueChannel(
            new ValueChannel(
                 requester,
                 multiValueProvider->valueChannelProvider,
                 (*multiValueProvider->valueChannelNames)[i]));
        arrayValueChannel->push_back(valueChannel);
    }
    for(size_t i=0; i<n; i++) {
        (*arrayValueChannel)[i]->connect();
    }
    bool allOK = true;
    for(size_t i=0; i<n; i++) {
        Status status = (*arrayValueChannel)[i]->waitConnect();
        if(!status.isOK()) {
             allOK = false;
        }
    }
    if(!allOK) {
        destroy();
        return false;
    }
    FieldConstPtrArray fields;
    fields.reserve(n+2);
    StringArray fieldNames;
    fieldNames.reserve(n+2);
    fields.push_back(standardField->alarm());
    fieldNames.push_back("alarm");
    fields.push_back(standardField->timeStamp());
    fieldNames.push_back("timeStamp");
    for(size_t i=0; i<n; i++) {
        FieldConstPtr valueField =
            (*arrayValueChannel)[i]->getValue()->getField();
        fields.push_back(valueField);
        fieldNames.push_back((*multiValueProvider->fieldNames)[i]);
    }
    structure = fieldCreate->createStructure(fieldNames,fields);
    return allOK;
}

void MultiValueChannel::destroy()
{
    size_t n = arrayValueChannel->size();
    for(size_t i=0; i<n; i++) {
       (*arrayValueChannel)[i]->destroy();
    }
    arrayValueChannel.reset();
}

void MultiValueChannel::getField(
    GetFieldRequester::shared_pointer const &requester,
    String const &subField)
{
    // for now just return structure
    requester->getDone(Status::Ok,structure);
}

ChannelGet::shared_pointer MultiValueChannel::createChannelGet(
    ChannelGetRequester::shared_pointer const &channelGetRequester,
    PVStructure::shared_pointer const &pvRequest)
{
    MultiValueChannelPtr xxx =
        static_pointer_cast<MultiValueChannel>(getPtrSelf());
    MultiValueChannelGetPtr channelGet(
        new MultiValueChannelGet(xxx,channelGetRequester));
    bool result = channelGet->init(pvRequest);
    if(!result) channelGet.reset();
    return channelGet;
}

}}

