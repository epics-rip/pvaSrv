< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/testV3Channel.dbd")
testV3Channel_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/dbScalar.db","name=byte01,type=byte")
dbLoadRecords("db/dbScalar.db","name=short01,type=short")
dbLoadRecords("db/dbScalar.db","name=int01,type=longout")
dbLoadRecords("db/dbScalar.db","name=float01,type=float")
dbLoadRecords("db/dbScalar.db","name=double01,type=ao")
dbLoadRecords("db/dbArray.db","name=byteArray01,type=CHAR")
dbLoadRecords("db/dbArray.db","name=shortArray01,type=SHORT")
dbLoadRecords("db/dbArray.db","name=intArray01,type=LONG")
dbLoadRecords("db/dbArray.db","name=floatArray01,type=FLOAT")
dbLoadRecords("db/dbArray.db","name=doubleArray01,type=DOUBLE")
dbLoadRecords("db/dbString.db","name=string01")
dbLoadRecords("db/dbStringArray.db","name=stringArray01")
dbLoadRecords("db/dbEnum.db","name=enum01")
dbLoadRecords("db/dbCounter.db","name=counter01");

dbLoadRecords("db/dbScalar.db","name=byte02,type=byte")
dbLoadRecords("db/dbScalar.db","name=short02,type=short")
dbLoadRecords("db/dbScalar.db","name=int02,type=longout")
dbLoadRecords("db/dbScalar.db","name=float02,type=float")
dbLoadRecords("db/dbScalar.db","name=double02,type=ao")
dbLoadRecords("db/dbArray.db","name=byteArray02,type=CHAR")
dbLoadRecords("db/dbArray.db","name=shortArray02,type=SHORT")
dbLoadRecords("db/dbArray.db","name=intArray02,type=LONG")
dbLoadRecords("db/dbArray.db","name=floatArray02,type=FLOAT")
dbLoadRecords("db/dbArray.db","name=doubleArray02,type=DOUBLE")
dbLoadRecords("db/dbString.db","name=string02")
dbLoadRecords("db/dbStringArray.db","name=stringArray02")
dbLoadRecords("db/dbEnum.db","name=enum02")
dbLoadRecords("db/dbCounter.db","name=counter02");

dbLoadRecords("db/dbScalar.db","name=byte03,type=byte")
dbLoadRecords("db/dbScalar.db","name=short03,type=short")
dbLoadRecords("db/dbScalar.db","name=int03,type=longout")
dbLoadRecords("db/dbScalar.db","name=float03,type=float")
dbLoadRecords("db/dbScalar.db","name=double03,type=ao")
dbLoadRecords("db/dbArray.db","name=byteArray03,type=CHAR")
dbLoadRecords("db/dbArray.db","name=shortArray03,type=SHORT")
dbLoadRecords("db/dbArray.db","name=intArray03,type=LONG")
dbLoadRecords("db/dbArray.db","name=floatArray03,type=FLOAT")
dbLoadRecords("db/dbArray.db","name=doubleArray03,type=DOUBLE")
dbLoadRecords("db/dbString.db","name=string03")
dbLoadRecords("db/dbStringArray.db","name=stringArray03")
dbLoadRecords("db/dbEnum.db","name=enum03")
dbLoadRecords("db/dbCounter.db","name=counter03");

cd ${TOP}/iocBoot/${IOC}
iocInit()
epicsThreadSleep(2.0)
casr
startV3Channel
startExampleChannel exampleChannel
startPVServiceChannel
dbpf string01 10.1
