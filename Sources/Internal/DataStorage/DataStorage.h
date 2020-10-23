#ifndef __DATA_STORAGE_H__
#define __DATA_STORAGE_H__

#include "Base/BaseObject.h"

namespace DAVA
{
// we suppose that we can have a couple of Storages for windows Regystry or MacOS file system, or some new cloud API
class IDataStorage : public BaseObject
{
public:
    virtual String GetStringValue(const String& key) = 0;
    virtual int64 GetLongValue(const String& key) = 0;
    virtual void SetStringValue(const String& key, const String& value) = 0;
    virtual void SetLongValue(const String& key, int64 value) = 0;
    virtual void RemoveEntry(const String& key) = 0;
    virtual void Clear() = 0;
    virtual void Push() = 0;
};

// when we have a couple of DataStorages - we could change constructor and add some parameter to determine impl type.
class DataStorage
{
public:
    static IDataStorage* Create();

private:
    DataStorage(){};
    ~DataStorage(){};
};

} //namespace DAVA

#endif // __DATA_STORAGE_H__
