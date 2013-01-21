TOP = ..
include $(TOP)/configure/CONFIG

LIBRARY_IOC = pvaSrv

pvaSrv_LIBS += pvIOC pvAccess pvData
pvaSrv_LIBS += $(EPICS_BASE_IOC_LIBS)

SRC_DIRS += ../v3Channel
DBD += v3Channel.dbd
INC += v3Channel.h
INC += v3ChannelDebug.h
INC += v3CAMonitor.h
LIBSRCS += v3Util.cpp
LIBSRCS += CAV3Context.cpp
LIBSRCS += v3Array.cpp
LIBSRCS += v3CAMonitor.cpp
LIBSRCS += v3Channel.cpp
LIBSRCS += v3ChannelProvider.cpp
LIBSRCS += v3ChannelProcess.cpp
LIBSRCS += v3ChannelGet.cpp
LIBSRCS += v3ChannelMultiGet.cpp
LIBSRCS += v3ChannelPut.cpp
LIBSRCS += v3ChannelMultiPut.cpp
LIBSRCS += v3ChannelArray.cpp
LIBSRCS += v3ChannelRegister.cpp
LIBSRCS += v3ChannelMonitor.cpp

SRC_DIRS += ../multiValue
DBD += createMultiValue.dbd
INC += multiValue.h
INC += valueChannel.h
LIBSRCS += valueChannel.cpp
LIBSRCS += createMultiValueRegister.cpp
LIBSRCS += multiValueChannelProvider.cpp
LIBSRCS += multiValueChannel.cpp
LIBSRCS += multiValueChannelGet.cpp

include $(TOP)/configure/RULES
