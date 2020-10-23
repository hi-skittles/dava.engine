#include "Debug/Private/ImGuiUtils.h"

#include "Debug/Private/ImGui.h"
#include "Engine/Engine.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/UIControlSystem.h"

#include <imgui/imgui_internal.h>

namespace DAVA
{
void ImGuiUtils::SetScale(float32 scale)
{
    ImGui::Settings::scale = ImGui::Settings::pendingScale = scale;
}

float32 ImGuiUtils::GetScale()
{
    return ImGui::Settings::scale;
}

void ImGuiUtils::SetScaleAsync(float32 scale)
{
    ImGui::Settings::pendingScale = scale;
}

float32 ImGuiUtils::GetImGuiScreenToPhysicalScreenSizeScale()
{
    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;

    Size2i screen = vcs->GetPhysicalScreenSize();
    float32 scaleX = static_cast<float32>(screen.dx) / ImGui::screenWidth;
    float32 scaleY = static_cast<float32>(screen.dy) / ImGui::screenHeight;

    return Min(scaleX, scaleY);
}

Vector2 ImGuiUtils::ConvertInputCoordsToImGuiCoords(const Vector2& inputCoords)
{
    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;

    Vector2 coords = vcs->ConvertInputToVirtual(inputCoords);
    Vector2 physCoords = vcs->ConvertVirtualToPhysical(coords);

    return physCoords / GetImGuiScreenToPhysicalScreenSizeScale();
}

Vector2 ImGuiUtils::ConvertPhysicalCoordsToImGuiCoords(const Vector2& physicalCoords)
{
    return physicalCoords / GetImGuiScreenToPhysicalScreenSizeScale();
}

Vector2 ImGuiUtils::ConvertVirtualCoordsToImGuiCoords(const Vector2& virtualCoords)
{
    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;

    Vector2 coords = vcs->ConvertVirtualToPhysical(virtualCoords);
    return ConvertPhysicalCoordsToImGuiCoords(coords);
}

Vector2 ImGuiUtils::ConvertImGuiCoordsToPhysicalCoords(const Vector2& virtualCoords)
{
    return virtualCoords * GetImGuiScreenToPhysicalScreenSizeScale();
}

Vector2 ImGuiUtils::ConvertImGuiCoordsToVirtualCoords(const Vector2& virtualCoords)
{
    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;

    Vector2 coords = ConvertImGuiCoordsToPhysicalCoords(virtualCoords);
    return vcs->ConvertPhysicalToVirtual(coords);
}
} // namespace DAVA