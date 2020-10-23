#include "Debug/Private/DebugOverlayItemRenderStats.h"
#include "Debug/DebugOverlay.h"
#include "Debug/Private/ImGui.h"
#include "Engine/Engine.h"
#include "Render/Renderer.h"
#include "Render/VisibilityQueryResults.h"

namespace DAVA
{
String DebugOverlayItemRenderStats::GetName() const
{
    return "Render stats";
}

void DebugOverlayItemRenderStats::Draw()
{
    bool shown = true;
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 200.0f), ImVec2(FLOAT_MAX, FLOAT_MAX));

    if (ImGui::Begin("RenderStatsWindow", &shown, ImGuiWindowFlags_NoFocusOnAppearing))
    {
#if defined(__DAVAENGINE_RENDERSTATS__)
        RenderStats& stats = Renderer::GetRenderStats();

        if (ImGui::CollapsingHeader("Draw Calls"))
        {
            AddUIntStat("Draw Primitive", stats.drawPrimitive);
            AddUIntStat("Draw Indexed Primitive", stats.drawIndexedPrimitive);
            AddUIntStat("Triangle List Count", stats.primitiveTriangleListCount);
            AddUIntStat("Triangle Strip Count", stats.primitiveTriangleStripCount);
            AddUIntStat("Line List Count", stats.primitiveLineListCount);
        }

        if (ImGui::CollapsingHeader("State Switch"))
        {
            AddUIntStat("Pipeline State", stats.pipelineStateSet);
            AddUIntStat("Sampler State", stats.samplerStateSet);
            AddUIntStat("Const Buffer", stats.constBufferSet);
            AddUIntStat("Texture Set", stats.textureSet);
            AddUIntStat("Vertex Buffer", stats.vertexBufferSet);
            AddUIntStat("Index Buffer", stats.indexBufferSet);
        }

        if (ImGui::CollapsingHeader("Params Bindings"))
        {
            AddUIntStat("Dynamic Param Bind", stats.dynamicParamBindCount);
            AddUIntStat("Material Param Bind", stats.materialParamBindCount);
        }

        if (ImGui::CollapsingHeader("2D"))
        {
            AddUIntStat("Batches", stats.batches2d);
            AddUIntStat("Packets", stats.packets2d);
        }

        if (ImGui::CollapsingHeader("Fragments Info"))
        {
            for (uint32 i = 0; i < uint32(VisibilityQueryResults::QUERY_INDEX_COUNT); ++i)
            {
                const FastName& queryName = VisibilityQueryResults::GetQueryIndexName(VisibilityQueryResults::eQueryIndex(i));
                float32 value = float32(stats.visibilityQueryResults[queryName]) / Renderer::GetFramebufferWidth() / Renderer::GetFramebufferHeight();
                AddPercentageStat(queryName.c_str(), value);
            }
        }
#else
        ImGui::Text("__DAVAENGINE_RENDERSTATS__ is not defined");
#endif
    }

    ImGui::End();

    if (!shown)
    {
        GetEngineContext()->debugOverlay->HideItem(this);
    }
}

void DebugOverlayItemRenderStats::AddUIntStat(const char* name, uint32 value)
{
    String valuestr = Format("%d", value);
    ImGui::TextUnformatted(name);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(valuestr.c_str()).x);
    ImGui::TextUnformatted(valuestr.c_str());
}

void DebugOverlayItemRenderStats::AddPercentageStat(const char* name, float32 value)
{
    String valuestr = Format("%.2f%%", value * 100.f);
    ImGui::TextUnformatted(name);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(valuestr.c_str()).x);
    ImGui::TextUnformatted(valuestr.c_str());
}
}