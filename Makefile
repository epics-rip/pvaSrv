#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS += configure

DIRS += pvIocApp
pvIocApp_DEPEND_DIRS = configure

DIRS += exampleApp
exampleApp_DEPEND_DIRS = pvIocApp

DIRS += testApp
testApp_DEPEND_DIRS = pvIocApp

DIRS += iocBoot

include $(TOP)/configure/RULES_TOP


