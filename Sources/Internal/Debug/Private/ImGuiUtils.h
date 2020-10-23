#pragma once

#include "Base/BaseTypes.h"
#include "Math/Vector.h"

namespace DAVA
{
class ImGuiUtils
{
public:
    /** Set ImGui interface scale. Useful for mobile devices with small screen. */
    static void SetScale(float32 scale);

    /** Get current ImGui interface scale */
    static float32 GetScale();

    /** Set ImGui interface scale from next frame. */
    static void SetScaleAsync(float32 scale);

    /** Get ImGui screen size to physical screen size scale. */
    static float32 GetImGuiScreenToPhysicalScreenSizeScale();

    /** Convert framework input coords to ImGui coords. */
    static Vector2 ConvertInputCoordsToImGuiCoords(const Vector2& inputCoords);

    /** Convert physical coords to ImGui coords. */
    static Vector2 ConvertPhysicalCoordsToImGuiCoords(const Vector2& physicalCoords);

    /** Convert framework virtual coords to ImGui coords. */
    static Vector2 ConvertVirtualCoordsToImGuiCoords(const Vector2& virtualCoords);

    /** Convert ImGui coords to physical coords. */
    static Vector2 ConvertImGuiCoordsToPhysicalCoords(const Vector2& virtualCoords);

    /** Convert ImGui coords to framework virtual coords. */
    static Vector2 ConvertImGuiCoordsToVirtualCoords(const Vector2& virtualCoords);
};
} // namespace DAVA