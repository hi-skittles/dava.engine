#include "Logger/RotationLogger.h"

#include <Engine/EngineContext.h>
#include <Time/DateTime.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/FilePath.h>
#include <Utils/StringFormat.h>

namespace ServerLoggerDetails
{
const DAVA::uint32 FILE_MAX_SIZE = 30 * 1024 * 1024;
const DAVA::uint32 FILES_COUNT = 10;
const size_t PREFIX_MAX_LENGTH = 128;
}

RotationLogger::RotationLogger(const DAVA::EngineContext* context)
    : engineContext(context)
{
    engineContext->logger->AddCustomOutput(this);
}

RotationLogger::~RotationLogger()
{
    engineContext->logger->RemoveCustomOutput(this);
}

void RotationLogger::SetLogPath(const DAVA::FilePath& path)
{
    GenerateLogPathNames(path);
    CreateDirectoriesForPath(path);
    logPath = logPathNames[0];
}

void RotationLogger::SetLogLevel(DAVA::Logger::eLogLevel level)
{
    levelAllowed = level;
}

void RotationLogger::Output(DAVA::Logger::eLogLevel logLevel, const DAVA::char8* text)
{
    using namespace DAVA;
    using namespace ServerLoggerDetails;

    if (logLevel < levelAllowed)
    {
        return;
    }

    FileSystem* fs = engineContext->fileSystem;
    if (fs)
    {
        uint64 fileSize = 0;
        if (fs->GetFileSize(logPath, fileSize) && fileSize > FILE_MAX_SIZE)
        {
            ShiftLogFiles();
        }

        ScopedPtr<File> file(File::Create(logPath, File::APPEND | File::WRITE));
        if (file)
        {
            Array<char8, PREFIX_MAX_LENGTH> prefix;
            DateTime dt = DateTime::Now();
            Snprintf(prefix.data(), prefix.size(), "%02i/%02i %02i:%02i:%02i [%s] ", dt.GetDay(), dt.GetMonth() + 1, dt.GetHour(), dt.GetMinute(), dt.GetSecond(), Logger::GetLogLevelString(logLevel));
            file->Write(prefix.data(), static_cast<uint32>(strlen(prefix.data())));
            file->Write(text, static_cast<uint32>(strlen(text)));
            file->WriteLine(""); // inserting \r\n
        }
    }
}

void RotationLogger::GenerateLogPathNames(const DAVA::FilePath& path)
{
    using namespace DAVA;

    FilePath logPathWithoutExt = FilePath::CreateWithNewExtension(path, "");
    String logExt = path.GetExtension();

    logPathNames.reserve(ServerLoggerDetails::FILES_COUNT);
    for (uint32 i = 0; i < ServerLoggerDetails::FILES_COUNT; ++i)
    {
        String suffix = (i > 0) ? Format(".%02u", i) : "";
        logPathNames.emplace_back(logPathWithoutExt + suffix + logExt);
    };
}

void RotationLogger::CreateDirectoriesForPath(const DAVA::FilePath& path)
{
    DAVA::FileSystem* fs = engineContext->fileSystem;
    if (fs)
    {
        fs->CreateDirectory(path.GetDirectory(), true);
    }
}

void RotationLogger::ShiftLogFiles()
{
    DAVA::FileSystem* fs = engineContext->fileSystem;
    if (fs)
    {
        // log.09.txt -> log.10.txt
        //.....
        // log.txt > log.01.txt
        for (DAVA::uint32 i = ServerLoggerDetails::FILES_COUNT - 1; i > 0; --i)
        {
            if (fs->Exists(logPathNames[i - 1]))
                fs->MoveFile(logPathNames[i - 1], logPathNames[i], true);
        }
    }
}
