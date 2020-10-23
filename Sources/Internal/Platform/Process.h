#ifndef __DAVAENGINE_PROCESS_H__
#define __DAVAENGINE_PROCESS_H__

#include "Base/BaseTypes.h"
#include "Base/Message.h"
#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)

namespace DAVA
{
class Process
{
public:
    Process(const FilePath& path, const Vector<String>& args);
    ~Process();

    bool Run(bool showWindow);
    void Wait();

    const String& GetOutput() const;
    int64 GetPID() const;
    const FilePath& GetPath() const;
    const Vector<String>& GetArgs() const;

    int GetExitCode() const;

private:
    void CleanupHandles();

#if defined(__DAVAENGINE_WIN32__) && defined(UNICODE)

    void ConvertToWideChar(const String& str, wchar_t** outStr, size_t* outLength);

#endif

private:
    int64 pid = -1;
    String output;
    FilePath executablePath;
    Vector<String> runArgs;
    bool running = false;

#if defined(__DAVAENGINE_WIN32__)
    HANDLE childProcIn[2];
    HANDLE childProcOut[2];
#else
    int pipes[2];
#endif

    int exitCode = -1; //invalid by default
};
};

#endif

#endif
