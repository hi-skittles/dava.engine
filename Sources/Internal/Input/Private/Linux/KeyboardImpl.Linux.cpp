#include "Input/Private/Linux/KeyboardImpl.Linux.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
namespace Private
{
eInputElements KeyboardImpl::ConvertNativeScancodeToDavaScancode(uint32 nativeScancode, uint32 /*nativeVirtual*/)
{
    DVASSERT(0, "Linux: KeyboardImpl::ConvertNativeScancodeToDavaScancode not implemented");
    return eInputElements::NONE;
}

uint32 KeyboardImpl::ConvertDavaScancodeToNativeScancode(eInputElements elementId)
{
    DVASSERT(0, "Linux: KeyboardImpl::ConvertDavaScancodeToNativeScancode not implemented");
    return 0;
}

String KeyboardImpl::TranslateElementToUTF8String(eInputElements elementId)
{
    DVASSERT(0, "Linux: KeyboardImpl::TranslateElementToUTF8String not implemented");
    return String();
}

} // namespace Private
} // namespace DAVA
