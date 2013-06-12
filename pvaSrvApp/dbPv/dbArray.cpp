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

#include <dbAccess.h>

#include <pv/lock.h>
#include <pv/serializeHelper.h>
#include <pv/convert.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>

#include "dbPv.h"
#include "dbArray.h"
#include "dbUtil.h"

namespace epics { namespace pvaSrv { 

using namespace epics::pvData;

template<typename T>
class dbArray : public epics::pvData::PVValueArray<T> {
public:
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef std::vector<T> vector;
    typedef const std::vector<T> const_vector;
    typedef std::tr1::shared_ptr<vector> shared_vector;

    dbArray(
        PVStructurePtr const & parent,
        ScalarArrayConstPtr const & scalar,
        DbAddr &dbAddr,bool shareData);
    virtual ~dbArray();
    virtual void setCapacity(size_t capacity);
    virtual size_t get(size_t offset, size_t length, PVArrayData<T> &data);
    virtual size_t put(size_t offset,size_t length, const_pointer from, size_t fromOffset);
    virtual void shareData(
         std::tr1::shared_ptr<std::vector<T> > const & value,
         size_t capacity,size_t length);
    virtual pointer get() ;
    virtual pointer get() const ;
    virtual vector const & getVector() { return *value.get(); }
    virtual shared_vector const & getSharedVector() { return value; }
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
    virtual bool operator==(PVField& pv);
    virtual bool operator!=(PVField& pv);
private:
    long getDbLength() const;
    void setDbLength(long length) const;
    DbAddr &dbAddr;
    bool shareDbData;
    shared_vector value;
};

typedef dbArray<int8> dbByteArray;
typedef dbArray<uint8> dbUByteArray;
typedef dbArray<int16> dbShortArray;
typedef dbArray<uint16> dbUShortArray;
typedef dbArray<int32> dbIntArray;
typedef dbArray<uint32> dbUIntArray;
typedef dbArray<float> dbFloatArray;
typedef dbArray<double> dbDoubleArray;
typedef dbArray<String> dbStringArray;

template<typename T>
dbArray<T>::dbArray(
    PVStructurePtr const & parent,ScalarArrayConstPtr const &scalarArray,
    DbAddr &dbAddr,bool shareData)
: PVValueArray<T>(scalarArray),
  dbAddr(dbAddr),
  shareDbData(false),
  value(std::tr1::shared_ptr<std::vector<T> >(new std::vector<T>()))
{
    size_t capacity = dbAddr.no_elements;
    value->resize(capacity);
    if(shareDbData) {
        parent->message("shareDbData not implemented", warningMessage);
    }
    size_t length = getDbLength();
    T* v3data = static_cast<T *>(dbAddr.pfield);
    T * data = get();
    for(size_t i=0; i<capacity; i++) data[i] = v3data[i];
    PVArray::setCapacityLength(capacity,length);
    PVArray::setCapacityMutable(false);
}

template<typename T>
long dbArray<T>::getDbLength() const
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
void dbArray<T>::setDbLength(long len) const
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
dbArray<T>::~dbArray()
{}

template<typename T>
void dbArray<T>::setCapacity(size_t capacity)
{}

template<typename T>
size_t dbArray<T>::get(size_t offset, size_t len, PVArrayData<T> &data)
{
    dbScanLock(dbAddr.precord);
    // updateFromDbRecord();
    size_t length = getDbLength();
    if(length!=this->getLength()) {
        this->setLength(length);
    }
    T * pvalue = get();
    if(!shareDbData) {
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
size_t dbArray<T>::put(size_t offset,size_t len,
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
    // update DB Record
    setDbLength(length);
    if(!shareDbData) {
        T * array = static_cast<T *>(dbAddr.pfield);
        for(size_t i=0; i<length; i++) array[i] = pvalue[i];
    }
    dbScanUnlock(dbAddr.precord);
    this->postPut();
    return len;
}

template<typename T>
void dbArray<T>::shareData(
    std::tr1::shared_ptr<std::vector<T> > const & value,
    size_t capacity,size_t length)
{
    throw std::logic_error(String("Not Implemented"));
}

template<typename T>
void dbArray<T>::serialize(ByteBuffer *pbuffer,
            SerializableControl *pflusher) const
{
    serialize(pbuffer, pflusher, 0, this->getLength());
}

template<typename T>
void dbArray<T>::deserialize(ByteBuffer *pbuffer,
        DeserializableControl *pcontrol)
{
    size_t length = SerializeHelper::readSize(pbuffer, pcontrol);
	T * pvalue = get();
    if(length>0) {
		if(length>this->getCapacity()) {
			throw std::logic_error(String("Capacity immutable"));
		}
		size_t i=0;
		while(true) {
			size_t maxIndex = std::min(length-i, (pbuffer->getRemaining()/sizeof(T)))+i;
            if(shareDbData) dbScanLock(dbAddr.precord);
			for(; i<maxIndex; i++) {
				pvalue[i] = pbuffer->GET(T);
			}
            if(shareDbData) dbScanUnlock(dbAddr.precord);
			if(i>=length) break;
			pcontrol->ensureData(sizeof(T));
		}
    }
    this->setLength(length);
    // update DB Record
    dbScanLock(dbAddr.precord);
    setDbLength(length);
    T * array = static_cast<T *>(dbAddr.pfield);
    for(size_t i=0; i<length; i++) array[i] = pvalue[i];
    dbScanUnlock(dbAddr.precord);
}

template<typename T>
void dbArray<T>::serialize(ByteBuffer *pbuffer,
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
bool dbArray<T>::operator==(PVField& pv)
{
    return getConvert()->equals(*this, pv);
}

template<typename T>
bool dbArray<T>::operator!=(PVField& pv)
{
    return !(getConvert()->equals(*this, pv));
}

template<>
dbArray<String>::dbArray(
    PVStructurePtr const & parent,ScalarArrayConstPtr const &scalarArray,
    DbAddr &dbAddr,bool shareData)
: PVValueArray<String>(scalarArray),
  dbAddr(dbAddr),
  shareDbData(false),
  value(std::tr1::shared_ptr<std::vector<String> >(new std::vector<String>()))
{
    size_t capacity = dbAddr.no_elements;
    value->resize(capacity);
    if(shareDbData) {
        parent->message("shareV3Data not implemented",warningMessage);
    }
    size_t length = getDbLength();
    char *pchar = static_cast<char *>(dbAddr.pfield);
    String *pvalue = get();
    for(size_t i=0; i<length; i++) {
        if(strcmp(pchar,pvalue[i].c_str())!=0) {
           pvalue[i] = String(pchar);
        }
        pchar += dbAddr.field_size;
    }
    PVArray::setCapacityLength(capacity,length);
    PVArray::setCapacityMutable(false);
}


template<>
size_t dbArray<String>::get(size_t offset, size_t len, PVArrayData<String> &data)
{
    dbScanLock(dbAddr.precord);
    // update from DB Record
    size_t length = getDbLength();
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
size_t dbArray<String>::put(size_t offset,size_t len,
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
    // update DB record
    dbScanLock(dbAddr.precord);
    setDbLength(length);
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
T *dbArray<T>::get()
{
     std::vector<T> *vec = value.get();
     T *praw = &((*vec)[0]);
     return praw;
}

template<typename T>
T *dbArray<T>::get() const
{
     std::vector<T> *vec = value.get();
     T *praw = &((*vec)[0]);
     return praw;
}

template<>
void dbArray<String>::serialize(ByteBuffer *pbuffer,
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
void dbArray<String>::deserialize(ByteBuffer *pbuffer,
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
    // update DB record
    dbScanLock(dbAddr.precord);
    setDbLength(length);
    char *pchar = static_cast<char *>(dbAddr.pfield);
    for(size_t i=0; i<length; i++) {
        strcpy(pchar,pvalue[i].c_str());
        pchar += dbAddr.field_size;
    }
    dbScanUnlock(dbAddr.precord);
}

PVByteArrayPtr dbArrayCreate::createByteArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVByteArrayPtr(new dbByteArray(parent,scalar,dbAddr,shareData));
}

PVUByteArrayPtr dbArrayCreate::createUByteArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVUByteArrayPtr(new dbUByteArray(parent,scalar,dbAddr,shareData));
}


PVShortArrayPtr dbArrayCreate::createShortArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVShortArrayPtr(new dbShortArray(parent,scalar,dbAddr,shareData));
}

PVUShortArrayPtr dbArrayCreate::createUShortArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVUShortArrayPtr(new dbUShortArray(parent,scalar,dbAddr,shareData));
}

PVIntArrayPtr dbArrayCreate::createIntArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVIntArrayPtr(new dbIntArray(parent,scalar,dbAddr,shareData));
}

PVUIntArrayPtr dbArrayCreate::createUIntArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVUIntArrayPtr(new dbUIntArray(parent,scalar,dbAddr,shareData));
}

PVFloatArrayPtr dbArrayCreate::createFloatArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVFloatArrayPtr(new dbFloatArray(parent,scalar,dbAddr,shareData));
}

PVDoubleArrayPtr dbArrayCreate::createDoubleArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr,
    bool shareData)
{
    return PVDoubleArrayPtr(new dbDoubleArray(parent,scalar,dbAddr,shareData));
}

PVStringArrayPtr dbArrayCreate::createStringArray(
    PVStructurePtr const & parent,
    ScalarArrayConstPtr const & scalar,
    DbAddr &dbAddr)
{
    return PVStringArrayPtr(new dbStringArray(parent,scalar,dbAddr,false));
}
 
dbArrayCreatePtr getDbValueArrayCreate()
{
     static Mutex mutex;
     static dbArrayCreatePtr dbValueArrayCreate;
     Lock xx(mutex);

     if(dbValueArrayCreate.get()==0){
          dbValueArrayCreate = dbArrayCreatePtr(new dbArrayCreate());
     }
     return dbValueArrayCreate;

}

}}
