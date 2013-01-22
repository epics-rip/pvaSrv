/*createMultiValueRegister.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <vector>

#include <iocsh.h>

#include <epicsExport.h>
#include <epicsThread.h>

#include <multiValue.h>
#include <pv/channelProviderLocal.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvIOC;
using namespace epics::pvaSrv;
using std::tr1::static_pointer_cast;


static String requesterName("configureMultiValue");

class MultiChannelConfig;
typedef std::tr1::shared_ptr<MultiChannelConfig> MultiChannelConfigPtr;

class MultiChannelConfig : public Requester 
{
public:
    POINTER_DEFINITIONS(MultiChannelConfig);
    std::vector<MultiValueChannelProviderPtr> providerArray;
    virtual ~MultiChannelConfig() {}
    virtual String getRequesterName() {return requesterName;}
    virtual void message(String const & message,MessageType messageType)
    {
         printf("%s %s\n",requesterName.c_str(),message.c_str());
    }
};

static MultiChannelConfigPtr multiChannelConfig(new MultiChannelConfig());     

static const iocshArg multiChannelArg0 = { "configFileName",iocshArgString};
static const iocshArg *multiChannelArgs[] = {&multiChannelArg0};
static const iocshFuncDef multiChannelFuncDef = 
  {"createMultiValue",1,multiChannelArgs};

extern "C" void configureMultiValue(const iocshArgBuf *args)
{
    String fileName(args[0].sval);
    epicsThreadSleep(.1);
    FILE *f;
    f = fopen(fileName.c_str(),"r");
    if(f==NULL) {
        printf("configureMultiValue file %s open failure\n",fileName.c_str());
        return;
    }
    String valueProvider("v3Channel");
    String channelName;
    StringArrayPtr fieldNames(new StringArray());
    StringArrayPtr valueChannelNames(new StringArray());
    char buffer[180];
    bool gotStart = false;
    while(true) {
        char *result = fgets(buffer,180,f);
        if(result==NULL) break;
        String line(buffer);
        size_t pos = line.find("channelValueProvider");
        if(pos!=std::string::npos) {
              pos = line.find(' ');
              line = line.substr(pos+1);
              pos = line.find('\n');
              line = line.substr(0,pos);
              valueProvider = line;
              continue;
        } 
        pos = line.find("channelName");
        if(pos!=std::string::npos) {
              pos = line.find(' ');
              line = line.substr(pos+1);
              pos = line.find('\n');
              line = line.substr(0,pos);
              channelName = line;
              continue;
        } 
        pos = line.find("channelValue");
        if(pos!=std::string::npos) gotStart = true;
        break;
    }
    if(!gotStart) {
         printf("configureMultiValue fileName %s bad syntax\n",
            fileName.c_str());
         fclose(f);
         return;
    }
    fpos_t fpos;
    fgetpos(f,&fpos);
    int nlines = 0;
    while(true) {
        char *result = fgets(buffer,180,f);
        if(result==NULL) break;
        nlines++;
    }
    fieldNames->reserve(nlines);
    valueChannelNames->reserve(nlines);
    fsetpos(f,&fpos);
    bool isOK = true;
    for(int i=0; i<nlines; i++) {
        char *result = fgets(buffer,180,f);
        if(result==NULL) {
            printf("configureMultiValue fileName %s bad syntax\n",
               fileName.c_str());
            fclose(f);
            return;
        }
        String line(buffer);
        size_t pos = line.find_first_not_of(' ');
        line = line.substr(pos);
        pos = line.find_first_of(' ');
        String fieldName = line.substr(0,pos);
        fieldNames->push_back(fieldName);
        line = line.substr(pos+1);
        pos = line.find_first_not_of(' ');
        line = line.substr(pos);
        pos = line.find_first_of('\n');
        String valueChannelName = line.substr(0,pos);
        valueChannelNames->push_back(valueChannelName);
    }
    fclose(f);
    if(!isOK) {
        printf("configureMultiValue fileName %s bad syntax for channelValues\n",
           fileName.c_str());
        return;
    }
    ChannelAccess::shared_pointer channelAccess = getChannelAccess();
    ChannelProvider::shared_pointer channelProvider =
        channelAccess->getProvider(valueProvider);
    if(channelProvider.get()==NULL) {
        printf("channelProvider v3Channel not found\n");
        return;
    }
    MultiValueChannelProviderPtr provider(
        new MultiValueChannelProvider(
             multiChannelConfig,
             channelProvider,
             channelName,
             fieldNames,
             valueChannelNames));
    size_t oldSize = multiChannelConfig->providerArray.size();
    multiChannelConfig->providerArray.reserve(oldSize+1);
    multiChannelConfig->providerArray.push_back(provider);
    channelProvider = channelAccess->getProvider("local");
    if(channelProvider.get()==NULL) {
        printf("channelProvider local not found\n");
        return;
    }
    ChannelProviderLocalPtr channelProviderLocal =
         static_pointer_cast<ChannelProviderLocal>(channelProvider);
    channelProviderLocal->registerProvider(channelName,provider);
}

static void createMultiValueRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&multiChannelFuncDef, configureMultiValue);
    }
}


epicsExportRegistrar(createMultiValueRegister);
