# Makefile for the pvaSrv library

TOP = ..
include $(TOP)/configure/CONFIG

PVASRV_VERSION = 0.11.4

LIBRARY_IOC = pvaSrv

SHRLIB_VERSION = 0.11.0

PVASRV_SRC = $(TOP)/src

include $(PVASRV_SRC)/dbPv/Makefile
#include $(PVASRV_SRC)/dbGroup/Makefile

pvaSrv_LIBS += pvAccess pvData
pvaSrv_LIBS += $(EPICS_BASE_IOC_LIBS)

USR_CPPFLAGS = -DPVASRV_VERSION=\"$(PVASRV_VERSION)\"

include $(TOP)/configure/RULES
