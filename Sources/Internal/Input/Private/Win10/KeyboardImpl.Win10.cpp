#include "Input/Private/Win10/KeyboardImpl.Win10.h"
#include "Input/Private/KeyboardImplWinCodes.h"
#include "Base/TemplateHelpers.h"
#include "Utils/UTF8Utils.h"
#include "Engine/Private/Win10/DllImportWin10.h"

#include <cwctype>

namespace DAVA
{
namespace Private
{
eInputElements KeyboardImpl::ConvertNativeScancodeToDavaScancode(uint32 nativeScancode, uint32 nativeVirtual)
{
    using ::Windows::System::VirtualKey;

    const bool isExtended = (nativeScancode & 0xE000) == 0xE000;
    const uint32 nonExtendedScancode = nativeScancode & 0x00FF;

    // Pause/Break and right control send the same scancode (29) with extended flag
    // So use virtual key to distinguish between them
    if (nonExtendedScancode == 29 && isExtended)
    {
        VirtualKey virtualKey = static_cast<VirtualKey>(nativeVirtual);

        if (virtualKey == VirtualKey::Pause)
        {
            return eInputElements::KB_PAUSE;
        }
        else
        {
            DVASSERT(virtualKey == VirtualKey::Control);

            // Send right control since this is the one which has 'extended' flag
            return eInputElements::KB_RCTRL;
        }
    }

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
    if (DllImport::fnMapVirtualKey == nullptr)
    {
        // Return default en name if MapVirtualKey couldn't be imported
        return GetInputElementInfo(elementId).name;
    }

    int nativeScancode = ConvertDavaScancodeToNativeScancode(elementId);
    wchar_t character = TranslateNativeScancodeToWChar(static_cast<uint32>(nativeScancode));

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
    const uint32 nativeVirtual = DllImport::fnMapVirtualKey(nativeScancode, MAPVK_VSC_TO_VK);
    const wchar_t character = static_cast<wchar_t>(DllImport::fnMapVirtualKey(nativeVirtual, MAPVK_VK_TO_CHAR));

    return character;
}

} // namespace Private
} // namespace DAVA
