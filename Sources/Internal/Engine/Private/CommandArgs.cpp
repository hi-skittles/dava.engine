#include "Base/BaseTypes.h"

#include "Base/Platform.h"
#include "Utils/Utils.h"
#include "Utils/UTF8Utils.h"

#if defined(__DAVAENGINE_WINDOWS__)
#include <shellapi.h>
#endif

#include <sstream>

namespace DAVA
{
namespace Private
{
Vector<String> GetCommandArgs(int argc, char* argv[])
{
#if defined(__DAVAENGINE_MACOS__)
    struct SkippedParams
    {
        String param;
        bool hasValues = true;
    };

    Vector<SkippedParams> skippedParams =
    {
      { "-NSDocumentRevisionsDebugMode", true }
    };
#endif //#if defined(__DAVAENGINE_MACOS__)

    Vector<String> cmdargs;
    cmdargs.reserve(argc);
    for (int i = 0; i < argc; ++i)
    {
#if defined(__DAVAENGINE_MACOS__)

        Vector<SkippedParams>::iterator it = std::find_if(skippedParams.begin(), skippedParams.end(), [&i, &argv](const SkippedParams& sp)
                                                          {
                                                              return sp.param == argv[i];
                                                          });
        if (it != skippedParams.end())
        {
            if (it->hasValues)
            {
                ++i;
            }
            continue;
        }
#endif //#if defined(__DAVAENGINE_MACOS__)

        cmdargs.push_back(argv[i]);
    }
    return cmdargs;
}

Vector<String> GetCommandArgs(const String& cmdline)
{
    //TODO: correctly break command line into args
    Vector<String> cmdargs;
    if (!cmdline.empty())
    {
        std::istringstream stream(cmdline);

        String token;
        while (std::getline(stream, token, ' '))
            cmdargs.push_back(token);
    }
    return cmdargs;
}

#if defined(__DAVAENGINE_WIN32__)

Vector<String> GetCommandArgs()
{
    Vector<String> cmdargs;

    int nargs = 0;
    LPWSTR cmdline = ::GetCommandLineW();
    LPWSTR* arglist = ::CommandLineToArgvW(cmdline, &nargs);
    if (arglist != nullptr)
    {
        cmdargs.reserve(nargs);
        for (int i = 0; i < nargs; ++i)
        {
            cmdargs.push_back(UTF8Utils::EncodeToUTF8(arglist[i]));
        }
        ::LocalFree(arglist);
    }
    return cmdargs;
}

#elif defined(__DAVAENGINE_WIN_UAP__)

Vector<String> GetCommandArgs()
{
    LPWSTR cmdline = ::GetCommandLineW();
    return GetCommandArgs(UTF8Utils::EncodeToUTF8(cmdline));
}

#endif

} // namespace Private
} // namespace DAVA
