# Makefile for the EPICS V4 pvaSrv module

TOP = .
include $(TOP)/configure/CONFIG

DIRS := configure

DIRS += src
src_DEPEND_DIRS = configure

EMBEDDED_TOPS += exampleApp
EMBEDDED_TOPS += testApp

DIRS += $(EMBEDDED_TOPS)

exampleApp_DEPEND_DIRS += src
testApp_DEPEND_DIRS += src

include $(TOP)/configure/RULES_TOP
