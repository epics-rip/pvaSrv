TOP = ..
include $(TOP)/configure/CONFIG

IOC = $(TOP)/pvIocApp/

LIBRARY_IOC += pvIOC
pvIOC_LIBS += $(EPICS_BASE_IOC_LIBS)
pvIoc_LIBS += Com pvData pvAccess

SRC_DIRS += $(IOC)/monitor
INC += monitorAlgorithm.h

SRC_DIRS += $(IOC)/database
INC += pvDatabase.h
LIBSRCS += pvDatabase.cpp
LIBSRCS += pvRecord.cpp
LIBSRCS += pvReplace.cpp

SRC_DIRS += $(IOC)/install
INC += install.h
LIBSRCS += install.cpp

SRC_DIRS += $(IOC)/support
INC += support.h
LIBSRCS += support.cpp

SRC_DIRS += $(IOC)/util
INC += util.h
LIBSRCS += util.cpp

SRC_DIRS += $(IOC)/v3Channel
INC += v3Channel.h
INC += v3CAMonitor.h
LIBSRCS += v3Array.cpp
LIBSRCS += v3Util.cpp
LIBSRCS += v3Channel.cpp
LIBSRCS += v3ChannelProvider.cpp
LIBSRCS += v3ChannelGet.cpp
LIBSRCS += v3ChannelMonitor.cpp
LIBSRCS += v3ChannelPut.cpp
LIBSRCS += v3ChannelProcess.cpp
LIBSRCS += v3ChannelArray.cpp
LIBSRCS += v3CAMonitor.cpp

SRC_DIRS += $(IOC)/register
DBD += pvIOC.dbd
LIBSRCS += startPVAccessServer.cpp
LIBSRCS += startV3Channel.cpp

include $(TOP)/configure/RULES

