#include "Clipboard/Clipboard.h"

#if defined(__DAVAENGINE_WIN32__)
#include "Win32/ClipboardImpl.Win32.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "Mac/ClipboardImpl.Macos.h"
#else
#include "ClipboardImplStub.h"
#endif

namespace DAVA
{
Clipboard::Clipboard()
    : pImpl(std::make_unique<ClipboardImpl>())
{
}

Clipboard::~Clipboard()
{
}

bool Clipboard::IsReadyToUse() const
{
    return pImpl->IsReadyToUse();
}

bool Clipboard::Clear() const
{
    return pImpl->Clear();
}

bool Clipboard::HasText() const
{
    return pImpl->HasText();
}

bool Clipboard::SetText(const WideString& str)
{
    return pImpl->SetText(str);
}

WideString Clipboard::GetText() const
{
    return pImpl->GetText();
}
}
