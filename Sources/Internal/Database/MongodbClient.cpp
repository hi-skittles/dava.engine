#include "Database/MongodbClient.h"
#include "Database/MongodbObject.h"
#include "mongodb/mongo.h"
#include "mongodb/gridfs.h"

#include "Logger/Logger.h"
#include "Utils/Utils.h"
#include "Utils/UTF8Utils.h"
#include "Utils/StringFormat.h"

#include "FileSystem/KeyedArchive.h"

namespace DAVA
{
class MongodbClientInternalData : public BaseObject
{
protected:
    virtual ~MongodbClientInternalData()
    {
        mongo_write_concern_destroy(write_concern);
        mongo_destroy(connection);
        SafeDelete(connection);
    }

public:
    MongodbClientInternalData()
    {
        connection = new mongo();
        DVASSERT(connection);

        Memset(connection, 0, sizeof(mongo));
        mongo_init(connection);

        /* Initialize the write concern object.*/
        mongo_write_concern_init(write_concern);
        write_concern->w = 1;
        mongo_write_concern_finish(write_concern);
    }

public:
    mongo* connection;
    mongo_write_concern write_concern[1];
};

MongodbClient* MongodbClient::Create(const String& ip, int32 port)
{
    MongodbClient* client = new MongodbClient();
    if (client)
    {
        bool ret = client->Connect(ip, port);
        if (!ret)
        {
            SafeRelease(client);
            Logger::Error("[MongodbClient] can't connect to database");
        }
    }
    return client;
}

MongodbClient::MongodbClient()
{
#if defined(__DAVAENGINE_WINDOWS__)
    mongo_init_sockets();
#endif //#if defined (__DAVAENGINE_WINDOWS__)

    clientData = new MongodbClientInternalData();

    SetDatabaseName(String("Database"));
    SetCollectionName(String("Collection"));
}

MongodbClient::~MongodbClient()
{
    Disconnect();

    SafeRelease(clientData);
}

bool MongodbClient::Connect(const String& ip, int32 port)
{
    int32 status = mongo_connect(clientData->connection, ip.c_str(), port);

    mongo_set_write_concern(clientData->connection, clientData->write_concern);

    if (MONGO_OK != status)
    {
        LogError(String("Connect"), clientData->connection->err);
    }

    return (MONGO_OK == status);
}

void MongodbClient::Disconnect()
{
    if (IsConnected())
    {
        mongo_disconnect(clientData->connection);
    }
}

void MongodbClient::LogError(const String functionName, int32 errorCode)
{
    static String ERROR_MESSAGES[] =
    {
      String("Connection success!"), //MONGO_CONN_SUCCESS
      String("Could not create a socket."), //MONGO_CONN_NO_SOCKET
      String("An error occured while calling connect()."), //MONGO_CONN_FAIL
      String("An error occured while calling getaddrinfo()."), //MONGO_CONN_ADDR_FAIL
      String("Warning: connected to a non-master node (read-only)."), //MONGO_CONN_NOT_MASTER
      String("Given rs name doesn't match this replica set."), //MONGO_CONN_BAD_SET_NAME
      String("Can't find primary in replica set. Connection closed."), //MONGO_CONN_NO_PRIMARY
      String("An error occurred while reading or writing on the socket."), //MONGO_IO_ERROR
      String("Other socket error."), //MONGO_SOCKET_ERROR
      String("The response is not the expected length."), //MONGO_READ_SIZE_ERROR
      String("The command returned with 'ok' value of 0."), //MONGO_COMMAND_FAILED
      String("Write with given write_concern returned an error."), //MONGO_WRITE_ERROR
      String("The name for the ns (database or collection) is invalid."), //MONGO_NS_INVALID
      String("BSON not valid for the specified op."), //MONGO_BSON_INVALID
      String("BSON object has not been finished."), //MONGO_BSON_NOT_FINISHED
      String("BSON object exceeds max BSON size."), //MONGO_BSON_TOO_LARGE
      String("Supplied write concern object is invalid.") //MONGO_WRITE_CONCERN_INVALID
    };

    DVASSERT((MONGO_CONN_SUCCESS <= errorCode) && (errorCode <= MONGO_WRITE_CONCERN_INVALID));

    Logger::Error("[MongodbClient] error %d at function %s", errorCode, functionName.c_str());
    Logger::Error("[MongodbClient] %s", ERROR_MESSAGES[errorCode].c_str());
}

void MongodbClient::SetDatabaseName(const String& newDatabase)
{
    database = newDatabase;
    namespaceName = database + String(".") + collection;
}

void MongodbClient::SetCollectionName(const String& newCollection)
{
    collection = newCollection;
    namespaceName = database + String(".") + collection;
}

void MongodbClient::DropDatabase()
{
    int32 status = mongo_cmd_drop_db(clientData->connection, database.c_str());
    if (MONGO_OK != status)
    {
        LogError(String("DropDatabase"), clientData->connection->err);
    }
}

void MongodbClient::DropCollection()
{
    int32 status = mongo_cmd_drop_collection(clientData->connection, database.c_str(), collection.c_str(), nullptr);
    if (MONGO_OK != status)
    {
        LogError(String("DropCollection"), clientData->connection->err);
    }
}

bool MongodbClient::IsConnected()
{
    int32 connectStatus = mongo_is_connected(clientData->connection);
    if (0 != connectStatus)
    {
        int32 checkStatus = mongo_check_connection(clientData->connection);
        if (MONGO_OK == checkStatus)
            return true;
    }

    Logger::Warning("[MongodbClient::IsConnected] is not connected.");
    return false;
}

bool MongodbClient::SaveBufferToGridFS(const String& name, char* buffer, uint32 length)
{
    gridfs gfs[1];
    gridfs_init(clientData->connection, database.c_str(), "fs", gfs);
    bool isOk = false;
    isOk = (MONGO_OK == gridfs_store_buffer(gfs, buffer, length, name.c_str(), nullptr));
    if (!isOk)
    {
        Logger::Error("MongodbClient::SaveBufferToGridFS failed to save %s to gridfs", name.c_str());
    }
    gridfs_destroy(gfs);
    return isOk;
}

bool MongodbClient::SaveFileToGridFS(const String& name, const String& pathToFile)
{
    gridfs gfs[1];
    gridfs_init(clientData->connection, database.c_str(), "fs", gfs);
    bool isOk = false;
    isOk = (MONGO_OK == gridfs_store_file(gfs, pathToFile.c_str(), name.c_str(), nullptr));
    if (!isOk)
    {
        Logger::Error("MongodbClient::SaveFileToGridFS failed to save %s to gridfs", name.c_str());
    }
    gridfs_destroy(gfs);
    return isOk;
}

bool MongodbClient::SaveBinary(const String& key, uint8* data, int32 dataSize)
{
    int32 status = MONGO_ERROR;
    if (IsConnected())
    {
        MongodbObject* binary = new MongodbObject();
        DVASSERT(binary);

        binary->SetObjectName(key);
        binary->AddInt32(String("DataSize").c_str(), dataSize);
        binary->AddData(String("Data").c_str(), data, dataSize);
        binary->Finish();

        MongodbObject* foundObject = FindObjectByKey(key);
        if (foundObject)
        {
            status = mongo_update(clientData->connection, namespaceName.c_str(), static_cast<bson*>(foundObject->InternalObject()), static_cast<bson*>(binary->InternalObject()), 0, nullptr);
            if (MONGO_OK != status)
            {
                LogError(String("SaveBinary, update"), clientData->connection->err);
            }

            SafeRelease(foundObject);
        }
        else
        {
            status = mongo_insert(clientData->connection, namespaceName.c_str(), static_cast<bson*>(binary->InternalObject()), nullptr);
            if (MONGO_OK != status)
            {
                LogError(String("SaveBinary, insert"), clientData->connection->err);
            }
        }

        SafeRelease(binary);
    }

    return (MONGO_OK == status);
}

int32 MongodbClient::GetBinarySize(const String& key)
{
    int32 retSize = 0;

    MongodbObject* object = FindObjectByKey(key);
    if (object)
    {
        retSize = object->GetInt32(String("DataSize"));
        SafeRelease(object);
    }
    else
    {
        Logger::Error("[MongodbClient] Can't find binary to get size.");
    }

    return retSize;
}

bool MongodbClient::GetBinary(const String& key, uint8* outData, int32 dataSize)
{
    bool found = false;

    MongodbObject* object = FindObjectByKey(key);
    if (object)
    {
        found = object->GetData(String("Data"), outData, dataSize);
        SafeRelease(object);
    }
    else
    {
        Logger::Error("[MongodbClient] Can't find binary to get data.");
    }

    return found;
}

MongodbObject* MongodbClient::FindObjectByKey(const String& key)
{
    MongodbObject* query = new MongodbObject();
    DVASSERT(query);

    query->SetObjectName(key);
    query->Finish();

    MongodbObject* foundObject = new MongodbObject();
    DVASSERT(foundObject);

    int32 status = mongo_find_one(clientData->connection, namespaceName.c_str(), static_cast<bson*>(query->InternalObject()), 0, static_cast<bson*>(foundObject->InternalObject()));
    if (MONGO_OK != status)
    {
        SafeRelease(foundObject);
        foundObject = nullptr;
    }

    SafeRelease(query);
    return foundObject;
}

bool MongodbClient::FindObjectByKey(const String& key, MongodbObject* foundObject)
{
    DVASSERT(foundObject);

    ScopedPtr<MongodbObject> query(new MongodbObject());

    query->SetObjectName(key);
    query->Finish();

    int32 status = mongo_find_one(clientData->connection, namespaceName.c_str(), static_cast<bson*>(query->InternalObject()), 0, static_cast<bson*>(foundObject->InternalObject()));
    if (MONGO_OK != status)
    {
        return false;
    }

    return true;
}

bool MongodbClient::SaveDBObject(MongodbObject* object)
{
    int32 status = MONGO_ERROR;
    if (IsConnected())
    {
        MongodbObject* foundObject = FindObjectByKey(object->GetObjectName());
        if (foundObject)
        {
            status = mongo_update(clientData->connection, namespaceName.c_str(), static_cast<bson*>(foundObject->InternalObject()), static_cast<bson*>(object->InternalObject()), 0, nullptr);
            if (MONGO_OK != status)
            {
                LogError(String("SaveObject, update"), clientData->connection->err);
            }

            SafeRelease(foundObject);
        }
        else
        {
            status = mongo_insert(clientData->connection, namespaceName.c_str(), static_cast<bson*>(object->InternalObject()), nullptr);
            if (MONGO_OK != status)
            {
                LogError(String("SaveObject, insert"), clientData->connection->err);
            }
        }
    }

    Logger::FrameworkDebug("MongodbClient::SaveObject status = %d", status);
    return (MONGO_OK == status);
}

bool MongodbClient::SaveDBObject(MongodbObject* newObject, MongodbObject* oldObject)
{
    int32 status = MONGO_ERROR;
    if (IsConnected())
    {
        if (oldObject)
        {
            status = mongo_update(clientData->connection, namespaceName.c_str(), static_cast<bson*>(oldObject->InternalObject()), static_cast<bson*>(newObject->InternalObject()), 0, nullptr);
            if (MONGO_OK != status)
            {
                LogError(String("SaveObject, update"), clientData->connection->err);
            }
        }
        else
        {
            status = mongo_insert(clientData->connection, namespaceName.c_str(), static_cast<bson*>(newObject->InternalObject()), nullptr);
            if (MONGO_OK != status)
            {
                LogError(String("SaveObject, insert"), clientData->connection->err);
            }
        }
    }

    return (MONGO_OK == status);
}

void MongodbClient::DumpDB()
{
    Logger::FrameworkDebug("***** MONGO DUMP *******");

    bson query;
    bson_empty(&query);

    mongo_cursor* cursor = mongo_find(clientData->connection, namespaceName.c_str(), &query, nullptr, 0, 0, 0);
    int32 count = 0;
    while (mongo_cursor_next(cursor) == MONGO_OK)
    {
        const bson* currentObject = mongo_cursor_bson(cursor);

        Logger::FrameworkDebug(Format("BSON[%d]:", count).c_str());
        bson_print(currentObject);

        ++count;
    }

    mongo_cursor_destroy(cursor);

    Logger::FrameworkDebug("Count: %d", count);
    Logger::FrameworkDebug("************************");
}

bool MongodbClient::KeyedArchiveToDBObject(KeyedArchive* archive, MongodbObject* outObject)
{
    DVASSERT(archive && outObject);

    if (!outObject->IsFinished())
    {
        //copy data from archive into db object
        const KeyedArchive::UnderlyingMap& archiveData = archive->GetArchieveData();
        for (auto it = archiveData.begin(); it != archiveData.end(); ++it)
        {
            MongodbClient::WriteData(outObject, it->first, it->second);
        }
        return true;
    }
    return false;
}

bool MongodbClient::DBObjectToKeyedArchive(MongodbObject* dbObject, KeyedArchive* outArchive)
{
    DVASSERT(dbObject && outArchive);

    if (dbObject->IsFinished())
    {
        //copy data from db object into archive
        ReadData(outArchive, static_cast<bson*>(dbObject->InternalObject()));

        return true;
    }

    DVASSERT(false);

    return false;
}

void MongodbClient::ReadData(KeyedArchive* archive, void* bsonObj)
{
    if ((!archive) || (!bsonObj))
        return;

    bson_iterator it;
    bson_iterator_init(&it, static_cast<bson*>(bsonObj));

    while (bson_iterator_next(&it))
    {
        String key = String(bson_iterator_key(&it));
        bson_type type = bson_iterator_type(&it);

        if (key == "_id")
            continue; // ignore _id

        switch (type)
        {
        case BSON_STRING:
            archive->SetString(key, String(bson_iterator_string(&it)));
            break;

        case BSON_INT:
            archive->SetInt32(key, bson_iterator_int(&it));
            break;

        case BSON_LONG:
            archive->SetInt32(key, static_cast<int32>(bson_iterator_long(&it)));
            break;

        case BSON_DOUBLE:
            archive->SetFloat(key, static_cast<float32>(bson_iterator_double(&it)));
            break;

        case BSON_OBJECT:
        {
            bson sub;

            bson_iterator_subobject(&it, &sub);

            KeyedArchive* subArchive = new KeyedArchive();
            ReadData(subArchive, &sub);
            archive->SetArchive(key, subArchive);
            SafeRelease(subArchive);
            break;
        }

        case BSON_NULL:
        {
            Logger::Warning("[MongodbClient::ReadData] Get NULL type by key: %s", key.c_str());
            break;
        }

        case BSON_DATE:
        {
            int oneSecond = 1000;
            time_t rawTime = static_cast<time_t>(bson_iterator_date(&it) / oneSecond);
            tm tms = { 0 };
#if defined(__DAVAENGINE_WINDOWS__)
            gmtime_s(&tms, &rawTime);
#else // __DAVAENGINE_WINDOWS__
            gmtime_r(&rawTime, &tms);
#endif // __DAVAENGINE_WINDOWS__
            Array<char8, 50> buf = { { 0 } };
            strftime(buf.data(), buf.size(), "%d.%b.%Y_%H-%M-%S", &tms);
            archive->SetString(key, String(buf.data()));
            break;
        }

        case BSON_OID:
            //TODO: add 12-bytes array
            //bson_append_oid(object, key, bson_iterator_oid(&it));
            break;

        default:
            Logger::Error("[MongodbClient::ReadData] Not implemented type: %d", type);
            DVASSERT(false);
            break;
        }
    }
}

void MongodbClient::WriteData(MongodbObject* mongoObj, const String& key, VariantType* value)
{
    if (mongoObj && value)
    {
        if (key == "_id")
            return; // ignore _id

        //TODO: bool, uint32 and WideString have no corresponding types in mongoDB
        switch (value->GetType())
        {
        case VariantType::TYPE_BOOLEAN:
        {
            mongoObj->AddInt32(key, value->AsBool());
        }
        break;
        case VariantType::TYPE_INT8:
        {
            mongoObj->AddInt32(key, value->AsInt8());
        }
        break;
        case VariantType::TYPE_UINT8:
        {
            mongoObj->AddInt32(key, value->AsUInt8());
        }
        break;
        case VariantType::TYPE_INT16:
        {
            mongoObj->AddInt32(key, value->AsInt16());
        }
        break;
        case VariantType::TYPE_UINT16:
        {
            mongoObj->AddInt32(key, value->AsUInt16());
        }
        break;
        case VariantType::TYPE_INT32:
        {
            mongoObj->AddInt32(key, value->AsInt32());
        }
        break;
        case VariantType::TYPE_UINT32:
        {
            mongoObj->AddInt32(key, value->AsUInt32());
        }
        break;
        case VariantType::TYPE_FLOAT:
        {
            mongoObj->AddDouble(key, value->AsFloat());
        }
        break;
        case VariantType::TYPE_FLOAT64:
        {
            mongoObj->AddDouble(key, value->AsFloat64());
        }
        break;
        case VariantType::TYPE_STRING:
        {
            mongoObj->AddString(key, value->AsString());
        }
        break;
        case VariantType::TYPE_WIDE_STRING:
        {
            mongoObj->AddString(key, UTF8Utils::EncodeToUTF8(value->AsWideString()));
        }
        break;
        case VariantType::TYPE_BYTE_ARRAY:
        {
            mongoObj->AddData(key, const_cast<uint8*>(value->AsByteArray()), value->AsByteArraySize());
        }
        break;
        case VariantType::TYPE_KEYED_ARCHIVE:
        {
            MongodbObject* subObject = new MongodbObject();
            KeyedArchive* subArchive = value->AsKeyedArchive();

            KeyedArchiveToDBObject(subArchive, subObject);

            subObject->SetObjectName(key);
            subObject->Finish();

            mongoObj->AddObject(key, subObject);
            SafeRelease(subObject);
        }
        break;
        default:
        {
            DVASSERT(false);
            Logger::Error("[MongodbUpdateObject::WriteData] Not implemented type: %d", value->GetType());
        }
        break;
        }
    }
}
}
