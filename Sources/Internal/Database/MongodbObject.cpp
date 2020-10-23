#include "Database/MongodbObject.h"
#include "Utils/StringFormat.h"
#include "Logger/Logger.h"
#include "mongodb/bson.h"

#define BSON_VERIFY(command) \
{\
    int32 status = command;\
    if (BSON_OK != status)\
    {  \
        Logger::Error("%s at line:%d failed with errorcode: %d", #command, __LINE__, objectData->object->err);\
    }\
}

namespace DAVA
{
class MongodbObjectInternalData : public BaseObject
{
public:
    MongodbObjectInternalData()
    {
        object = new bson();
        DVASSERT(object);

        bson_init(object);
    }

    virtual ~MongodbObjectInternalData()
    {
        bson_destroy(object);
        SafeDelete(object);
    }

    bool FindField(bson_iterator* itIn, bson_iterator* itOut, const String& fieldname, bool recursive)
    {
        bool found = false;
        while (!found && bson_iterator_next(itIn))
        {
            String itKey = String(bson_iterator_key(itIn));
            if (fieldname == itKey)
            {
                *itOut = *itIn;
                found = true;
            }
            else if ((recursive && (BSON_OBJECT == bson_iterator_type(itIn)))
                     || (recursive && (BSON_ARRAY == bson_iterator_type(itIn))))
            {
                bson_iterator subIt;
                bson_iterator_subiterator(itIn, &subIt);

                found = FindField(&subIt, itOut, fieldname, recursive);
            }
        }

        return found;
    }

    void InitWith(bson* obj)
    {
        bson_iterator it;
        bson_iterator_init(&it, obj);

        while (bson_iterator_next(&it))
        {
            const char* key = bson_iterator_key(&it);
            bson_type type = bson_iterator_type(&it);

            switch (type)
            {
            case BSON_STRING:
                bson_append_string(object, key, bson_iterator_string(&it));
                break;

            case BSON_INT:
                bson_append_int(object, key, bson_iterator_int(&it));
                break;

            case BSON_LONG:
                bson_append_long(object, key, bson_iterator_long(&it));
                break;

            case BSON_DOUBLE:
                bson_append_double(object, key, bson_iterator_double(&it));
                break;

            case BSON_OBJECT:
            {
                bson sub;

                bson_iterator_subobject(&it, &sub);
                bson_append_bson(object, key, &sub);
                break;
            }

            case BSON_OID:
                bson_append_oid(object, key, bson_iterator_oid(&it));
                break;

            default:
                DVASSERT(false);
                Logger::Error("[MongodbObjectInternalData::InitWith] Not implemented type: %d", type);
                break;
            }
        }
    }

    void InitFinished(bson* obj)
    {
        bson_copy(object, obj);
        bson_finish(object);
    }

public:
    bson* object;
};

MongodbObject::MongodbObject()
{
    objectData = new MongodbObjectInternalData();
    DVASSERT(objectData);
}

MongodbObject::~MongodbObject()
{
    SafeRelease(objectData);
}

void MongodbObject::SetUniqueObjectName()
{
    char tmp[128];
    bson_oid_t oid;

    bson_oid_gen(&oid);
    bson_oid_to_string(&oid, tmp);

    SetObjectName(tmp);
}

void MongodbObject::SetObjectName(const String& objectname)
{
    BSON_VERIFY(bson_append_string(objectData->object, String("_id").c_str(), objectname.c_str()));
}

void MongodbObject::AddInt32(const String& fieldname, int32 value)
{
    BSON_VERIFY(bson_append_int(objectData->object, fieldname.c_str(), value));
}

void MongodbObject::AddInt64(const String& fieldname, int64 value)
{
    BSON_VERIFY(bson_append_long(objectData->object, fieldname.c_str(), value));
}

void MongodbObject::AddData(const String& fieldname, uint8* data, int32 dataSize)
{
    BSON_VERIFY(bson_append_binary(objectData->object, fieldname.c_str(), BSON_BIN_BINARY, reinterpret_cast<const char*>(data), dataSize));
}

void MongodbObject::AddString(const String& fieldname, const String& value)
{
    BSON_VERIFY(bson_append_string(objectData->object, fieldname.c_str(), value.c_str()));
}

void MongodbObject::AddDouble(const String& fieldname, double value)
{
    BSON_VERIFY(bson_append_double(objectData->object, fieldname.c_str(), value));
}

void MongodbObject::AddObject(const String& fieldname, DAVA::MongodbObject* addObject)
{
    BSON_VERIFY(bson_append_bson(objectData->object, fieldname.c_str(), addObject->objectData->object));
}

void MongodbObject::Finish()
{
    BSON_VERIFY(bson_finish(objectData->object));
}

bool MongodbObject::IsFinished()
{
    return (0 != objectData->object->finished);
}

String MongodbObject::GetObjectName()
{
    return GetString(String("_id"));
}

int32 MongodbObject::GetInt32(const String& fieldname)
{
    int32 retValue = 0;

    bson_iterator it;
    bson_iterator_init(&it, objectData->object);

    bson_iterator foundIt;
    bool found = objectData->FindField(&it, &foundIt, fieldname, true);
    if (found)
    {
        retValue = bson_iterator_int(&foundIt);
    }

    return retValue;
}

int64 MongodbObject::GetInt64(const String& fieldname)
{
    int64 retValue = 0;

    bson_iterator it;
    bson_iterator_init(&it, objectData->object);

    bson_iterator foundIt;
    bool found = objectData->FindField(&it, &foundIt, fieldname, true);
    if (found)
    {
        retValue = bson_iterator_long(&foundIt);
    }

    return retValue;
}

bool MongodbObject::GetData(const String& fieldname, uint8* outData, int32 dataSize)
{
    bson_iterator it;
    bson_iterator_init(&it, objectData->object);

    bson_iterator foundIt;
    bool found = objectData->FindField(&it, &foundIt, fieldname, true);
    if (found)
    {
        const uint8* binaryData = reinterpret_cast<const uint8*>(bson_iterator_bin_data(&foundIt));
        Memcpy(outData, binaryData, dataSize);
        found = true;
    }

    return found;
}

String MongodbObject::GetString(const String& fieldname)
{
    String retValue = String("");

    bson_iterator it;
    bson_iterator_init(&it, objectData->object);

    bson_iterator foundIt;
    bool found = objectData->FindField(&it, &foundIt, fieldname, true);
    if (found)
    {
        retValue = bson_iterator_string(&foundIt);
    }

    return retValue;
}

double MongodbObject::GetDouble(const String& fieldname)
{
    double retValue = 0;

    bson_iterator it;
    bson_iterator_init(&it, objectData->object);

    bson_iterator foundIt;
    bool found = objectData->FindField(&it, &foundIt, fieldname, true);
    if (found)
    {
        retValue = bson_iterator_double(&foundIt);
    }

    return retValue;
}

void* MongodbObject::InternalObject()
{
    return objectData->object;
}

void MongodbObject::StartArray(const String& fieldname)
{
    BSON_VERIFY(bson_append_start_array(objectData->object, fieldname.c_str()));
}

void MongodbObject::FinishArray()
{
    BSON_VERIFY(bson_append_finish_array(objectData->object));
}

void MongodbObject::StartObject(const String& fieldname)
{
    BSON_VERIFY(bson_append_start_object(objectData->object, fieldname.c_str()));
}

void MongodbObject::FinishObject()
{
    BSON_VERIFY(bson_append_finish_object(objectData->object));
}

void MongodbObject::EnableForEdit()
{
    bson* newObject = new bson();
    bson_init(newObject);

    bson_iterator it;
    bson_iterator_init(&it, objectData->object);

    while (bson_iterator_next(&it))
    {
        const char* key = bson_iterator_key(&it);
        bson_type type = bson_iterator_type(&it);

        switch (type)
        {
        case BSON_STRING:
            bson_append_string(newObject, key, bson_iterator_string(&it));
            break;

        case BSON_INT:
            bson_append_int(newObject, key, bson_iterator_int(&it));
            break;

        case BSON_LONG:
            bson_append_long(newObject, key, bson_iterator_long(&it));
            break;

        case BSON_DOUBLE:
            bson_append_double(newObject, key, bson_iterator_double(&it));
            break;

        case BSON_OBJECT:

            bson sub;
            bson_iterator_subobject(&it, &sub);
            bson_append_bson(newObject, key, &sub);
            break;

        case BSON_OID:
            bson_append_oid(newObject, key, bson_iterator_oid(&it));
            break;

        default:
            break;
        }
    }

    bson_destroy(objectData->object);
    SafeDelete(objectData->object);
    objectData->object = newObject;
}

void MongodbObject::CopyFinished(MongodbObject* fromObject)
{
    objectData->InitFinished(fromObject->objectData->object);
}

void MongodbObject::Copy(MongodbObject* fromObject)
{
    objectData->InitWith(fromObject->objectData->object);
}

bool MongodbObject::GetSubObject(MongodbObject* subObject, const String& fieldname)
{
    return GetSubObject(subObject, fieldname, false);
}

bool MongodbObject::GetSubObject(MongodbObject* subObject, const String& fieldname, bool needFinished)
{
    bool found = false;

    if (0 != objectData->object->finished)
    {
        bson_iterator it;
        bson_iterator_init(&it, objectData->object);

        bson_type foundType = bson_find(&it, objectData->object, fieldname.c_str());
        if (BSON_OBJECT == foundType)
        {
            found = true;

            bson sub;
            bson_iterator_subobject(&it, &sub);

            if (needFinished)
            {
                subObject->objectData->InitFinished(&sub);
            }
            else
            {
                subObject->objectData->InitWith(&sub);
            }
        }
    }

    return found;
}

void MongodbObject::Print()
{
    if (objectData->object->finished)
    {
        bson_print(objectData->object);
    }
    else
    {
        Logger::Warning("[MongodbObject::Print] Object not finished");
    }
}
}