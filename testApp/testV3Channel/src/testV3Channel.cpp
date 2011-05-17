/*testV3Channel.cpp */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>

#include <cantProceed.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>
#include <iocsh.h>

#include <epicsExport.h>

#include <pvIntrospect.h>
#include <pvData.h>
#include <pvAccess.h>

#include "pvDatabase.h"
#include "v3Channel.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvIOC;

class FindRequester : public ChannelFindRequester {
public:
    FindRequester(){}
    virtual ~FindRequester(){}
    virtual void channelFindResult(
        const Status &status,
        ChannelFind::shared_pointer const &channelFind,
        bool wasFound);
};

void FindRequester::channelFindResult(
    const Status &status,
    ChannelFind::shared_pointer const &channelFind,
    bool wasFound)
{
    String message = status.getMessage();
    printf("channelFindResult status %s wasFound %s\n",
        message.c_str(),
        (wasFound ? "true" : "false"));
}

class MyRequester :
    public virtual Requester,
    public ChannelRequester,
    public ChannelGetRequester
{
public:
    POINTER_DEFINITIONS(MyRequester);
    MyRequester()
    : name(String("testV3Channel")),
      channelPtr(Channel::shared_pointer()),
      channelGetPtr(ChannelGet::shared_pointer()),
      pvStructurePtr(PVStructure::shared_pointer()),
      bitSetPtr(BitSet::shared_pointer())
    { }
    Channel::shared_pointer const & getChannel() {return channelPtr;}
    virtual ~MyRequester() { }
    virtual String getRequesterName()
    {
        return name;
    }
    virtual void message(String message,MessageType messageType)
    {
        String typeName = messageTypeName[messageType];
        printf("ChannelRequester message %s messageType %s\n",
            message.c_str(),typeName.c_str());
    }
    virtual void channelCreated(
        const Status &status,
        Channel::shared_pointer const &channel)
    {
        channelPtr = channel;
        String message = status.getMessage();
        bool isOK = status.isOK();
        printf("channelCreated status %s statusOK %s\n",
        message.c_str(),
        (isOK ? "true" : "false"));
    }
    virtual void channelStateChange(
        Channel::shared_pointer const & c,
        Channel::ConnectionState connectionState)
    {
        String state = Channel::ConnectionStateNames[connectionState];
        printf("channelStateChange %s\n",state.c_str());
    }
    virtual void channelGetConnect(
        const Status &status,
        ChannelGet::shared_pointer const &channelGet,
        PVStructure::shared_pointer const &pvStructure,
        BitSet::shared_pointer const &bitSet)
    {
        channelGetPtr = channelGet;
        pvStructurePtr = pvStructure;
        bitSetPtr = bitSet;
        printf("channelGetConnect statusOK %s\n",
            (status.isOK() ? "true" : "false"));
    }
    virtual void getDone(const Status &status)
    {
        printf("getDone statusOK %s\n",
            (status.isOK() ? "true" : "false"));
        String buffer("");
        pvStructurePtr->toString(&buffer);
        printf("%s\n",buffer.c_str());
    }
private:
    String name;
    Channel::shared_pointer channelPtr;
    ChannelGet::shared_pointer channelGetPtr;
    PVStructure::shared_pointer pvStructurePtr;
    BitSet::shared_pointer bitSetPtr;
};

static const iocshArg testArg0 = { "pvName", iocshArgString };
static const iocshArg *testArgs[] = {
    &testArg0};

static const iocshFuncDef testV3ChannelFuncDef = {
    "testV3Channel", 1, testArgs};
static void testV3ChannelCallFunc(const iocshArgBuf *args)
{
    char *pvName = args[0].sval;
    printf("testV3Channel pvName %s\n",pvName);
    String channelName(pvName);
    printf("channelName %s\n",channelName.c_str());
    ChannelProvider::shared_pointer const &channelProvider
        = V3ChannelProvider::getChannelProvider();
    String providerName = channelProvider->getProviderName();
    printf("providerName %s\n",providerName.c_str());
    FindRequester::shared_pointer findRequester
        = FindRequester::shared_pointer(new FindRequester());
    channelProvider->channelFind(
        channelName,
        findRequester);
    MyRequester::shared_pointer  myRequester
         = MyRequester::shared_pointer(new MyRequester());
    Channel::shared_pointer channel = channelProvider->createChannel(
         channelName,
         myRequester,
         0,
         String(""));
    CreateRequest::shared_pointer createRequest = getCreateRequest();
    PVStructure::shared_pointer pvRequest = createRequest->createRequest(
        String("record[process=true]field(value,timeStamp,alarm)"));
    ChannelGet::shared_pointer channelGet = channel->createChannelGet(
        myRequester,pvRequest);
    channelGet->get(false);
    channelGet->destroy();
    if(channel!=0) {
        channel->destroy();
    }
}


static void testV3ChannelRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&testV3ChannelFuncDef, testV3ChannelCallFunc);
    }
}
epicsExportRegistrar(testV3ChannelRegister);
