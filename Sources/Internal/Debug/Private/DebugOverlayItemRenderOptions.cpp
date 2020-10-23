#include "Debug/Private/DebugOverlayItemRenderOptions.h"
#include "Debug/DebugOverlay.h"
#include "Debug/Private/ImGui.h"
#include "Engine/Engine.h"
#include "Render/Renderer.h"

namespace DAVA
{
String DebugOverlayItemRenderOptions::GetName() const
{
    return "Render options";
}

void DebugOverlayItemRenderOptions::Draw()
{
    RenderOptions* options = Renderer::GetOptions();

    if (options == nullptr)
    {
        return;
    }

    bool shown = true;
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 600.0f), ImVec2(FLOAT_MAX, FLOAT_MAX));

    if (ImGui::Begin("RenderOptionsWindow", &shown, ImGuiWindowFlags_NoFocusOnAppearing))
    {
        for (uint32 i = 0; i < uint32(RenderOptions::OPTIONS_COUNT); ++i)
        {
            RenderOptions::RenderOption option = RenderOptions::RenderOption(i);
            bool value = options->IsOptionEnabled(option);

            ImGui::Checkbox(options->GetOptionName(option).c_str(), &value);
            if (value != options->IsOptionEnabled(option))
                options->SetOption(option, value);
        }
    }

    ImGui::End();

    if (!shown)
    {
        GetEngineContext()->debugOverlay->HideItem(this);
    }
}
}