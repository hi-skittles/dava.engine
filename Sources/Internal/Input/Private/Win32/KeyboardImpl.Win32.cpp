#include "Input/Private/Win32/KeyboardImpl.Win32.h"

#include "Base/TemplateHelpers.h"
#include "Input/Private/KeyboardImplWinCodes.h"
#include "Utils/UTF8Utils.h"

#include <cwctype>
#include <Windows.h>

namespace DAVA
{
namespace Private
{
eInputElements KeyboardImpl::ConvertNativeScancodeToDavaScancode(uint32 nativeScancode, uint32 /*nativeVirtual*/)
{
    // Windows uses 0xE000 mask throughout its API to distinguish between extended and non-extended keys
    // We follow this convention and use the same mask

    const bool isExtended = (nativeScancode & 0xE000) == 0xE000;
    const uint32 nonExtendedScancode = nativeScancode & 0x00FF;

    if (isExtended)
    {
        if (nonExtendedScancode >= COUNT_OF(nativeScancodeExtToDavaScancode))
        {
            return eInputElements::NONE;
        }

        return nativeScancodeExtToDavaScancode[nonExtendedScancode];
    }
    else
    {
        if (nonExtendedScancode >= COUNT_OF(nativeScancodeToDavaScancode))
        {
            return eInputElements::NONE;
        }

        return nativeScancodeToDavaScancode[nonExtendedScancode];
    }
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

    if (nativeScancode == -1)
    {
        for (size_t i = 0; i < COUNT_OF(nativeScancodeExtToDavaScancode); ++i)
        {
            if (nativeScancodeExtToDavaScancode[i] == elementId)
            {
                nativeScancode = static_cast<int>(i) | 0xE000;
            }
        }
    }

    DVASSERT(nativeScancode >= 0);

    return static_cast<uint32>(nativeScancode);
}

String KeyboardImpl::TranslateElementToUTF8String(eInputElements elementId)
{
    uint32 nativeScancode = ConvertDavaScancodeToNativeScancode(elementId);
    wchar_t character = TranslateNativeScancodeToWChar(nativeScancode);

    if (character == 0 || !std::iswprint(character) || std::iswspace(character))
    {
        // Non printable
        return GetInputElementInfo(elementId).name;
    }
    else
    {
        return UTF8Utils::EncodeToUTF8(WideString(1, character));
    }
}

wchar_t KeyboardImpl::TranslateNativeScancodeToWChar(uint32 nativeScancode)
{
    const uint32 nativeVirtual = MapVirtualKey(nativeScancode, MAPVK_VSC_TO_VK);
    const wchar_t character = static_cast<wchar_t>(MapVirtualKey(nativeVirtual, MAPVK_VK_TO_CHAR));

    return character;
}

} // namespace Private
} // namespace DAVA
