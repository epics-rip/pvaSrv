/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
/* Marty Kraimer 2011.03 */
/* This connects to a DB record and presents the data as a PVStructure
 * It provides access to  value, alarm, display, and control.
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <memory>

#include <dbAccess.h>
#include <dbEvent.h>
#include <dbNotify.h>
#include <dbCommon.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>

#include "dbPv.h"
#include "dbUtil.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::string;

namespace epics { namespace pvaSrv { 

DbPvPut::DbPvPut(
        DbPvPtr const &dbPv,
        ChannelPutRequester::shared_pointer const &channelPutRequester)
    : dbUtil(DbUtil::getDbUtil()),
      dbPv(dbPv),
      channelPutRequester(channelPutRequester),
      propertyMask(0),
      process(false),
      block(false),
      firstTime(true),
      beingDestroyed(false)
{
    if(DbPvDebug::getLevel()>0) printf("dbPvPut::dbPvPut()\n");
}

DbPvPut::~DbPvPut()
{
    if(DbPvDebug::getLevel()>0) printf("dbPvPut::~dbPvPut()\n");
}

bool DbPvPut::init(PVStructure::shared_pointer const &pvRequest)
{
    propertyMask = dbUtil->getProperties(
        channelPutRequester,
        pvRequest,
        dbPv->getDbChannel(),
        true);
    if(propertyMask==dbUtil->noAccessBit) return false;
    if(propertyMask==dbUtil->noModBit) {
        channelPutRequester->message(
                    "field not allowed to be changed",
                    errorMessage);
        return 0;
    }
    pvStructure = PVStructure::shared_pointer(
        dbUtil->createPVStructure(
            channelPutRequester,
            propertyMask,
            dbPv->getDbChannel()));
    if (!pvStructure.get()) return false;
    if (propertyMask & dbUtil->dbPutBit) {
        if (propertyMask & dbUtil->processBit) {
            channelPutRequester->message(
                        "process determined by dbPutField",
                        errorMessage);
        }
    } else if (propertyMask&dbUtil->processBit) {
        process = true;
        pNotify.reset(new (struct processNotify)());
        struct processNotify *pn = pNotify.get();
        pn->chan = dbPv->getDbChannel();
        pn->requestType = putProcessRequest;
        pn->putCallback  = this->putCallback;
        pn->doneCallback = this->doneCallback;
        pn->usrPvt = this;
        if (propertyMask & dbUtil->blockBit) block = true;
    }
    int numFields = pvStructure->getNumberFields();
    bitSet.reset(new BitSet(numFields));
    channelPutRequester->channelPutConnect(
       Status::Ok,
       getPtrSelf(),
       pvStructure->getStructure());
    return true;
}

string DbPvPut::getRequesterName() {
    return channelPutRequester->getRequesterName();
}

void DbPvPut::message(string const &message,MessageType messageType)
{
    channelPutRequester->message(message,messageType);
}

void DbPvPut::destroy() {
    if(DbPvDebug::getLevel()>0) printf("dbPvPut::destroy beingDestroyed %s\n",
         (beingDestroyed ? "true" : "false"));
    {
        Lock xx(mutex);
        if (beingDestroyed) return;
        beingDestroyed = true;
        if (pNotify) dbNotifyCancel(pNotify.get());
    }
}

void DbPvPut::put(PVStructurePtr const &pvStructure, BitSetPtr const & bitSet)
{
    if (DbPvDebug::getLevel()>0) printf("dbPvPut::put()\n");
    Lock lock(dataMutex);
    this->pvStructure = pvStructure;
    this->bitSet = bitSet;
    PVFieldPtr pvField = pvStructure.get()->getPVFields()[0];
    if (propertyMask & dbUtil->dbPutBit) {
        status = dbUtil->putField(
                    channelPutRequester,
                    propertyMask,
                    dbPv->getDbChannel(),
                    pvField);
        lock.unlock();
        channelPutRequester->putDone(status, getPtrSelf());
        return;
    }
    dbScanLock(dbChannelRecord(dbPv->getDbChannel()));
    status = dbUtil->put(
                channelPutRequester, propertyMask, dbPv->getDbChannel(), pvField);
    if (process && !block) dbProcess(dbChannelRecord(dbPv->getDbChannel()));
    dbScanUnlock(dbChannelRecord(dbPv->getDbChannel()));
    lock.unlock();
    if (block && process) {
        dbProcessNotify(pNotify.get());
    } else {
        channelPutRequester->putDone(status, getPtrSelf());
    }
}

void DbPvPut::doneCallback(struct processNotify *pn)
{
    DbPvPut * pdp = static_cast<DbPvPut *>(pn->usrPvt);
    pdp->channelPutRequester->putDone(
                pdp->status,
                pdp->getPtrSelf());
}

void DbPvPut::get()
{
    if(DbPvDebug::getLevel()>0) printf("dbPvPut::get()\n");
    {
        Lock lock(dataMutex);
        dbScanLock(dbChannelRecord(dbPv->getDbChannel()));
        bitSet->clear();
        Status status = dbUtil->get(
                    channelPutRequester,
                    propertyMask,
                    dbPv->getDbChannel(),
                    pvStructure,
                    bitSet,
                    0);
        dbScanUnlock(dbChannelRecord(dbPv->getDbChannel()));
        if(firstTime) {
            firstTime = false;
            bitSet->set(pvStructure->getFieldOffset());
        }
    }
    channelPutRequester->getDone(
                status,
                getPtrSelf(),
                pvStructure,
                bitSet);
}

void DbPvPut::lock()
{
    dataMutex.lock();
}

void DbPvPut::unlock()
{
    dataMutex.unlock();
}

}}
