#include "Logger/Logger.h"
#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "Debug/DVAssert.h"
#include <cstdarg>
#include <array>
#include <ctime>

#include "Utils/Utils.h"
#include "Utils/StringFormat.h"
#include "Engine/Engine.h"

namespace DAVA
{
namespace
{
const size_t defaultBufferSize{ 4096 };
}

#if defined(__DAVAENGINE_WIN32__)
void Win32AttachStdoutToConsole(bool attach);
#endif

String ConvertCFormatListToString(const char8* format, va_list pargs)
{
    String dynamicbuf;
    dynamicbuf.resize(defaultBufferSize * 2);

    while (true)
    {
        va_list copy;
        va_copy(copy, pargs);
        int32 charactersWritten = vsnprintf(&dynamicbuf[0], dynamicbuf.size(), format, copy);
        va_end(copy);
        // NB. C99 (which modern Linux and OS X follow) says vsnprintf
        // failure returns the length it would have needed.  But older
        // glibc and current Windows return -1 for failure, i.e., not
        // telling us how much was needed.
        if (charactersWritten < static_cast<int32>(dynamicbuf.size()) && charactersWritten >= 0)
        {
            dynamicbuf.resize(charactersWritten);
            return dynamicbuf;
        }
        // do you really want to print 1Mb with one call may be your format
        // string incorrect?
        DVASSERT(dynamicbuf.size() < 1024 * 1024, Format("format: {%s}", format).c_str());

        dynamicbuf.resize(dynamicbuf.size() * 2);
    }
    DVASSERT(false);
    return String("never happen! ");
}

void Logger::Logv(eLogLevel ll, const char8* text, va_list li) const
{
    Logv(logFilename, ll, text, li);
}

void Logger::Logv(const FilePath& customLogFilename, eLogLevel ll, const char8* text, va_list li) const
{
    if (!text || text[0] == '\0')
        return;

    // try use stack first
    Array<char8, defaultBufferSize> stackbuf;

    va_list copy;
    va_copy(copy, li);
    int32 charactersWritten = vsnprintf(&stackbuf[0], defaultBufferSize - 1, text, copy);
    va_end(copy);

    if (charactersWritten < 0 || charactersWritten >= static_cast<int32>(defaultBufferSize - 1))
    {
        String formatedMessage = ConvertCFormatListToString(text, li);
        formatedMessage += '\n';

        Output(customLogFilename, ll, formatedMessage.c_str());
    }
    else
    {
        stackbuf[charactersWritten] = '\n';
        stackbuf[charactersWritten + 1] = '\0';

        Output(customLogFilename, ll, &stackbuf[0]);
    }
}

static const Array<const char8*, 5> logLevelString
{
  {
  "framework",
  "debug",
  "info",
  "warning",
  "error"
  }
};

Logger::Logger()
    : logLevel{ LEVEL_FRAMEWORK }
    , consoleModeEnabled{ false }
{
    SetLogFilename(String());
}

Logger::~Logger()
{
    for (auto logOutput : customOutputs)
    {
        delete logOutput;
    }

#if defined(__DAVAENGINE_WIN32__)
    Win32AttachStdoutToConsole(false);
#endif
}

Logger::eLogLevel Logger::GetLogLevel() const
{
    return logLevel;
}

const char8* Logger::GetLogLevelString(eLogLevel ll)
{
#ifndef __DAVAENGINE_WINDOWS__
    static_assert(logLevelString.size() == LEVEL__DISABLE,
                  "please update strings values");
#endif
    return logLevelString[ll];
}

Logger::eLogLevel Logger::GetLogLevelFromString(const char8* ll)
{
    for (size_t i = 0; i < logLevelString.size(); ++i)
    {
        if (strcmp(ll, logLevelString[i]) == 0)
        {
            return static_cast<eLogLevel>(i);
        }
    }
    return LEVEL__DISABLE;
}

void Logger::SetLogLevel(eLogLevel ll)
{
    logLevel = ll;
}

void Logger::Log(eLogLevel ll, const char8* text, ...) const
{
    if (ll < logLevel)
        return;

    va_list vl;
    va_start(vl, text);
    Logv(ll, text, vl);
    va_end(vl);
}

void Logger::FrameworkDebug(const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(LEVEL_FRAMEWORK, text, vl);
        va_end(vl);
    }
}

void Logger::Debug(const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(LEVEL_DEBUG, text, vl);
        va_end(vl);
    }
}

void Logger::Info(const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(LEVEL_INFO, text, vl);
        va_end(vl);
    }
}

void Logger::Warning(const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(LEVEL_WARNING, text, vl);
        va_end(vl);
    }
}

void Logger::Error(const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(LEVEL_ERROR, text, vl);
        va_end(vl);
    }
}

void Logger::FrameworkDebugToFile(const FilePath& customLogFileName, const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(customLogFileName, LEVEL_FRAMEWORK, text, vl);
        va_end(vl);
    }
}

void Logger::DebugToFile(const FilePath& customLogFileName, const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(customLogFileName, LEVEL_INFO, text, vl);
        va_end(vl);
    }
}

void Logger::InfoToFile(const FilePath& customLogFileName, const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(customLogFileName, LEVEL_INFO, text, vl);
        va_end(vl);
    }
}

void Logger::WarningToFile(const FilePath& customLogFileName, const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(customLogFileName, LEVEL_WARNING, text, vl);
        va_end(vl);
    }
}

void Logger::ErrorToFile(const FilePath& customLogFileName, const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(customLogFileName, LEVEL_ERROR, text, vl);
        va_end(vl);
    }
}

