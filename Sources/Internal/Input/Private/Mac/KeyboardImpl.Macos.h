#pragma once

#include "Input/InputElements.h"

DAVA_FORWARD_DECLARE_OBJC_CLASS(NSMutableCharacterSet);

namespace DAVA
{
namespace Private
{
class KeyboardImpl final
{
public:
    KeyboardImpl();
    ~KeyboardImpl();
    KeyboardImpl(const KeyboardImpl&) = delete;
    KeyboardImpl& operator=(const KeyboardImpl&) = delete;

    eInputElements ConvertNativeScancodeToDavaScancode(uint32 nativeScancode, uint32 nativeVirtual);
    uint32 ConvertDavaScancodeToNativeScancode(eInputElements nativeScancode);
    String TranslateElementToUTF8String(eInputElements elementId);

private:
    NSMutableCharacterSet* nonPrintableCharacters; // To use inside of TranslateElementToUTF8String
};

} // namespace Private
} // namespace DAVA
