#include "Input/Private/Ios/KeyboardImpl.Ios.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
namespace Private
{
// TODO: Implement keyboard on iOS

eInputElements KeyboardImpl::ConvertNativeScancodeToDavaScancode(uint32 nativeScancode, uint32 nativeVirtual)
{
    return eInputElements::NONE;
}

uint32 KeyboardImpl::ConvertDavaScancodeToNativeScancode(eInputElements nativeScancode)
{
    return 0;
}

String KeyboardImpl::TranslateElementToUTF8String(eInputElements elementId)
{
    return GetInputElementInfo(elementId).name;
}

} // namespace Private
} // namespace DAVA
