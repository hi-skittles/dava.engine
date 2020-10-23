#pragma once

#if defined(__DAVAENGINE_WINDOWS__)

// Mappings from native to dava scancodes are the same for both Win32 and UWP, so put them in common header

#include "Input/InputElements.h"

namespace DAVA
{
namespace Private
{
// Maps native windows scancode to KB_* scancode element
extern const eInputElements nativeScancodeToDavaScancode[0x59];

// Maps native windows extended scancode to KB_* scancode element
extern const eInputElements nativeScancodeExtToDavaScancode[0x5E];
}
}
#endif
