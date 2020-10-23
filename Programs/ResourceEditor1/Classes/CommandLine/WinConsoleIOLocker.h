#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_WIN32__)

namespace WinConsoleIO
{
struct IOHandle;
} //END of WinConsoleIO

class WinConsoleIOLocker final
{
public:
    WinConsoleIOLocker();
    ~WinConsoleIOLocker();

private:
    std::unique_ptr<WinConsoleIO::IOHandle> ioHandle;
};

#endif //#if defined(__DAVAENGINE_WIN32__)
