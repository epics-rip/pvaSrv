< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/testMultiValue.dbd")
testMultiValue_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords "db/quadruple.db","name=quadruple"

cd ${TOP}/iocBoot/${IOC}
iocInit()

startV3Channel
startChannelProviderLocal
createMultiValueChannel quadrupole.txt
