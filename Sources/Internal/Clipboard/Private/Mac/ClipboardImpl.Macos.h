#pragma once

#include "Base/BaseTypes.h"
#include "Clipboard/Private/IClipboard.h"

namespace DAVA
{
class ClipboardImplMac : public IClipboard
{
public:
    ClipboardImplMac();
    ~ClipboardImplMac() override;
    bool IsReadyToUse() const override;
    bool Clear() const override;
    bool HasText() const override;
    bool SetText(const WideString& str) override;
    WideString GetText() const override;
};

using ClipboardImpl = ClipboardImplMac;
}
