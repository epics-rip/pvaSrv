TOP = ..
include $(TOP)/configure/CONFIG

IOC = $(TOP)/pvIocApp/

SRC_DIRS += $(IOC)/database
INC += pvDatabase.h
LIBSRCS += pvDatabase.cpp

SRC_DIRS += $(IOC)/install
INC += install.h
LIBSRCS += install.cpp

SRC_DIRS += $(IOC)/support
INC += support.h
LIBSRCS += support.cpp


SRC_DIRS += $(IOC)/util
INC += util.h
LIBSRCS += util.cpp

LIBRARY=pvIoc

pvIoc_LIBS += Com pvData pvAccess

include $(TOP)/configure/RULES

