# This is an example V3 IOC that includes the pvaSrv pvAccess server
# To add this functionality:
# 1) link against the pvData, pvAccess and pvIOC libraries
# 2) include v3Channel.dbd
# 3) call startV3Channel in st.cmd

TOP=../../..

include $(TOP)/configure/CONFIG

DBD += simpleV3Channel.dbd
PROD_IOC += simpleV3Channel

simpleV3Channel_SRCS += simpleV3Channel_registerRecordDeviceDriver.cpp
simpleV3Channel_SRCS_DEFAULT += simpleV3ChannelMain.cpp

simpleV3Channel_LIBS += pvaSrv pvIOC pvAccess pvData
simpleV3Channel_LIBS += $(EPICS_BASE_IOC_LIBS)

simpleV3Channel_SRCS_vxWorks += -nil-
simpleV3Channel_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

include $(TOP)/configure/RULES
