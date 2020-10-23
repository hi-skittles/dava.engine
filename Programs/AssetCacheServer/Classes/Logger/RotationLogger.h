#pragma once

#include <Logger/Logger.h>
#include <FileSystem/FilePath.h>

namespace DAVA
{
class EngineContext;
}

class RotationLogger : public DAVA::LoggerOutput
{
public:
    RotationLogger(const DAVA::EngineContext*);
    ~RotationLogger();

    void SetLogPath(const DAVA::FilePath& logFilePath);
    void SetLogLevel(DAVA::Logger::eLogLevel);

    void Output(DAVA::Logger::eLogLevel ll, const DAVA::char8* text) override;

private:
    void GenerateLogPathNames(const DAVA::FilePath& path);
    void CreateDirectoriesForPath(const DAVA::FilePath& path);
    void ShiftLogFiles();

private:
    const DAVA::EngineContext* engineContext = nullptr;
    DAVA::FilePath logPath;
    DAVA::Vector<DAVA::FilePath> logPathNames;
    DAVA::Logger::eLogLevel levelAllowed = DAVA::Logger::eLogLevel::LEVEL__DISABLE;
};
