#ifndef __DAVAENGINE_LOGGER_H__
#define __DAVAENGINE_LOGGER_H__

#define LOG_AS_INT(param) Logger::Debug("%s = %d", #param, param);
#define LOG_AS_FLOAT(param) Logger::Debug("%s = %.4f", #param, param);
#define LOG_AS_BOOL(param) Logger::Debug("%s = %s", #param, param ? "true" : "false");
#define LOG_AS_MATRIX4(param)   Logger::Debug("%s_0 = %f, %f, %f, %f", #param, param._00, param._01, param._02, param._03); \
                                Logger::Debug("%s_1 = %f, %f, %f, %f", #param, param._10, param._11, param._12, param._13); \
                                Logger::Debug("%s_2 = %f, %f, %f, %f", #param, param._20, param._21, param._22, param._23); \
                                Logger::Debug("%s_2 = %f, %f, %f, %f", #param, param._30, param._31, param._32, param._33);

#include "Base/BaseTypes.h"

#include "FileSystem/FilePath.h"

#include <cstdarg>

namespace DAVA
{
class LoggerOutput;

class Logger
{
public:
    enum eLogLevel
    {
        LEVEL_FRAMEWORK = 0, //<! Designates fine-grained informational events to debug an engine.
        LEVEL_DEBUG, //<! Designates fine-grained informational events to debug an application.
        LEVEL_INFO, //<! Designates informational messages that highlight the progress of the application at coarse-grained level.
        LEVEL_WARNING, //<! Designates potentially harmful situations.
        LEVEL_ERROR, //<! Designates error events that might still allow the application to continue running or can lead the application to abort.

        LEVEL__DISABLE //<! Disable logs.
    };

    Logger();
    virtual ~Logger();

    //! Enables/disables logging to file. Disabled by default.
    //! \param[in] filename - name of log file. Empty string disables logging to file,
    //! non-empty creates log file in working directory.
    virtual void SetLogFilename(const String& filename);

    //! Enables/disables logging to file. Disabled by default.
    //! \param[in] filepath - path to log file. Empty string disables logging to file,
    //! non-empty creates log file described by filepath.
    virtual void SetLogPathname(const FilePath& filepath);

    //! Returns the current set log level.
    virtual eLogLevel GetLogLevel() const;

    //! Sets a new log level. With this value, texts which are sent to
    //! the logger are filtered out. For example setting this value to
    //! ELL_WARNING, only warnings and
    //! errors are printed out. Setting it to ELL_INFORMATION, which is
    //! the default setting, warnings,
    //! errors and informational texts are printed out.
    //! \param ll - new log level filter value.
    virtual void SetLogLevel(eLogLevel ll);

    //! Prints out a text into the log
    //! \param text - Text to print out.
    //! \param ll - Log level of the text. If the text is an error, set
    //! it to ELL_ERROR, if it is warning set it to ELL_WARNING, and if it
    //! is just an informational text, set it to ELL_INFORMATION. Texts are
    //! filtered with these levels. If you want to be a text displayed,
    //! independent on what level filter is set, use ELL_NONE.
    virtual void Log(eLogLevel ll, const char8* text, ...) const;
    virtual void Logv(eLogLevel ll, const char8* text, va_list li) const;
    virtual void Logv(const FilePath& customLogFilename, eLogLevel ll, const char8* text, va_list li) const;

    /**
        Write message text to platfrom specific logging facility.

        This method can be used even when Logger is not instantiated.
        Newline character is not appended to message text.
    */
    static void PlatformLog(eLogLevel ll, const char8* text);

    //** Log event with LEVEL_FRAMEWORK level. */
    static void FrameworkDebug(const char8* text, ...);

    //** Log event with LEVEL_DEBUG level. */
    static void Debug(const char8* text, ...);

    //** Log event with LEVEL_INFO level. */
    static void Info(const char8* text, ...);

    //** Log event with LEVEL_WARNING level. */
    static void Warning(const char8* text, ...);

    //** Log event with LEVEL_ERROR level. */
    static void Error(const char8* text, ...);

    // logs which writes to the given file
    static void FrameworkDebugToFile(const FilePath& customLogFileName, const char8* text, ...);
    static void DebugToFile(const FilePath& customLogFileName, const char8* text, ...);
    static void WarningToFile(const FilePath& customLogFileName, const char8* text, ...);
    static void InfoToFile(const FilePath& customLogFileName, const char8* text, ...);
    static void ErrorToFile(const FilePath& customLogFileName, const char8* text, ...);
    static void LogToFile(const FilePath& customLogFileName, eLogLevel ll, const char8* text, ...);

    static void AddCustomOutput(DAVA::LoggerOutput* lo);
    static void RemoveCustomOutput(DAVA::LoggerOutput* lo);

#if defined(__DAVAENGINE_ANDROID__)
    static void SetTag(const char8* logTag);
#endif

    static FilePath GetLogPathForFilename(const String& filename);
    void SetMaxFileSize(uint32 size);
    void EnableConsoleMode();

    static const char8* GetLogLevelString(eLogLevel ll);
    //TODO: insert Optional
    static eLogLevel GetLogLevelFromString(const char8* ll);

private:
    static Logger* GetLoggerInstance();
    bool CutOldLogFileIfExist(const FilePath& logFile) const;

    void FileLog(const FilePath& filepath, eLogLevel ll, const char8* text) const;
    void CustomLog(eLogLevel ll, const char8* text) const;
    void ConsoleLog(eLogLevel ll, const char8* text) const;
    void Output(eLogLevel ll, const char8* formatedMsg) const;
    void Output(const FilePath& customLogFilename, eLogLevel ll, const char8* formatedMsg) const;

    eLogLevel logLevel;
    FilePath logFilename;
    Vector<LoggerOutput*> customOutputs;
    bool consoleModeEnabled;
    uint32 cutLogSize = 512 * 1024; //0.5 MB;
};

class LoggerOutput
{
public:
    LoggerOutput() = default;

    virtual void Output(Logger::eLogLevel ll, const char8* text) = 0;

protected:
    friend class DAVA::Logger;
    virtual ~LoggerOutput() = default;
};

} // end namespace DAVA

#endif // __DAVAENGINE_LOGGER_H__
