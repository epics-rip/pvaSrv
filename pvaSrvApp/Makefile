TOP = ..
include $(TOP)/configure/CONFIG

IOC = $(TOP)/pvIocApp/

LIBRARY_IOC += pvIOC
pvIOC_LIBS += $(EPICS_BASE_IOC_LIBS)
pvIoc_LIBS += Com pvData pvAccess

SRC_DIRS += $(IOC)/service
DBD += pvServiceChannelRegister.dbd
INC += channelBase.h
INC += pvServiceProvider.h
INC += service.h
LIBSRCS += channelBase.cpp
LIBSRCS += channelBaseProvider.cpp
LIBSRCS += pvServiceProvider.cpp
LIBSRCS += pvServiceChannelRegister.cpp
LIBSRCS += pvServiceChannelCTX.cpp
LIBSRCS += serviceChannelRPC.cpp

SRC_DIRS += $(IOC)/ezchannelRPC
INC += ezchannelRPC.h
LIBSRCS += ezchannelRPC.cpp

SRC_DIRS += $(IOC)/v3Channel
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
LIBSRCS += v3ChannelPut.cpp
LIBSRCS += v3ChannelArray.cpp
LIBSRCS += v3ChannelRegister.cpp
LIBSRCS += v3ChannelMonitor.cpp

SRC_DIRS += $(IOC)/channelProviderLocal
DBD += channelProviderLocal.dbd
INC += channelProviderLocal.h
LIBSRCS += channelProviderLocal.cpp
LIBSRCS += channelProviderLocalRegister.cpp

SRC_DIRS += $(IOC)/multiValue
DBD += createMultiValue.dbd
INC += multiValue.h
INC += valueChannel.h
LIBSRCS += valueChannel.cpp
LIBSRCS += createMultiValueRegister.cpp
LIBSRCS += multiValueChannelProvider.cpp
LIBSRCS += multiValueChannel.cpp
LIBSRCS += multiValueChannelGet.cpp

#SRC_DIRS += $(IOC)/support
#INC += support.h
#LIBSRCS += support.cpp
#LIBSRCS += supportState.cpp
#LIBSRCS += recordProcess.cpp

#SRC_DIRS += $(IOC)/database
#INC += pvDatabase.h
#LIBSRCS += pvRecord.cpp
#LIBSRCS += pvDatabase.cpp

#SRC_DIRS += $(IOC)/install
#INC += install.h
#LIBSRCS += afterStart.cpp
#LIBSRCS += install.cpp
#LIBSRCS += supportCreation.cpp

#SRC_DIRS += $(IOC)/support/basic
#INC += basicSupport.h
#INC += noopSupport.cpp
#INC += genericSupport.cpp
#INC += scanFieldSupport.cpp

#SRC_DIRS += $(IOC)/util
#INC += util.h
#LIBSRCS += scanTypeFunc.cpp
#LIBSRCS += requestResultFunc.cpp
#LIBSRCS += eventScanner.cpp
#LIBSRCS += periodicScanner.cpp
#LIBSRCS += scanField.cpp

#SRC_DIRS += $(IOC)/monitor
#INC += monitorAlgorithm.h

#SRC_DIRS += $(IOC)/pvCopy
#INC += pvCopy.h
#LIBSRCS += pvCopy.cpp
#LIBSRCS += pvShare.cpp
##LIBSRCS += pvShareStructureArray.cpp


include $(TOP)/configure/RULES

