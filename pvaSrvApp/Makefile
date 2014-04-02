# Makefile for the pvaSrv library

TOP = ..
include $(TOP)/configure/CONFIG

PVASRV_VERSION = 0.9.1

LIBRARY_IOC = pvaSrv

PVASRVAPP = $(TOP)/pvaSrvApp

include $(PVASRVAPP)/dbPv/Makefile
# include $(PVASRVAPP)/dbGroup/Makefile

pvaSrv_LIBS += pvAccess pvData
pvaSrv_LIBS += $(EPICS_BASE_IOC_LIBS)

USR_CPPFLAGS = -DPVASRV_VERSION=\"$(PVASRV_VERSION)\"

include $(TOP)/configure/RULES
