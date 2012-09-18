/* createMultiValueChannel.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#ifndef CREATEMULTIVALUECHANNEL_H
#define CREATEMULTIVALUECHANNEL_H

#include <pv/pvData.h>

namespace epics { namespace pvIOC { 

    /**
     * Create a top level PVStructure and add to channelProviderLocal.
     * The structure will have an alarm field and a timeStamp
     * and a field for each v3Record.
     * @param requester The requester.
     * @param channelName The channelName.
     * @param fieldNames An array of fieldNames for the top level PVStructure.
     * @param v3RecordNames An array of V3 record names.
     */
extern bool createMultiValueChannel(
         epics::pvData::RequesterPtr const & requester,
         epics::pvData::String const & channelName,
         epics::pvData::StringArrayPtr const & fieldNames,
         epics::pvData::StringArrayPtr const & v3RecordNames);

}}

#endif  /* CREATEMULTIVALUECHANNEL_H */
