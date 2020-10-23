#ifndef __DAVAENGINE_MONGODB_OBJECT_H__
#define __DAVAENGINE_MONGODB_OBJECT_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"

namespace DAVA
{
/** 
 \ingroup Mongodb
 \brief this class is mongodb object and it used if you want to work with mongodb
 */
class MongodbClient;
class MongodbObjectInternalData;
class MongodbObject : public BaseObject
{
    friend class MongodbClient;

protected:
    void* InternalObject();
    virtual ~MongodbObject();

public:
    MongodbObject();

    void EnableForEdit();
    void Finish();
    bool IsFinished();

    void SetUniqueObjectName();
    void SetObjectName(const String& objectname);
    String GetObjectName();

    void AddInt32(const String& fieldname, int32 value);
    int32 GetInt32(const String& fieldname);

    void AddInt64(const String& fieldname, int64 value);
    int64 GetInt64(const String& fieldname);

    void AddData(const String& fieldname, uint8* data, int32 dataSize);
    bool GetData(const String& fieldname, uint8* outData, int32 dataSize);

    void AddString(const String& fieldname, const String& value);
    String GetString(const String& fieldname);

    void AddDouble(const String& fieldname, double value);
    double GetDouble(const String& fieldname);

    void AddObject(const String& fieldname, MongodbObject* addObject);

    bool GetSubObject(MongodbObject* subObject, const String& fieldname);
    bool GetSubObject(MongodbObject* subObject, const String& fieldname, bool needFinished);

    void StartArray(const String& fieldname);
    void FinishArray();

    void StartObject(const String& fieldname);
    void FinishObject();

    void CopyFinished(MongodbObject* fromObject);

    void Copy(MongodbObject* fromObject);

    void Print();

protected:
    MongodbObjectInternalData* objectData;
};
};

#endif // __DAVAENGINE_MONGODB_OBJECT_H__
