#ifndef __DAVAENGINE_AUTOTESTING_DB_H__
#define __DAVAENGINE_AUTOTESTING_DB_H__

#include "DAVAConfig.h"

#ifdef __DAVAENGINE_AUTOTESTING__


#include "Database/MongodbClient.h"

#include "Autotesting/MongodbUpdateObject.h"
#include "Autotesting/AutotestingSystem.h"

namespace DAVA
{
class Image;
class Logger;

class AutotestingDB : public Singleton<AutotestingDB>
{
public:
    AutotestingDB();
    ~AutotestingDB();

    static const String DB_ERROR_STR_VALUE;
    static const int32 DB_ERROR_INT_VALUE = -9999;

    bool ConnectToDB(const String& collection, const String& dbName, const String& dbHost, const int32 dbPort);
    void CloseConnection();
    void FailOnLocalBuild();

    // Work with log object in DB
    KeyedArchive* FindBuildArchive(MongodbUpdateObject* dbUpdateObject, const String& auxArg);
    KeyedArchive* FindOrInsertBuildArchive(MongodbUpdateObject* dbUpdateObject, const String& auxArg);

    KeyedArchive* FindOrInsertGroupArchive(KeyedArchive* buildArchive, const String& groupId);
    KeyedArchive* InsertTestArchive(KeyedArchive* currentGroupArchive, const String& testId);
    KeyedArchive* InsertStepArchive(KeyedArchive* testArchive, const String& stepId, const String& description);

    KeyedArchive* FindOrInsertTestArchive(MongodbUpdateObject* dbUpdateObject, const String& testId);
    KeyedArchive* FindOrInsertStepArchive(KeyedArchive* testArchive, const String& stepId);
    KeyedArchive* FindOrInsertTestStepLogEntryArchive(KeyedArchive* testStepArchive, const String& logId);

    // Getting and Setting data from/in DB
    bool SaveToDB(MongodbUpdateObject* dbUpdateObject);

    void WriteLogHeader();
    void Log(const String& level, const String& message);

    String GetStringTestParameter(const String& deviceName, const String& parameter);
    int32 GetIntTestParameter(const String& deviceName, const String& parameter);

    bool SaveKeyedArchiveToDevice(const String& archiveName, KeyedArchive* archive);
    String YamlToString(const KeyedArchive* archive);

    void UploadScreenshot(const String& name, Image* image);

    // multiplayer api
    String ReadState(const String& device, const String& param);
    void WriteState(const String& device, const String& param, const String& state);

    void SetTestStarted();

    FilePath logsFolder;

protected:
    MongodbClient* dbClient = nullptr;
    FilePath logFilePath;
    AutotestingSystem* autoSys = nullptr;
    std::unique_ptr<Logger> autotestingLogger;
};
}

#endif //__DAVAENGINE_AUTOTESTING__

#endif //__DAVAENGINE_AUTOTESTING_DB_H__
