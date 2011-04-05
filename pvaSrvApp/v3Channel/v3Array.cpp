/* v3Array.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>

#include <lock.h>
#include <dbAccess.h>
#include <serializeHelper.h>
#include <convert.h>
#include <pvIntrospect.h>
#include <pvData.h>
#include <pvAccess.h>
#include "v3Channel.h"
#include "v3Array.h"
#include "v3Util.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;

template<typename T>
class V3ValueArray : public epics::pvData::PVValueArray<T> {
public:
    typedef T  value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef PVArrayData<T> ArrayDataType;

    V3ValueArray(
        PVStructure *parent,
        ScalarArrayConstPtr scalar,
        DbAddr &dbAddr,bool shareData);
    virtual ~V3ValueArray();
    virtual void setCapacity(int capacity);
    virtual int get(int offset, int length, ArrayDataType *data);
    virtual int put(int offset,int length, pointer from, int fromOffset);
    virtual void shareData(pointer value,int capacity,int length);
    // from Serializable
    virtual void serialize(
        ByteBuffer *pbuffer,
        SerializableControl *pflusher) const;
    virtual void deserialize(
        ByteBuffer *pbuffer,
        DeserializableControl *pflusher);
    virtual void serialize(
         ByteBuffer *pbuffer,
         SerializableControl *pflusher,
         int offset, int count) const;
    virtual bool operator==(PVField& pv) ;
    virtual bool operator!=(PVField& pv) ;
private:
    void updateV3Record() const;
    void updateFromV3Record() const;
    int getV3Length() const;
    void setV3Length(int int32) const;
    DbAddr &dbAddr;
    T *value;
    T *shareValue;
};


typedef V3ValueArray<int8> V3ByteArray;
typedef V3ValueArray<int16> V3ShortArray;
typedef V3ValueArray<int32> V3IntArray;
typedef V3ValueArray<float> V3FloatArray;
typedef V3ValueArray<double> V3DoubleArray;
typedef V3ValueArray<String> V3StringArray;
;

template<typename T>
V3ValueArray<T>::V3ValueArray(
    PVStructure *parent,ScalarArrayConstPtr scalarArray,
    DbAddr &dbAddr,bool shareData)
: PVValueArray<T>(parent,scalarArray),
  dbAddr(dbAddr),
  value(0),
  shareValue(0)
{
    if(shareData) {
        shareValue = static_cast<T *>(dbAddr.pfield);
    } else {
        value = new T[dbAddr.no_elements];
        for(int i=0;i<dbAddr.no_elements;i++) value[i] = T();
    }
    int capacity = dbAddr.no_elements;
    int length = getV3Length();
    PVArray::setCapacityLength(capacity,length);
    PVArray::setCapacityMutable(false);
}

template<typename T>
int V3ValueArray<T>::getV3Length() const
{
    long rec_length = 0;
    long rec_offset = 0;
    struct rset *prset = dbGetRset(&dbAddr);
    get_array_info get_info;
    get_info = (get_array_info)(prset->get_array_info);
    get_info(&dbAddr, &rec_length, &rec_offset);
    if(rec_offset!=0) {
         throw std::logic_error(String("Can't handle offset != 0"));
    }
    int length = rec_length;
    return length;
}

template<typename T>
void V3ValueArray<T>::setV3Length(int32 len) const
{
    long length = len;
    struct rset *prset = dbGetRset(&dbAddr);
    if(prset && prset->put_array_info) {
        put_array_info put_info;
        put_info = (put_array_info)(prset->put_array_info);
        put_info(&dbAddr, length);
    } 
}

template<typename T>
V3ValueArray<T>::~V3ValueArray()
{
    if(value!=0) delete[] value;
}

template<typename T>
void V3ValueArray<T>::setCapacity(int capacity)
{ }

template<typename T>
int V3ValueArray<T>::get(int offset, int len, PVArrayData<T> *data)
{
    dbScanLock(dbAddr.precord);
    updateFromV3Record();
    dbScanUnlock(dbAddr.precord);
    int length = this->getLength();
    int n = len;
    if(offset+len > length) {
        n = length-offset;
    }
    if(n<1) n = 0;
    data->offset = offset;
    data->data = value;
    if(value==0) data->data = shareValue;
    return n;
}

template<typename T>
int V3ValueArray<T>::put(int offset,int len,
    pointer from,int fromOffset)
{
    if(PVField::isImmutable()) {
        PVField::message("field is immutable",errorMessage);
        return 0;
    }
    if(from==shareValue) return len;
    if(from==value) return len;
    int length = this->getLength();
    if(offset+len > length) len = length - offset;
    if(len<1) return 0;
    T * data = (value==0) ? shareValue : value;
    dbScanLock(dbAddr.precord);
    for(int i=0;i<len;i++) {
       data[i+offset] = from[i+fromOffset];
    }
    length = len+offset;
    this->setLength(length);
    updateV3Record();
    dbScanUnlock(dbAddr.precord);
    this->postPut();
    return len;
}

template<typename T>
void V3ValueArray<T>::shareData(pointer shareValue,int capacity,int length)
{
    throw std::logic_error(String("Not Implemented"));
}

template<typename T>
void V3ValueArray<T>::serialize(ByteBuffer *pbuffer,
            SerializableControl *pflusher) const
{
    serialize(pbuffer, pflusher, 0, this->getLength());
}

template<typename T>
void V3ValueArray<T>::deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pcontrol)
{
    int length = SerializeHelper::readSize(pbuffer, pcontrol);
    if(length<=0) return;
    if(length>this->getCapacity()) {
        throw std::logic_error(String("Capacity immutable"));
    }
    T * data = (value==0) ? shareValue : value;
    int i=0;
    while(true) {
        int maxIndex = std::min(length-i, pbuffer->getRemaining())+i;
        if(data==shareValue) dbScanLock(dbAddr.precord);
        for(; i<maxIndex; i++) {
            data[i] = pbuffer->get<T>();
        }
        if(data==shareValue) dbScanUnlock(dbAddr.precord);
        if(i>=length) break;
        pcontrol->ensureData(1);
    }
    this->setLength(length);
    dbScanLock(dbAddr.precord);
    updateV3Record();
    dbScanUnlock(dbAddr.precord);
    PVField::postPut();
}

template<typename T>
void V3ValueArray<T>::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, int offset, int count) const
{
    int oldLength = count;
    dbScanLock(dbAddr.precord);
    updateFromV3Record();
    dbScanUnlock(dbAddr.precord);
    int length = this->getLength();
    if(offset==0 && oldLength==count) {
        count = length;
    } else {
        if(offset<0) offset = 0;
        if(offset>length) offset = length;
        if(count<0) count = length-offset;
        int maxcount = length-offset;
        if(count>maxcount) count = maxcount;
    }
    SerializeHelper::writeSize(count, pbuffer, pflusher);
    int end = offset+count;
    int i = offset;
    T * data = (value==0) ? shareValue : value;
    while(true) {
        int maxIndex = std::min<int>(end-i, pbuffer->getRemaining())+i;
        if(data==shareValue) dbScanLock(dbAddr.precord);
        for(; i<maxIndex; i++) pbuffer->put(data[i]);
        if(data==shareValue) dbScanUnlock(dbAddr.precord);
        if(i>=end) break;
        pflusher->flushSerializeBuffer();
    }
}

template<typename T>
bool V3ValueArray<T>::operator==(PVField& pv)
{
    return getConvert()->equals(this, &pv);
}

template<typename T>
bool V3ValueArray<T>::operator!=(PVField& pv)
{
    return !(getConvert()->equals(this, &pv));
}

template<typename T>
void V3ValueArray<T>::updateV3Record() const
{
    int32 length = this->getLength();
    setV3Length(length);
    if(shareValue) return;
    T * array = static_cast<T *>(dbAddr.pfield);
    for(int i=0; i<length; i++) array[i] = value[i];
}

template<typename T>
void V3ValueArray<T>::updateFromV3Record() const
{
    int length = getV3Length();
    if(length!=this->getLength()) {
        V3ValueArray<T> *xxx = const_cast<V3ValueArray<T> *>(this);
        xxx->setLength(length);
    }
    if(shareValue) return;
    T * array = static_cast<T *>(dbAddr.pfield);
    for(int i=0; i<length; i++) value[i] = array[i];
}

template<>
void V3ValueArray<String>::updateV3Record() const
{
    int32 length = this->getLength();
    setV3Length(length);
    if(shareValue) return;
    char *pchar = static_cast<char *>(dbAddr.pfield);
    for(int i=0; i<length; i++) {
        strcpy(pchar,value[i].c_str());
        pchar += dbAddr.field_size;
    }
}

template<>
void V3ValueArray<String>::updateFromV3Record() const
{
    int length = getV3Length();
    V3ValueArray<String> *xxx = const_cast<V3ValueArray<String> *>(this);
    xxx->setLength(length);
    if(shareValue) return;
    char *pchar = static_cast<char *>(dbAddr.pfield);
    for(int i=0; i<length; i++) {
        if(strcmp(pchar,value[i].c_str())!=0) {
           value[i] = String(pchar);
        }
        pchar += dbAddr.field_size;
    }
}

template<>
int V3ValueArray<String>::get(int offset, int len, PVArrayData<String> *data)
{
    dbScanLock(dbAddr.precord);
    updateFromV3Record();
    dbScanUnlock(dbAddr.precord);
    int n = len;
    int length = this->getLength();
    if(offset+len > length) {
        n = length-offset;
    }
    if(n<=0) return 0;
    char *pchar = static_cast<char *>(dbAddr.pfield);
    pchar += dbAddr.field_size*offset;
    for(int i=offset; i<offset+n; i++) {
        value[i] = String(pchar);
        pchar += dbAddr.field_size;
    }
    data->data = value;
    data->offset = offset;
    return n;
}

template<>
int V3ValueArray<String>::put(int offset,int len,
    pointer from,int fromOffset)
{
    if(PVField::isImmutable()) {
        PVField::message("field is immutable",errorMessage);
        return 0;
    }
    if(from==shareValue) return len;
    if(from==value) return len;
    int length = this->getLength();
    if(offset+len > length) len = length - offset;
    if(len<1) return 0;
    String::size_type field_size =
        static_cast<String::size_type>(dbAddr.field_size);
    for(int i=0; i<len;i++) {
        String val = from[i];
        if(val.length()>=field_size) val = val.substr(0,field_size-1);
        value[i] = val;
    }
    length = len+offset;
    this->setLength(length);
    dbScanLock(dbAddr.precord);
    updateV3Record();
    dbScanUnlock(dbAddr.precord);
    this->postPut();
    return len;
}

template<>
void V3ValueArray<String>::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, int offset, int count) const
{
    int oldLength = count;
    dbScanLock(dbAddr.precord);
    updateFromV3Record();
    dbScanUnlock(dbAddr.precord);
    int length = this->getLength();
    if(offset==0 && oldLength==count) {
        count = length;
    } else {
        if(offset<0) offset = 0;
        if(offset>length) offset = length;
        if(count<0) count = length-offset;
        int maxcount = length-offset;
        if(count>maxcount) count = maxcount;
    }
    SerializeHelper::writeSize(count, pbuffer, pflusher);
    int end = offset+count;
    for(int i = offset; i<end; i++) {
        SerializeHelper::serializeString(value[i], pbuffer, pflusher);
    }
}

template<>
void V3ValueArray<String>::deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pcontrol) {
    int size = SerializeHelper::readSize(pbuffer, pcontrol);
    if(size<=0) return;
    if(size>getCapacity()) {
        throw std::logic_error(String("Capacity immutable"));
    }
    String::size_type field_size =
        static_cast<String::size_type>(dbAddr.field_size);
    for(int i = 0; i<size; i++) {
        value[i] = SerializeHelper::deserializeString(pbuffer, pcontrol);
        if(value[i].length()>=field_size) {
            value[i] = value[i].substr(0,field_size-1);
        }
    }
    this->setLength(size);
    dbScanLock(dbAddr.precord);
    updateV3Record();
    dbScanUnlock(dbAddr.precord);
    postPut();
}

PVByteArray *V3ValueArrayCreate::createByteArray(
    PVStructure *parent,
    ScalarArrayConstPtr scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return new V3ByteArray(parent,scalar,dbAddr,shareData);
}

PVShortArray *V3ValueArrayCreate::createShortArray(
    PVStructure *parent,
    ScalarArrayConstPtr scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return new V3ShortArray(parent,scalar,dbAddr,shareData);
}

PVIntArray *V3ValueArrayCreate::createIntArray(
    PVStructure *parent,
    ScalarArrayConstPtr scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return new V3IntArray(parent,scalar,dbAddr,shareData);
}

PVFloatArray *V3ValueArrayCreate::createFloatArray(
    PVStructure *parent,
    ScalarArrayConstPtr scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return new V3FloatArray(parent,scalar,dbAddr,shareData);
}

PVDoubleArray *V3ValueArrayCreate::createDoubleArray(
    PVStructure *parent,
    ScalarArrayConstPtr scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return new V3DoubleArray(parent,scalar,dbAddr,shareData);
}

PVStringArray *V3ValueArrayCreate::createStringArray(
    PVStructure *parent,
    ScalarArrayConstPtr scalar,
    DbAddr &dbAddr)
{
    return new V3StringArray(parent,scalar,dbAddr,false);
}

static V3ValueArrayCreate* v3ValueArrayCreate = 0;
 
V3ValueArrayCreate *getV3ValueArrayCreate()
{
static Mutex mutex;
     Lock xx(mutex);

     if(v3ValueArrayCreate==0){
          v3ValueArrayCreate = new V3ValueArrayCreate();
     }
     return v3ValueArrayCreate;

}

}}
