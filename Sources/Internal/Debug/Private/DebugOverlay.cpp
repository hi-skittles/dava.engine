#include "Debug/DebugOverlay.h"

#include "Concurrency/Thread.h"
#include "Debug/Private/ImGui.h"
#include "Debug/Private/ImGuiUtils.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Engine/Window.h"

#include "Debug/DebugOverlayItem.h"
#include "Debug/Private/DebugOverlayItemEngineSettings.h"
#include "Debug/Private/DebugOverlayItemLogger.h"
#include "Debug/Private/DebugOverlayItemProfiler.h"
#include "Debug/Private/DebugOverlayItemRenderOptions.h"
#include "Debug/Private/DebugOverlayItemRenderStats.h"

#include <imgui/imgui_internal.h>

namespace DAVA
{
DebugOverlay::DebugOverlay()
    : defaultItemEngineSettings{ new DebugOverlayItemEngineSettings }
    , defaultItemLogger{ new DebugOverlayItemLogger }
    , defaultItemRenderOptions{ new DebugOverlayItemRenderOptions }
    , defaultItemRenderStats{ new DebugOverlayItemRenderStats }
    , defaultItemProfiler{ new DebugOverlayItemProfiler(ProfilerGPU::globalProfiler, ProfilerCPU::globalProfiler, ProfilerCPUMarkerName::ENGINE_ON_FRAME) }
{
    RegisterDefaultItems();
}

DebugOverlay::~DebugOverlay()
{
    UnregisterDefaultItems();

    DVASSERT(items.size() == 0);

    if (shown)
    {
        Engine::Instance()->update.Disconnect(this);
    }
}

void DebugOverlay::Show()
{
    DVASSERT(Thread::IsMainThread());

    if (!shown)
    {
        Window* primaryWindow = GetPrimaryWindow();
        DVASSERT(primaryWindow != nullptr);
        primaryWindow->update.Connect(this, &DebugOverlay::OnUpdate);

        shown = true;
    }
}

void DebugOverlay::Hide()
{
    DVASSERT(Thread::IsMainThread());

    if (shown)
    {
        Window* primaryWindow = GetPrimaryWindow();
        DVASSERT(primaryWindow != nullptr);
        primaryWindow->update.Disconnect(this);

        shown = false;
    }
}

bool DebugOverlay::IsShown() const
{
    DVASSERT(Thread::IsMainThread());

    return shown;
}

void DebugOverlay::RegisterItem(DebugOverlayItem* overlayItem)
{
    DVASSERT(Thread::IsMainThread());
    DVASSERT(overlayItem != nullptr);
    DVASSERT(std::find_if(items.begin(), items.end(), [overlayItem](ItemData const& itemData) { return itemData.item == overlayItem; }) == items.end());

    ItemData itemData;
    itemData.item = overlayItem;
    itemData.name = overlayItem->GetName();
    itemData.shown = false;

    items.push_back(std::move(itemData));
}

void DebugOverlay::UnregisterItem(DebugOverlayItem* overlayItem)
{
    DVASSERT(Thread::IsMainThread());
    DVASSERT(overlayItem != nullptr);

    auto iter = std::find_if(items.begin(), items.end(), [overlayItem](ItemData const& itemData) { return itemData.item == overlayItem; });
    DVASSERT(iter != items.end());

    if (iter->shown)
    {
        iter->item->OnHidden();
    }

    items.erase(iter);
}

void DebugOverlay::ShowItem(DebugOverlayItem* overlayItem)
{
    DVASSERT(Thread::IsMainThread());
    DVASSERT(overlayItem != nullptr);

    auto iter = std::find_if(items.begin(), items.end(), [overlayItem](ItemData const& itemData) { return itemData.item == overlayItem; });
    DVASSERT(iter != items.end());

    if (iter->shown == false)
    {
        iter->shown = true;
        iter->item->OnShown();
    }
}

void DebugOverlay::HideItem(DebugOverlayItem* overlayItem)
{
    DVASSERT(Thread::IsMainThread());
    DVASSERT(overlayItem != nullptr);

    auto iter = std::find_if(items.begin(), items.end(), [overlayItem](ItemData const& itemData) { return itemData.item == overlayItem; });
    DVASSERT(iter != items.end());

    if (iter->shown == true)
    {
        iter->shown = false;
        iter->item->OnHidden();
    }
}

void DebugOverlay::RegisterDefaultItems()
{
    RegisterItem(defaultItemEngineSettings.get());
    RegisterItem(defaultItemLogger.get());
    RegisterItem(defaultItemRenderOptions.get());
    RegisterItem(defaultItemRenderStats.get());
    RegisterItem(defaultItemProfiler.get());
}

void DebugOverlay::UnregisterDefaultItems()
{
    UnregisterItem(defaultItemEngineSettings.get());
    UnregisterItem(defaultItemLogger.get());
    UnregisterItem(defaultItemRenderOptions.get());
    UnregisterItem(defaultItemRenderStats.get());
    UnregisterItem(defaultItemProfiler.get());
}

void DebugOverlay::OnUpdate(Window* window, float32 timeDelta)
{
    DVASSERT(ImGui::IsInitialized());

    auto PushColor = [](float32 mainH) {
        ImGui::PushStyleColor(ImGuiCol_Button, static_cast<ImVec4>(ImColor::HSV(mainH, 1.f, 0.6f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, static_cast<ImVec4>(ImColor::HSV(mainH, 1.f, 0.7f)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, static_cast<ImVec4>(ImColor::HSV(mainH, 1.f, 0.8f)));
    };

    auto PopColor = []() {
        ImGui::PopStyleColor(3);
    };

    if (ImGui::IsInitialized())
    {
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
        uint32 windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin("DebugOverlayWindow", nullptr, windowFlags))
        {
            if (ImGui::Button("Debug views"))
            {
                ImGui::OpenPopup("DebugOverlayViewsPopup");
            }

            ImGui::SetNextWindowPos(ImVec2(20.0f, ImGui::GetWindowSize().y + 20.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            if (ImGui::BeginPopup("DebugOverlayViewsPopup"))
            {
                for (ItemData& itemData : items)
                {
                    const bool wasShown = itemData.shown;
                    ImGui::Checkbox(itemData.name.c_str(), &itemData.shown);
                    if (itemData.shown != wasShown)
                    {
                        itemData.shown ? itemData.item->OnShown() : itemData.item->OnHidden();
                    }
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar(1);
        }

        ImGui::End();
        ImGui::PopStyleVar(2);

        float32 scale = ImGuiUtils::GetScale();

        float32 buttonSide = Max(40.f, 40.f * (window->GetDPI() / 300.f)) / scale;
        Vector2 hLine = { buttonSide * 0.5f, buttonSide * 0.1f };
        Vector2 vLine = { buttonSide * 0.1f, buttonSide * 0.5f };

        // For '+' and '-'
        auto DrawLine = [buttonSide](Vector2 pos, const Vector2& size) {
            pos.x += buttonSide / 2.f - size.x / 2.f;
            pos.y += buttonSide / 2.f;
            ImGui::GetWindowDrawList()->AddLine({ pos.x, pos.y }, { pos.x + size.x, pos.y }, 0xFFFFFFFF, size.y);
        };

        auto DrawEqualSign = [buttonSide](Vector2 pos, const Vector2& size) {
            pos.x += buttonSide / 2.f - size.x / 2.f;
            pos.y += buttonSide / 2.f - size.y;
            ImGui::GetWindowDrawList()->AddLine({ pos.x, pos.y }, { pos.x + size.x, pos.y }, 0xFFFFFFFF, size.y);
            pos.y += 2.f * size.y;
            ImGui::GetWindowDrawList()->AddLine({ pos.x, pos.y }, { pos.x + size.x, pos.y }, 0xFFFFFFFF, size.y);
        };

        auto DrawRect = [buttonSide](Vector2 pos, const Vector2& size) {
            pos.x += buttonSide * 0.1f;
            pos.y += buttonSide * 0.1f;
            ImGui::GetWindowDrawList()->AddRectFilled({ pos.x, pos.y }, { pos.x + size.x, pos.y + size.y }, 0xFFFFFFFF);
        };

        float32 x = static_cast<float32>(ImGui::screenWidth - 20) / scale - buttonSide;
        float32 y = -buttonSide;

        const float32 maxScale = 2.5f;
        const float32 minScale = 0.5f;
        const float32 scaleStep = 0.075f;

        auto NextButton = [&y, scale, buttonSide]() {
            y += buttonSide + 20.f / scale;
            ImGui::SetCursorPosY(y);
        };

        ImGui::SetNextWindowPos(ImVec2(x, 0.f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        if (ImGui::Begin("DebugOverlayButtons", nullptr, windowFlags))
        {
            NextButton();

            PushColor(0.3f);
            // We can't leave button id empty, but characters after '##' will be ignored and will not be shown on the button.
            if (ImGui::Button("##incScale", { buttonSide, buttonSide }) && scale < maxScale)
            {
                ImGuiUtils::SetScaleAsync(scale + scaleStep);
            }
            PopColor();

            // Draw '+'
            DrawLine({ x, y }, hLine);
            DrawLine({ x, y }, vLine);

            NextButton();

            PushColor(0.6f);
            if (ImGui::Button("##resetScale", { buttonSide, buttonSide }))
            {
                ImGuiUtils::SetScaleAsync(1.f);
            }
            PopColor();

            DrawEqualSign({ x, y }, hLine);

            NextButton();

            PushColor(1.f);
            if (ImGui::Button("##decScale", { buttonSide, buttonSide }) && scale > minScale)
            {
                ImGuiUtils::SetScaleAsync(scale - scaleStep);
            }
            PopColor();

            // Draw '-'
            DrawLine({ x, y }, hLine);

            NextButton();
            NextButton();

            PushColor(0.1f);
            if (ImGui::Button("##resetWindowsPosition", { buttonSide, buttonSide }))
            {
                ImGuiContext* ctx = ImGui::GetCurrentContext();

                for (ImGuiWindow* window : ctx->Windows)
                {
                    if (std::strncmp(window->Name, "DebugOverlay", 12 /* strlen("DebugOverlay") */) != 0)
                    {
                        window->PosFloat.x = 0.f;
                        window->PosFloat.y = 0.f;
                    }
                }
            }
            PopColor();

            DrawRect({ x, y }, { buttonSide * 0.5f, buttonSide * 0.5f });
        }
        ImGui::End();
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(1);

        for (ItemData& itemData : items)
        {
            if (itemData.shown)
            {
                itemData.item->Draw();
            }
        }
    }
}
}
