#include "Autotesting/MongodbUpdateObject.h"

#ifdef __DAVAENGINE_AUTOTESTING__

#include "Logger/Logger.h"
#include "Database/MongodbClient.h"

namespace DAVA
{
MongodbUpdateObject::MongodbUpdateObject()
    : MongodbObject()
    , updateObject(NULL)
    , updateData(new KeyedArchive())
{
}

MongodbUpdateObject::~MongodbUpdateObject()
{
    SafeRelease(updateObject);
    SafeRelease(updateData);
}

bool MongodbUpdateObject::LoadData()
{
    SafeRelease(updateData);
    updateData = new KeyedArchive();
    Finish();

    return MongodbClient::DBObjectToKeyedArchive(this, updateData);
}

bool MongodbUpdateObject::SaveData()
{
    Logger::Debug("MongodbUpdateObject::SaveData");
    SafeRelease(updateObject);
    updateObject = new MongodbObject();
    updateObject->SetObjectName(GetObjectName());
    return MongodbClient::KeyedArchiveToDBObject(updateData, updateObject);
}

KeyedArchive* MongodbUpdateObject::GetData()
{
    return updateData;
}

MongodbObject* MongodbUpdateObject::GetUpdateObject()
{
    return updateObject;
}

bool MongodbUpdateObject::SaveToDB(MongodbClient* dbClient)
{
    if (!updateObject)
    {
        if (!SaveData())
        {
            Logger::Error("MongodbUpdateObject::SaveToDB SaveData failed");
        }
    }

    if (dbClient && updateObject)
    {
        updateObject->Finish();

        //Logger::Debug("MongodbUpdateObject::SaveToDB old object:");
        //Print();

        //Logger::Debug("MongodbUpdateObject::SaveToDB new object:");
        //updateObject->Print();
        Logger::Debug("MongodbUpdateObject::SaveToDB return dbClient->SaveObject");
        return dbClient->SaveDBObject(updateObject);
    }
    Logger::Error("MongodbUpdateObject::SaveToDB return false");
    return false;
}
};

#endif
