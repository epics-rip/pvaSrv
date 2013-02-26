TOP=../../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#


DBD += testMultiValue.dbd


#=============================
# build an ioc application

PROD_IOC += testMultiValue


# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
testMultiValue_SRCS += testMultiValue_registerRecordDeviceDriver.cpp
testMultiValue_SRCS_DEFAULT += testMultiValueMain.cpp
testMultiValue_SRCS_vxWorks += -nil-


# The following adds support from base/src/vxWorks
testMultiValue_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

testMultiValue_LIBS += pvaSrv pvIOC pvAccess pvData pvMB
testMultiValue_LIBS += testV3ChannelSupport
testMultiValue_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

