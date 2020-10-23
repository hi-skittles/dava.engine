#include "Engine/EngineTypes.h"

namespace DAVA
{
eInputDevices TranslateUIntToInputDevice(uint32 value)
{
    static const struct
    {
        uint32 intValue;
        eInputDevices enumValue;
    } mapping[] = {
        // corev1 values
        { 0, eInputDevices::UNKNOWN },
        { 1, eInputDevices::TOUCH_SURFACE },
        { 2, eInputDevices::MOUSE },
        { 3, eInputDevices::KEYBOARD },
        { 4, eInputDevices::GAMEPAD },
        { 5, eInputDevices::PEN },
        { 6, eInputDevices::TOUCH_PAD },

        // corev2 values
        { static_cast<uint32>(eInputDevices::TOUCH_SURFACE), eInputDevices::TOUCH_SURFACE },
        { static_cast<uint32>(eInputDevices::MOUSE), eInputDevices::MOUSE },
        { static_cast<uint32>(eInputDevices::KEYBOARD), eInputDevices::KEYBOARD },
        { static_cast<uint32>(eInputDevices::GAMEPAD), eInputDevices::GAMEPAD },
        { static_cast<uint32>(eInputDevices::PEN), eInputDevices::PEN },
        { static_cast<uint32>(eInputDevices::TOUCH_PAD), eInputDevices::TOUCH_PAD },
    };

    eInputDevices result = eInputDevices::UNKNOWN;
    for (const auto& m : mapping)
    {
        if (m.intValue == value)
        {
            result = m.enumValue;
            break;
        }
    }
    return result;
}

} // namespace DAVA
