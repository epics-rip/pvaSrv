# This is a Makefile fragment, see ../Makefile

PLACE=$(EPICS_VERSION).$(EPICS_REVISION)

ifeq ($(PLACE),3.16)
  PLACE=3.15
endif

SRC_DIRS += $(PVASRV_SRC)/dbPv/$(PLACE)

DBD += dbPv.dbd

INC += pv/dbPv.h
INC += pv/dbPvDebug.h
INC += pv/caMonitor.h
INC += pv/caSecurity.h

LIBSRCS += caContext.cpp
LIBSRCS += dbUtil.cpp
LIBSRCS += caMonitor.cpp
LIBSRCS += caSecurity.cpp
LIBSRCS += dbPv.cpp
LIBSRCS += dbPvProvider.cpp
LIBSRCS += dbPvProcess.cpp
LIBSRCS += dbPvGet.cpp
LIBSRCS += dbPvPut.cpp
LIBSRCS += dbPvArray.cpp
LIBSRCS += dbPvRegister.cpp
LIBSRCS += dbPvMonitor.cpp
