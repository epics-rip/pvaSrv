TOP = ..
include $(TOP)/configure/CONFIG

PVASRV_VERSION = 0.9.1

LIBRARY_IOC = pvaSrv

pvaSrv_LIBS += pvIOC pvAccess pvData
pvaSrv_LIBS += $(EPICS_BASE_IOC_LIBS)

SRC_DIRS += ../dbPv
DBD += dbPv.dbd
INC += dbPv.h
INC += dbPvDebug.h
INC += caMonitor.h
LIBSRCS += dbUtil.cpp
LIBSRCS += caContext.cpp
LIBSRCS += dbArray.cpp
LIBSRCS += caMonitor.cpp
LIBSRCS += dbPv.cpp
LIBSRCS += dbPvProvider.cpp
LIBSRCS += dbPvProcess.cpp
LIBSRCS += dbPvGet.cpp
LIBSRCS += dbPvMultiGet.cpp
LIBSRCS += dbPvPut.cpp
LIBSRCS += dbPvMultiPut.cpp
LIBSRCS += dbPvArray.cpp
LIBSRCS += dbPvRegister.cpp
LIBSRCS += dbPvMonitor.cpp

#SRC_DIRS += ../dbGroup
#DBD += dbGroup.dbd
#INC += dbGroup.h
#INC += pvValue.h
#LIBSRCS += pvValue.cpp
#LIBSRCS += dbGroupRegister.cpp
#LIBSRCS += dbGroupProvider.cpp
#LIBSRCS += dbGroup.cpp
#LIBSRCS += dbGroupGet.cpp

USR_CPPFLAGS = -DPVASRV_VERSION=\"$(PVASRV_VERSION)\"

include $(TOP)/configure/RULES
