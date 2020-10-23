#pragma once

#include "Debug/DebugOverlayItem.h"

namespace DAVA
{
class DebugOverlayItemRenderOptions final : public DebugOverlayItem
{
public:
    DebugOverlayItemRenderOptions() = default;
    ~DebugOverlayItemRenderOptions() = default;

    String GetName() const override;
    void Draw() override;
};
}