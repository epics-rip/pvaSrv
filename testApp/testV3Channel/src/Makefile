TOP=../../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# Build an IOC support library
#

DBDINC += byteRecord
DBDINC += shortRecord
DBDINC += floatRecord

DBD += testV3Channel.dbd

LIBRARY_IOC += testV3ChannelSupport
testV3ChannelSupport_SRCS += byteRecord.c
testV3ChannelSupport_SRCS += shortRecord.c
testV3ChannelSupport_SRCS += floatRecord.c
testV3ChannelSupport_SRCS += testV3Channel.cpp
testV3ChannelSupport_LIBS += pvData
testV3ChannelSupport_LIBS += pvAccess
testV3ChannelSupport_LIBS += pvIOC
testV3ChannelSupport_LIBS += nt
testV3ChannelSupport_LIBS += exampleServiceIOC
testV3ChannelSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# build an ioc application

PROD_IOC += testV3Channel


# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
testV3Channel_SRCS += testV3Channel_registerRecordDeviceDriver.cpp
testV3Channel_SRCS_DEFAULT += testV3ChannelMain.cpp
testV3Channel_SRCS_vxWorks += -nil-


# The following adds support from base/src/vxWorks
testV3Channel_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

testV3Channel_LIBS += nt pvIOC pvData pvAccess
testV3Channel_LIBS += exampleServiceIOC
testV3Channel_LIBS += testV3ChannelSupport
testV3Channel_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

