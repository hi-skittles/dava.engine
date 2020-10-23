#include "ClipboardImpl.Win32.h"
#include "Base/Platform.h"
#include <WinUser.h>

namespace DAVA
{
ClipboardImplWin32::ClipboardImplWin32()
{
    isReady = ::OpenClipboard(nullptr) != 0;
}

ClipboardImplWin32::~ClipboardImplWin32()
{
    if (isReady)
    {
        ::CloseClipboard();
    }
}

bool ClipboardImplWin32::IsReadyToUse() const
{
    return isReady;
}

bool ClipboardImplWin32::Clear() const
{
    if (isReady)
    {
        return ::EmptyClipboard() != 0;
    }
    return false;
}

bool ClipboardImplWin32::HasText() const
{
    return ::IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}

bool ClipboardImplWin32::SetText(const WideString& str)
{
    if (isReady)
    {
        auto length = str.length();
        auto hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, (length + 1) * sizeof(WideString::value_type));
        if (hglbCopy != nullptr)
        {
            auto lptstrCopy = static_cast<LPWSTR>(::GlobalLock(hglbCopy));
            if (lptstrCopy != nullptr)
            {
                Memcpy(lptstrCopy, str.c_str(), length * sizeof(WideString::value_type));
                lptstrCopy[length] = static_cast<WideString::value_type>(0); // null character
                ::GlobalUnlock(hglbCopy);

                Clear();
                if (::SetClipboardData(CF_UNICODETEXT, hglbCopy) != nullptr)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

WideString ClipboardImplWin32::GetText() const
{
    WideString outPut;
    if (isReady && HasText())
    {
        auto hglb = ::GetClipboardData(CF_UNICODETEXT);
        if (hglb != nullptr)
        {
            auto lptstr = static_cast<LPTSTR>(::GlobalLock(hglb));
            if (lptstr != nullptr)
            {
                outPut = WideString(lptstr);
                ::GlobalUnlock(hglb);
            }
        }
    }
    return outPut;
}
}
