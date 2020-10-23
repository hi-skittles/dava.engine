#pragma once

#include "Base/BaseTypes.h"
#include "Clipboard/Private/IClipboard.h"

namespace DAVA
{
class ClipboardImplStub : public IClipboard
{
public:
    ClipboardImplStub() = default;
    ~ClipboardImplStub() override = default;
    bool IsReadyToUse() const override;
    bool Clear() const override;
    bool HasText() const override;
    bool SetText(const WideString& str) override;
    WideString GetText() const override;
};

using ClipboardImpl = ClipboardImplStub;

inline bool ClipboardImplStub::IsReadyToUse() const
{
    return false;
}

inline bool ClipboardImplStub::Clear() const
{
    return false;
}

inline bool ClipboardImplStub::HasText() const
{
    return false;
}

inline bool ClipboardImplStub::SetText(const WideString& str)
{
    return false;
}

inline WideString ClipboardImplStub::GetText() const
{
    return WideString();
}
}
