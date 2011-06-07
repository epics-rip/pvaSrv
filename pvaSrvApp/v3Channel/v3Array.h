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
    epics::pvData::PVByteArray *createByteArray(
        epics::pvData::PVStructure *parent,
        epics::pvData::ScalarArrayConstPtr scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVShortArray *createShortArray(
        epics::pvData::PVStructure *parent,
        epics::pvData::ScalarArrayConstPtr scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVIntArray *createIntArray(
        epics::pvData::PVStructure *parent,
        epics::pvData::ScalarArrayConstPtr scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVFloatArray *createFloatArray(
        epics::pvData::PVStructure *parent,
        epics::pvData::ScalarArrayConstPtr scalar,
        DbAddr &dbAddr,bool shareData);
    epics::pvData::PVDoubleArray *createDoubleArray(
        epics::pvData::PVStructure *parent,
        epics::pvData::ScalarArrayConstPtr scalar,
        DbAddr &dbAddr,bool shareData);
    // Note that V3StringArray can not share
    epics::pvData::PVStringArray *createStringArray(
        epics::pvData::PVStructure *parent,
        epics::pvData::ScalarArrayConstPtr scalar,
        DbAddr &dbAddr);
};

extern V3ValueArrayCreate *getV3ValueArrayCreate();

}}

#endif  /* V3ARRAY_H */
