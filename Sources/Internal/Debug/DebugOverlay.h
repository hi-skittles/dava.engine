#pragma once

#include "Base/BaseTypes.h"
#include "Base/Vector.h"

namespace DAVA
{
namespace Private
{
class EngineBackend;
}

class Window;
class DebugOverlayItem;
class DebugOverlayItemEngineSettings;
class DebugOverlayItemLogger;
class DebugOverlayItemRenderStats;
class DebugOverlayItemRenderOptions;
class DebugOverlayItemProfiler;

/**
    Class representing visual overlay meant for debugging.

    It provides features for extending and showing custom information or UI via `DebugOverlayItem`.
    Each `DebugOverlayItem` is shown as a menu item which can be enabled or disabled. Multiple items can be enabled simultaneously.
*/
class DebugOverlay final
{
public:
    /** Show overlay. */
    void Show();

    /** Hide overlay. */
    void Hide();

    /** Return `true` if overlay is active, `false` otherwise. */
    bool IsShown() const;

    /** Add `overlayItem` to the menu. */
    void RegisterItem(DebugOverlayItem* overlayItem);

    /** Remove `overlayItem` from the menu. The item must be registered. */
    void UnregisterItem(DebugOverlayItem* overlayItem);

    /**
        Enable drawing `overlayItem`.
        Calling this method has the same effect as marking the checkbox as enabled in the menu.
        `overlayItem` must be registered.    
    */
    void ShowItem(DebugOverlayItem* overlayItem);

    /**
        Disable drawing of `overlayItem`.
        Calling this method has the same effect as marking the checkbox as disabled in the menu.
        `overlayItem` must be registered.
    */
    void HideItem(DebugOverlayItem* overlayItem);

private:
    DebugOverlay();
    ~DebugOverlay();
    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    void OnUpdate(Window* window, float32 timeDelta);

    void RegisterDefaultItems();
    void UnregisterDefaultItems();

private:
    struct ItemData
    {
        DebugOverlayItem* item;
        String name;
        bool shown;
    };

    bool shown = false;
    Vector<ItemData> items;

    // Items added by default
    std::unique_ptr<DebugOverlayItemEngineSettings> defaultItemEngineSettings;
    std::unique_ptr<DebugOverlayItemLogger> defaultItemLogger;
    std::unique_ptr<DebugOverlayItemRenderOptions> defaultItemRenderOptions;
    std::unique_ptr<DebugOverlayItemRenderStats> defaultItemRenderStats;
    std::unique_ptr<DebugOverlayItemProfiler> defaultItemProfiler;

    // For creation
    friend class Private::EngineBackend;
};
}