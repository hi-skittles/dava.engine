#include "UTF8Utils.h"

#if defined(__DAVAENGINE_WINDOWS__)
#include <Objbase.h>

#if defined(__DAVAENGINE_WIN32__)
#include <ShellAPI.h>
#elif defined(__DAVAENGINE_WIN_UAP__)
#include <ppltasks.h>
#include "Utils/Utils.h"
#endif

namespace DAVA
{
String GenerateGUID()
{
    // Create new GUID
    //
    GUID guid;
    if (FAILED(CoCreateGuid(&guid)))
        return "";

    // Get string representation of GUID
    //
    Array<OLECHAR, 64> guidStringRaw{};
    ::StringFromGUID2(guid, guidStringRaw.data(), static_cast<int>(guidStringRaw.size()));


#ifndef OLE2ANSI
    // Convert to normal string
    // OLECHAR's type is wchar if OLE2ANSI is not defined, otherwise its type is char
    //
    WideString guidWideStr(guidStringRaw.data());
    return UTF8Utils::EncodeToUTF8(guidWideStr);
#else
    return String(guidStringRaw.data());
#endif
}

#if defined(__DAVAENGINE_WIN_UAP__)

void OpenURL(const String& url)
{
    Windows::Foundation::Uri ^ uri = nullptr;

    try
    {
        auto platformString = ref new Platform::String(UTF8Utils::EncodeToWideString(url).c_str());
        uri = ref new Windows::Foundation::Uri(platformString);
    }
    catch (Platform::InvalidArgumentException ^ )
    {
        // nothing to do if uri is invalid
        return;
    }

    WaitAsync(Windows::System::Launcher::LaunchUriAsync(uri));
}

#elif defined(__DAVAENGINE_WIN32__)

void OpenURL(const String& url)
{
    WideString urlWide = UTF8Utils::EncodeToWideString(url);
    ShellExecute(nullptr, L"open", urlWide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

#endif //  __DAVAENGINE_WIN32__

} //  namespace DAVA

#endif //  __DAVAENGINE_WINDOWS__