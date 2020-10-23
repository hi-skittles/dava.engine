#include "Input/Private/Mac/KeyboardImpl.Macos.h"

#include "Base/TemplateHelpers.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Utils/UTF8Utils.h"
#include "Utils/NSStringUtils.h"

#import <Carbon/Carbon.h>

namespace DAVA
{
namespace Private
{
// Native scancode (kVK_* defined in Carbon/HIToolbox/Events.h) to dava scancode
const eInputElements nativeScancodeToDavaScancode[] =
{
  eInputElements::KB_A, // 0x00
  eInputElements::KB_S, // 0x01
  eInputElements::KB_D, // 0x02
  eInputElements::KB_F, // 0x03
  eInputElements::KB_H, // 0x04
  eInputElements::KB_G, // 0x05
  eInputElements::KB_Z, // 0x06
  eInputElements::KB_X, // 0x07
  eInputElements::KB_C, // 0x08
  eInputElements::KB_V, // 0x09
  eInputElements::KB_NONUSBACKSLASH, // 0x0A
  eInputElements::KB_B, // 0x0B
  eInputElements::KB_Q, // 0x0C
  eInputElements::KB_W, // 0x0D
  eInputElements::KB_E, // 0x0E
  eInputElements::KB_R, // 0x0F
  eInputElements::KB_Y, // 0x10
  eInputElements::KB_T, // 0x11
  eInputElements::KB_1, // 0x12
  eInputElements::KB_2, // 0x13
  eInputElements::KB_3, // 0x14
  eInputElements::KB_4, // 0x15
  eInputElements::KB_6, // 0x16
  eInputElements::KB_5, // 0x17
  eInputElements::KB_EQUALS, // 0x18
  eInputElements::KB_9, // 0x19
  eInputElements::KB_7, // 0x1A
  eInputElements::KB_MINUS, // 0x1B
  eInputElements::KB_8, // 0x1C
  eInputElements::KB_0, // 0x1D
  eInputElements::KB_RBRACKET, // 0x1E
  eInputElements::KB_O, // 0x1F
  eInputElements::KB_U, // 0x20
  eInputElements::KB_LBRACKET, // 0x21
  eInputElements::KB_I, // 0x22
  eInputElements::KB_P, // 0x23
  eInputElements::KB_ENTER, // 0x24
  eInputElements::KB_L, // 0x25
  eInputElements::KB_J, // 0x26
  eInputElements::KB_APOSTROPHE, // 0x27
  eInputElements::KB_K, // 0x28
  eInputElements::KB_SEMICOLON, // 0x29
  eInputElements::KB_BACKSLASH, // 0x2A
  eInputElements::KB_COMMA, // 0x2B
  eInputElements::KB_SLASH, // 0x2C
  eInputElements::KB_N, // 0x2D
  eInputElements::KB_M, // 0x2E
  eInputElements::KB_PERIOD, // 0x2F
  eInputElements::KB_TAB, // 0x30
  eInputElements::KB_SPACE, // 0x31
  eInputElements::KB_GRAVE, // 0x32
  eInputElements::KB_BACKSPACE, // 0x33
  eInputElements::NONE, // 0x34
  eInputElements::KB_ESCAPE, // 0x35
  eInputElements::KB_RCMD, // 0x36
  eInputElements::KB_LCMD, // 0x37
  eInputElements::KB_LSHIFT, // 0x38
  eInputElements::KB_CAPSLOCK, // 0x39
  eInputElements::KB_LALT, // 0x3A
  eInputElements::KB_LCTRL, // 0x3B
  eInputElements::KB_RSHIFT, // 0x3C
  eInputElements::KB_RALT, // 0x3D
  eInputElements::KB_RCTRL, // 0x3E
  eInputElements::NONE, // 0x3F
  eInputElements::NONE, // 0x40
  eInputElements::KB_NUMPAD_DELETE, // 0x41
  eInputElements::NONE, // 0x42
  eInputElements::KB_MULTIPLY, // 0x43
  eInputElements::NONE, // 0x44
  eInputElements::KB_NUMPAD_PLUS, // 0x45
  eInputElements::NONE, // 0x46
  eInputElements::KB_NUMLOCK, // 0x47
  eInputElements::NONE, // 0x48
  eInputElements::NONE, // 0x49
  eInputElements::NONE, // 0x4A
  eInputElements::KB_DIVIDE, // 0x4B
  eInputElements::KB_NUMPAD_ENTER, // 0x4C
  eInputElements::NONE, // 0x4D
  eInputElements::KB_NUMPAD_MINUS, // 0x4E
  eInputElements::NONE, // 0x4F
  eInputElements::NONE, // 0x50
  eInputElements::NONE, // 0x51
  eInputElements::KB_NUMPAD_0, // 0x52
  eInputElements::KB_NUMPAD_1, // 0x53
  eInputElements::KB_NUMPAD_2, // 0x54
  eInputElements::KB_NUMPAD_3, // 0x55
  eInputElements::KB_NUMPAD_4, // 0x56
  eInputElements::KB_NUMPAD_5, // 0x57
  eInputElements::KB_NUMPAD_6, // 0x58
  eInputElements::KB_NUMPAD_7, // 0x59
  eInputElements::NONE, // 0x5A
  eInputElements::KB_NUMPAD_8, // 0x5B
  eInputElements::KB_NUMPAD_9, // 0x5C
  eInputElements::NONE, // 0x5D
  eInputElements::NONE, // 0x5E
  eInputElements::NONE, // 0x5F
  eInputElements::KB_F5, // 0x60
  eInputElements::KB_F6, // 0x61
  eInputElements::KB_F7, // 0x62
  eInputElements::KB_F3, // 0x63
  eInputElements::KB_F8, // 0x64
  eInputElements::KB_F9, // 0x65
  eInputElements::NONE, // 0x66
  eInputElements::KB_F11, // 0x67
  eInputElements::NONE, // 0x68
  eInputElements::KB_PRINTSCREEN, // 0x69
  eInputElements::NONE, // 0x6A
  eInputElements::NONE, // 0x6B
  eInputElements::NONE, // 0x6C
  eInputElements::KB_F10, // 0x6D
  eInputElements::KB_MENU, // 0x6E
  eInputElements::KB_F12, // 0x6F
  eInputElements::NONE, // 0x70
  eInputElements::NONE, // 0x71
  eInputElements::KB_INSERT, // 0x72
  eInputElements::KB_HOME, // 0x73
  eInputElements::KB_PAGEUP, // 0x74
  eInputElements::KB_DELETE, // 0x75
  eInputElements::KB_F4, // 0x76
  eInputElements::KB_END, // 0x77
  eInputElements::KB_F2, // 0x78
  eInputElements::KB_PAGEDOWN, // 0x79
  eInputElements::KB_F1, // 0x7A
  eInputElements::KB_LEFT, // 0x7B
  eInputElements::KB_RIGHT, // 0x7C
  eInputElements::KB_DOWN, // 0x7D
  eInputElements::KB_UP, // 0x7E
};

KeyboardImpl::KeyboardImpl()
{
    nonPrintableCharacters = [[NSMutableCharacterSet alloc] init];
    [nonPrintableCharacters formUnionWithCharacterSet:[NSMutableCharacterSet alphanumericCharacterSet]];
    [nonPrintableCharacters formUnionWithCharacterSet:[NSMutableCharacterSet punctuationCharacterSet]];
    [nonPrintableCharacters invert];
}

KeyboardImpl::~KeyboardImpl()
{
    [nonPrintableCharacters release];
}

eInputElements KeyboardImpl::ConvertNativeScancodeToDavaScancode(uint32 nativeScancode, uint32 /*nativeVirtual*/)
{
    if (nativeScancode >= COUNT_OF(nativeScancodeToDavaScancode))
    {
        return eInputElements::NONE;
    }

    return nativeScancodeToDavaScancode[nativeScancode];
}

uint32 KeyboardImpl::ConvertDavaScancodeToNativeScancode(eInputElements elementId)
{
    int nativeScancode = -1;

    for (size_t i = 0; i < COUNT_OF(nativeScancodeToDavaScancode); ++i)
    {
        if (nativeScancodeToDavaScancode[i] == elementId)
        {
            nativeScancode = static_cast<int>(i);
        }
    }

    DVASSERT(nativeScancode >= 0);

    return static_cast<uint32>(nativeScancode);
}

String KeyboardImpl::TranslateElementToUTF8String(eInputElements elementId)
{
    for (size_t i = 0; i < COUNT_OF(nativeScancodeToDavaScancode); ++i)
    {
        if (nativeScancodeToDavaScancode[i] == elementId)
        {
            String result;

            // Get ascii capable input source
            TISInputSourceRef inputSource = TISCopyCurrentASCIICapableKeyboardInputSource();
            CFDataRef layoutData = (CFDataRef)TISGetInputSourceProperty(inputSource, kTISPropertyUnicodeKeyLayoutData);

            if (layoutData != nullptr)
            {
                const UCKeyboardLayout* keyboardLayout = (const UCKeyboardLayout*)CFDataGetBytePtr(layoutData);

                // Translate key to unicode using selected input source
                const size_t maxLength = 4;
                UniChar unicodeString[maxLength];
                UniCharCount realLength;
                uint32 deadKeyState = 0;

                OSStatus status = UCKeyTranslate(keyboardLayout,
                                                 i,
                                                 kUCKeyActionDown,
                                                 0,
                                                 LMGetKbdType(),
                                                 kUCKeyTranslateNoDeadKeysMask,
                                                 &deadKeyState,
                                                 maxLength,
                                                 &realLength,
                                                 unicodeString);

                DVASSERT(status == 0);

                // UCKeyTranslate can produce characters we don't want to show to a user.
                // For example, backspace, delete and enter keys will be translated to control characters,
                // but we want to use GetInputElementInfo(elementId).name for them instead to show meaningful string.
                // So we trim all the characters except for letters, numbers and punctuation symbols.
                // If resulting string is empty, the character is non-printable

                DVASSERT(nonPrintableCharacters != nil);

                NSString* string = [NSString stringWithCharacters:unicodeString length:realLength];
                NSString* trimmedString = [string stringByTrimmingCharactersInSet:nonPrintableCharacters];

                if ([trimmedString length] == 0)
                {
                    // Non printable
                    result = GetInputElementInfo(elementId).name;
                }
                else
                {
                    result = StringFromNSString([trimmedString uppercaseString]);
                }
            }
            else
            {
                // kTISPropertyUnicodeKeyLayoutData gives null for Japanese and some other languages,
                // use default name in this case
                result = GetInputElementInfo(elementId).name;
            }

            CFRelease(inputSource);

            return result;
        }
    }

    DVASSERT(false);
    return String("");
}

} // namespace Private
} // namespace DAVA