void Logger::LogToFile(const FilePath& customLogFileName, eLogLevel ll, const char8* text, ...)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log)
    {
        va_list vl;
        va_start(vl, text);
        log->Logv(customLogFileName, ll, text, vl);
        va_end(vl);
    }
}

void Logger::AddCustomOutput(DAVA::LoggerOutput* lo)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log && nullptr != lo)
        log->customOutputs.push_back(lo);
}

void Logger::RemoveCustomOutput(DAVA::LoggerOutput* lo)
{
    Logger* log = GetLoggerInstance();
    if (nullptr != log && nullptr != lo)
    {
        auto& outputs = log->customOutputs;

        outputs.erase(std::remove(outputs.begin(), outputs.end(), lo));
    }
}

void Logger::SetLogFilename(const String& filename)
{
    if (filename.empty())
    {
        logFilename = FilePath();
    }
    else
    {
        SetLogPathname(GetLogPathForFilename(filename));
    }
}

void Logger::SetLogPathname(const FilePath& filepath)
{
    const bool canWorkWithFile = CutOldLogFileIfExist(filepath);
    DVASSERT(canWorkWithFile);

    logFilename = filepath;
}

FilePath Logger::GetLogPathForFilename(const String& filename)
{
    FilePath logFilePath(FileSystem::Instance()->GetCurrentDocumentsDirectory() + filename);
    return logFilePath;
}

void Logger::SetMaxFileSize(uint32 size)
{
    cutLogSize = size;
}

DAVA::Logger* Logger::GetLoggerInstance()
{
    const EngineContext* context = GetEngineContext();
    return context ? context->logger : nullptr;
}

bool Logger::CutOldLogFileIfExist(const FilePath& logFile) const
{
    if (!logFile.Exists())
    {
        return true; // ok. No file - no questions;
    }

    // take the tail of the log file and put it to the start of the file. Cut file to size of taken tail.

    File* log = File::Create(logFile, File::OPEN | File::READ | File::WRITE);
    if (nullptr == log)
    {
        return false; // cannot open file;
    }

    SCOPE_EXIT
    {
        SafeRelease(log);
    };

    const uint32 sizeToCut = cutLogSize;

    const uint32 fileSize = static_cast<uint32>(log->GetSize());
    if (sizeToCut >= fileSize)
    {
        return true; // ok! Have less data than we should to cut.
    }

    Vector<uint8> buff(sizeToCut);
    const bool seekSuccess = log->Seek(-static_cast<int32>(sizeToCut), File::SEEK_FROM_END);
    if (!seekSuccess)
    {
        return false; // have enought data but seek error
    }

    uint32 dataReaden = log->Read(buff.data(), sizeToCut);
    if (dataReaden != sizeToCut)
    {
        return false; // have enought data but can't read
    }

    SafeRelease(log);

    File* truncatedLog = File::Create(logFile, File::CREATE | File::WRITE);
    if (nullptr == truncatedLog)
    {
        return false;
    }

    SCOPE_EXIT
    {
        SafeRelease(truncatedLog);
    };

    const uint32 dataWritten = truncatedLog->Write(buff.data(), sizeToCut);
    if (dataWritten != sizeToCut)
    {
        return false; // have correct file and data size but can't write to file.
    }

    return true; // correct;
}

void Logger::FileLog(const FilePath& customLogFileName, eLogLevel ll, const char8* text) const
{
    if (nullptr != FileSystem::Instance())
    {
        ScopedPtr<File> file(File::Create(customLogFileName, File::APPEND | File::WRITE));
        if (file)
        {
            Array<char8, 128> prefix;

            time_t timestamp = time(nullptr); //Time in UTC format
            int32 seconds = timestamp % 60;
            int32 minutes = (timestamp / 60) % 60;
            int32 hours = (timestamp / (60 * 60)) % 24;

            Snprintf(&prefix[0], prefix.size(), "%02d:%02d:%02d [%s] ", hours, minutes, seconds, GetLogLevelString(ll));
            file->Write(prefix.data(), static_cast<uint32>(strlen(prefix.data())));
            file->Write(text, static_cast<uint32>(strlen(text)));
        }
    }
}

void Logger::CustomLog(eLogLevel ll, const char8* text) const
{
    for (auto output : customOutputs)
    {
        output->Output(ll, text);
    }
}

void Logger::EnableConsoleMode()
{
    consoleModeEnabled = true;
#if defined(__DAVAENGINE_WIN32__)
    Win32AttachStdoutToConsole(true);
#endif
}

void Logger::ConsoleLog(DAVA::Logger::eLogLevel ll, const char8* text) const
{
// On mac and linux ConsoleLog and PlatformLog use the same facility for log output.
// So do nothing in ConsoleLog to prevent log messages duplication.
#if !defined(__DAVAENGINE_MACOS__) && !defined(__DAVAENGINE_LINUX__)
    printf("[%s] %s", GetLogLevelString(ll), text);
#endif
}

void Logger::Output(eLogLevel ll, const char8* formatedMsg) const
{
    Output(logFilename, ll, formatedMsg);
}

void Logger::Output(const FilePath& customLogFilename, eLogLevel ll, const char8* formatedMsg) const
{
    CustomLog(ll, formatedMsg);
    // print platform log or write log to file
    // only if log level is acceptable
    if (ll >= logLevel)
    {
        PlatformLog(ll, formatedMsg);
        if (consoleModeEnabled)
        {
            ConsoleLog(ll, formatedMsg);
        }

        if (!customLogFilename.IsEmpty())
        {
            if (customLogFilename != logFilename)
            {
                CutOldLogFileIfExist(customLogFilename);
            }
            FileLog(customLogFilename, ll, formatedMsg);
        }
    }
}

} // end namespace DAVA
