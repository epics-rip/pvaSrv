< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/testLocalChannelProvider.dbd")
testLocalChannelProvider_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords "db/quadruple.db","name=quad0"

cd ${TOP}/iocBoot/${IOC}
iocInit()

startV3Channel
startChannelProviderLocal

