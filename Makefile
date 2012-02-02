#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS += configure

DIRS += pvIocApp
pvIocApp_DEPEND_DIRS = configure

#In order to build testApp configure/RELEASE must change
#NT=/home/mrk/hg/alphaCPP/normativeTypes to point to a working version of normativeTypes
DIRS += testApp
testApp_DEPEND_DIRS = pvIocApp

DIRS += iocBoot

include $(TOP)/configure/RULES_TOP


