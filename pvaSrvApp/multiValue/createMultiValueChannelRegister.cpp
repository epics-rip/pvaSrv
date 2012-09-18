/*createMultiValueChannelRegister.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>

#include <iocsh.h>

#include <epicsExport.h>

#include <pv/createMultiValueChannel.h>

using namespace epics::pvData;
using namespace epics::pvIOC;


static const iocshArg multiChannelArg0 = { "configFileName",iocshArgString};
static const iocshArg *multiChannelArgs[] = {&multiChannelArg0};
static const iocshFuncDef multiChannelFuncDef = 
  {"createMultiValueChannel",1,multiChannelArgs};

extern "C" void configureMultiValueChannel(const iocshArgBuf *args)
{
    String fileName(args[0].sval);
printf("createMultiValueChannel not implemented\n");
printf("configFileName %s\n",fileName.c_str());
}


static void createMultiValueChannelRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&multiChannelFuncDef, configureMultiValueChannel);
    }
}


epicsExportRegistrar(createMultiValueChannelRegister);
