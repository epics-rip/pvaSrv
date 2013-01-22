/* v3ChannelDebug.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#ifndef V3CHANNELDEBUG_H
#define V3CHANNELDEBUG_H

namespace epics { namespace pvaSrv { 

class V3ChannelDebug {
public:
    static void setLevel(int level);
    static int getLevel();
};

}}

#endif  /* V3CHANNELDEBUG_H */
