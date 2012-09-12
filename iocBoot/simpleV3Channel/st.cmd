< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/simpleV3Channel.dbd")
simpleV3Channel_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/test.db")

cd ${TOP}/iocBoot/${IOC}
iocInit()

startV3Channel
