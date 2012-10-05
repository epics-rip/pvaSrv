/* v3Array.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>

#include <pv/lock.h>
#include <dbAccess.h>
#include <pv/serializeHelper.h>
#include <pv/convert.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/v3Channel.h>
#include "v3Array.h"
#include "v3Util.h"

namespace epics { namespace pvIOC { 

using namespace epics::pvData;

template<typename T>
class V3ValueArray : public epics::pvData::PVValueArray<T> {
public:
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef std::vector<T> vector;
    typedef const std::vector<T> const_vector;
    typedef std::tr1::shared_ptr<vector> shared_vector;

    V3ValueArray(
        PVStructurePtr const & parent,
        ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr,bool shareData);
    virtual ~V3ValueArray();
    virtual void setCapacity(size_t capacity);
    virtual size_t get(size_t offset, size_t length, PVArrayData<T> &data);
    virtual size_t put(size_t offset,size_t length, const_pointer from, size_t fromOffset);
    virtual void shareData(
         std::tr1::shared_ptr<std::vector<T> > const & value,
         size_t capacity,size_t length);
    virtual pointer get() ;
    virtual pointer get() const ;
    virtual vector const & getVector() { return *value.get(); }
    virtual shared_vector const & getSharedVector(){return value;};
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
         size_t offset, size_t count) const;
    virtual bool operator==(PVField& pv) ;
    virtual bool operator!=(PVField& pv) ;
private:
    long getV3Length() const;
    void setV3Length(long length) const;
    DbAddr &dbAddr;
    bool shareV3Data;
    shared_vector value;
};


typedef V3ValueArray<int8> V3ByteArray;
typedef V3ValueArray<uint8> V3UByteArray;
typedef V3ValueArray<int16> V3ShortArray;
typedef V3ValueArray<uint16> V3UShortArray;
typedef V3ValueArray<int32> V3IntArray;
typedef V3ValueArray<uint32> V3UIntArray;
typedef V3ValueArray<float> V3FloatArray;
typedef V3ValueArray<double> V3DoubleArray;
typedef V3ValueArray<String> V3StringArray;

template<typename T>
V3ValueArray<T>::V3ValueArray(
    PVStructurePtr const & parent,ScalarArrayConstPtr const &scalarArray,
    DbAddr &dbAddr,bool shareData)
: PVValueArray<T>(scalarArray),
  dbAddr(dbAddr),
  shareV3Data(false),
  value(std::tr1::shared_ptr<std::vector<T> >(new std::vector<T>()))
{
    size_t capacity = dbAddr.no_elements;
    value->resize(capacity);
    if(shareV3Data) {
        parent->message("shareV3Data not implemented",warningMessage);
    }
    size_t length = getV3Length();
    T* v3data = static_cast<T *>(dbAddr.pfield);
    T * data = get();
    for(size_t i=0; i<capacity; i++) data[i] = v3data[i];
    PVArray::setCapacityLength(capacity,length);
    PVArray::setCapacityMutable(false);
}

template<typename T>
long V3ValueArray<T>::getV3Length() const
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
    long length = rec_length;
    return length;
}

template<typename T>
void V3ValueArray<T>::setV3Length(long len) const
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
}

template<typename T>
void V3ValueArray<T>::setCapacity(size_t capacity)
{ }

template<typename T>
size_t V3ValueArray<T>::get(size_t offset, size_t len, PVArrayData<T> &data)
{
    dbScanLock(dbAddr.precord);
    // updateFromV3Record();
    size_t length = getV3Length();
    if(length!=this->getLength()) {
        this->setLength(length);
    }
    T * pvalue = get();
    if(!shareV3Data) {
        T * array = static_cast<T *>(dbAddr.pfield);
        for(size_t i=0; i<length; i++) pvalue[i] = array[i];
    }
    dbScanUnlock(dbAddr.precord);
    size_t n = len;
    if(offset+len > length) {
        n = length-offset;
    }
    if(n<1) n = 0;
    data.data = getVector();
    data.offset = offset;
    return n;
}
template<typename T>
size_t V3ValueArray<T>::put(size_t offset,size_t len,
    const_pointer from,size_t fromOffset)
{
    if(PVField::isImmutable()) {
        PVField::message("field is immutable",errorMessage);
        return 0;
    }
    size_t length = this->getLength();
    size_t capacity = this->getCapacity();
    if(offset+len > capacity) len = capacity - offset;
    if(len<1) return 0;
    T * pvalue = get();
    dbScanLock(dbAddr.precord);
    if(len+offset>length) {
        length = len+offset;
        this->setLength(length);
    }
    if(from!=pvalue) {
        for(size_t i=0;i<len;i++) {
           pvalue[i+offset] = from[i+fromOffset];
        }
    }
    // update v4Record
    setV3Length(length);
    if(!shareV3Data) {
        T * array = static_cast<T *>(dbAddr.pfield);
        for(size_t i=0; i<length; i++) array[i] = pvalue[i];
    }
    dbScanUnlock(dbAddr.precord);
    this->postPut();
    return len;
}

template<typename T>
void V3ValueArray<T>::shareData(
    std::tr1::shared_ptr<std::vector<T> > const & value,
    size_t capacity,size_t length)
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
    size_t length = SerializeHelper::readSize(pbuffer, pcontrol);
    if(length<=0) return;
    if(length>this->getCapacity()) {
        throw std::logic_error(String("Capacity immutable"));
    }
    T * pvalue = get();
    size_t i=0;
    while(true) {
        size_t maxIndex = std::min(length-i, (pbuffer->getRemaining()/sizeof(T)))+i;
        if(shareV3Data) dbScanLock(dbAddr.precord);
        for(; i<maxIndex; i++) {
            pvalue[i] = pbuffer->GET(T);
        }
        if(shareV3Data) dbScanUnlock(dbAddr.precord);
        if(i>=length) break;
        pcontrol->ensureData(sizeof(T));
    }
    this->setLength(length);
    // update v4Record
    dbScanLock(dbAddr.precord);
    setV3Length(length);
    T * array = static_cast<T *>(dbAddr.pfield);
    for(size_t i=0; i<length; i++) array[i] = pvalue[i];
    dbScanUnlock(dbAddr.precord);
}

template<typename T>
void V3ValueArray<T>::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, size_t offset, size_t count) const
{
    size_t length = this->getLength();
    if(offset<0) offset = 0;
    if(offset>length) offset = length;
    if(count<0) count = length-offset;
    size_t maxcount = length-offset;
    if(count>maxcount) count = maxcount;
    SerializeHelper::writeSize(count, pbuffer, pflusher);
    size_t end = offset+count;
    size_t i = offset;
    T * pvalue = get();
    while(true) {
        size_t maxIndex = std::min(end-i, (pbuffer->getRemaining()/sizeof(T)))+i;
        for(; i<maxIndex; i++)
        	pbuffer->put(pvalue[i]);
        if(i>=end) break;
        pflusher->flushSerializeBuffer();
    }
}

template<typename T>
bool V3ValueArray<T>::operator==(PVField& pv)
{
    return getConvert()->equals(*this, pv);
}

template<typename T>
bool V3ValueArray<T>::operator!=(PVField& pv)
{
    return !(getConvert()->equals(*this, pv));
}

template<>
size_t V3ValueArray<String>::get(size_t offset, size_t len, PVArrayData<String> &data)
{
    dbScanLock(dbAddr.precord);
    //updateFromV3Record
    size_t length = getV3Length();
        if(length!=this->getLength()) {
        setLength(length);
    }
    char *pchar = static_cast<char *>(dbAddr.pfield);
    String *pvalue = get();
    for(size_t i=0; i<length; i++) {
        if(strcmp(pchar,pvalue[i].c_str())!=0) {
           pvalue[i] = String(pchar);
        }
        pchar += dbAddr.field_size;
    }
    dbScanUnlock(dbAddr.precord);
    size_t n = len;
    if(offset+len > length) {
        n = length-offset;
    }
    if(n<=0) return 0;
    data.data = *value.get();
    data.offset = offset;
    return n;
}

template<>
size_t V3ValueArray<String>::put(size_t offset,size_t len,
    const_pointer from,size_t fromOffset)
{
    if(PVField::isImmutable()) {
        PVField::message("field is immutable",errorMessage);
        return 0;
    }
    String * pvalue = get();
    if(from==pvalue) return len;
    size_t length = this->getLength();
    size_t capacity = this->getCapacity();
    if(offset+len > capacity) len = capacity - offset;
    if(len<1) return 0;
    if(len+offset>length) {
        length = len+offset;
        this->setLength(length);
    }
    String::size_type field_size =
    static_cast<String::size_type>(dbAddr.field_size);
    for(size_t i=offset; i<len;i++) {
        String val = from[i];
        if(val.length()>=field_size) val = val.substr(0,field_size-1);
        pvalue[i] = val;
    }
    //update V3 record
    dbScanLock(dbAddr.precord);
    setV3Length(length);
    char *pchar = static_cast<char *>(dbAddr.pfield);
    for(size_t i=0; i<length; i++) {
        strcpy(pchar,pvalue[i].c_str());
        pchar += dbAddr.field_size;
    }
    dbScanUnlock(dbAddr.precord);
    this->postPut();
    return len;
}

template<typename T>
T *V3ValueArray<T>::get()
{
     std::vector<T> *vec = value.get();
     T *praw = &((*vec)[0]);
     return praw;
}

template<typename T>
T *V3ValueArray<T>::get() const
{
     std::vector<T> *vec = value.get();
     T *praw = &((*vec)[0]);
     return praw;
}

template<>
void V3ValueArray<String>::serialize(ByteBuffer *pbuffer,
        SerializableControl *pflusher, size_t offset, size_t count) const
{
    size_t length = this->getLength();
    if(offset<0) offset = 0;
    if(offset>length) offset = length;
    if(count<0) count = length-offset;
    size_t maxcount = length-offset;
    if(count>maxcount) count = maxcount;
    SerializeHelper::writeSize(count, pbuffer, pflusher);
    size_t end = offset+count;
    String * pvalue = get();
    for(size_t i = offset; i<end; i++) {
        SerializeHelper::serializeString(pvalue[i], pbuffer, pflusher);
    }
}

template<>
void V3ValueArray<String>::deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pcontrol) {
    size_t length = SerializeHelper::readSize(pbuffer, pcontrol);
    if(length<=0) return;
    if(length>getCapacity()) {
        throw std::logic_error(String("Capacity immutable"));
    }
    String::size_type field_size =
        static_cast<String::size_type>(dbAddr.field_size);
    String *pvalue = get();
    for(size_t i = 0; i<length; i++) {
        pvalue[i] = SerializeHelper::deserializeString(pbuffer, pcontrol);
        if(pvalue[i].length()>=field_size) {
            pvalue[i] = pvalue[i].substr(0,field_size-1);
        }
    }
    this->setLength(length);
    //update V3 record
    dbScanLock(dbAddr.precord);
    setV3Length(length);
    char *pchar = static_cast<char *>(dbAddr.pfield);
    for(size_t i=0; i<length; i++) {
        strcpy(pchar,pvalue[i].c_str());
        pchar += dbAddr.field_size;
    }
    dbScanUnlock(dbAddr.precord);
}

PVByteArrayPtr V3ValueArrayCreate::createByteArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVByteArrayPtr(new V3ByteArray(parent,scalar,dbAddr,shareData));
}

PVUByteArrayPtr V3ValueArrayCreate::createUByteArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVUByteArrayPtr(new V3UByteArray(parent,scalar,dbAddr,shareData));
}


PVShortArrayPtr V3ValueArrayCreate::createShortArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVShortArrayPtr(new V3ShortArray(parent,scalar,dbAddr,shareData));
}

PVUShortArrayPtr V3ValueArrayCreate::createUShortArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVUShortArrayPtr(new V3UShortArray(parent,scalar,dbAddr,shareData));
}

PVIntArrayPtr V3ValueArrayCreate::createIntArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVIntArrayPtr(new V3IntArray(parent,scalar,dbAddr,shareData));
}

PVUIntArrayPtr V3ValueArrayCreate::createUIntArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVUIntArrayPtr(new V3UIntArray(parent,scalar,dbAddr,shareData));
}

PVFloatArrayPtr V3ValueArrayCreate::createFloatArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVFloatArrayPtr(new V3FloatArray(parent,scalar,dbAddr,shareData));
}

PVDoubleArrayPtr V3ValueArrayCreate::createDoubleArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVDoubleArrayPtr(new V3DoubleArray(parent,scalar,dbAddr,shareData));
}

PVStringArrayPtr V3ValueArrayCreate::createStringArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr)
{
    return PVStringArrayPtr(new V3StringArray(parent,scalar,dbAddr,false));
}

 
V3ValueArrayCreatePtr getV3ValueArrayCreate()
{
     static Mutex mutex;
     static V3ValueArrayCreatePtr v3ValueArrayCreate;
     Lock xx(mutex);

     if(v3ValueArrayCreate.get()==0){
          v3ValueArrayCreate = V3ValueArrayCreatePtr(new V3ValueArrayCreate());
     }
     return v3ValueArrayCreate;

}

}}
