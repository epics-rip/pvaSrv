/* v3Array.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#ifndef V3ARRAY_H
#define V3ARRAY_H
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>

#include <dbAccess.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>

namespace epics { namespace pvIOC { 

class V3ValueArrayCreate {
public:
    POINTER_DEFINITIONS(V3ValueArrayCreate);
    epics::pvData::PVByteArrayPtr createByteArray(
        epics::pvData::PVStructurePtr const & parent,
        epics::pvData::ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVShortArrayPtr createShortArray(
        epics::pvData::PVStructurePtr const & parent,
        epics::pvData::ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVIntArrayPtr createIntArray(
        epics::pvData::PVStructurePtr const & parent,
        epics::pvData::ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVFloatArrayPtr createFloatArray(
        epics::pvData::PVStructurePtr const & parent,
        epics::pvData::ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVDoubleArrayPtr createDoubleArray(
        epics::pvData::PVStructurePtr const & parent,
        epics::pvData::ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr,bool shareData);
    // Note that V3StringArray can not share
    epics::pvData::PVStringArrayPtr createStringArray(
        epics::pvData::PVStructurePtr const & parent,
        epics::pvData::ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr);
};

typedef std::tr1::shared_ptr<V3ValueArrayCreate> V3ValueArrayCreatePtr;

extern V3ValueArrayCreatePtr getV3ValueArrayCreate();

}}

#endif  /* V3ARRAY_H */
