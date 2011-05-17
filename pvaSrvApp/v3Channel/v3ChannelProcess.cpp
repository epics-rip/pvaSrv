/* vsChannelProcess.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/* Marty Kraimer 2011.03 */
/* This connects to a V3 record and presents the data as a PVStructure
 * It provides access to  value, alarm, display, and control.
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <memory>

#include <pvData.h>
#include <pvAccess.h>

#include "pvDatabase.h"
#include "v3Channel.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;
using namespace epics::pvAccess;

static String recordString("record");
static String processString("process");
static String fieldString("field");
static String fieldListString("fieldList");
static String valueString("value");

V3ChannelProcess::V3ChannelProcess(
    V3Channel::shared_pointer const &v3Channel,
    ChannelProcessRequester::shared_pointer const &channelProcessRequester,
    DbAddr &dbAddr)
: v3Channel(v3Channel),
  channelProcessRequester(channelProcessRequester),
  dbAddr(dbAddr),
  processListNode(*this),
  pNotify(0),
  notifyAddr(0),
  event(),
  v3ChannelProcessPtr(V3ChannelProcess::shared_pointer(this))
{
}

V3ChannelProcess::~V3ChannelProcess() {}


ChannelProcessListNode * V3ChannelProcess::init()
{
   pNotify = std::auto_ptr<struct putNotify>(new (struct putNotify)());
   notifyAddr = std::auto_ptr<DbAddr>(new DbAddr());
   memcpy(notifyAddr.get(),&dbAddr,sizeof(DbAddr));
   DbAddr *paddr = notifyAddr.get();
   struct dbCommon *precord = paddr->precord;
   char buffer[sizeof(precord->name) + 10];
   strcpy(buffer,precord->name);
   strcat(buffer,".PROC");
   if(dbNameToAddr(buffer,paddr)!=0) {
        throw std::logic_error(String("dbNameToAddr failed"));
   }
   struct putNotify *pn = pNotify.get();
   pn->userCallback = this->notifyCallback;
   pn->paddr = paddr;
   pn->nRequest = 1;
   pn->dbrType = DBR_CHAR;
   pn->usrPvt = this;
   channelProcessRequester->channelProcessConnect(Status::OK,v3ChannelProcessPtr);
   return &processListNode;
}

String V3ChannelProcess::getRequesterName() {
    return channelProcessRequester->getRequesterName();
}

void V3ChannelProcess::message(String message,MessageType messageType)
{
    channelProcessRequester->message(message,messageType);
}

void V3ChannelProcess::destroy() {
    v3Channel->removeChannelProcess(processListNode);
    delete this;
}

void V3ChannelProcess::process(bool lastRequest)
{
    epicsUInt8 value = 1;
    pNotify.get()->pbuffer = &value;
    dbPutNotify(pNotify.get());
    event.wait();
    channelProcessRequester->processDone(Status::OK);
    
}

void V3ChannelProcess::notifyCallback(struct putNotify *pn)
{
    V3ChannelProcess * cp = static_cast<V3ChannelProcess *>(pn->usrPvt);
    cp->event.signal();
}


}}

