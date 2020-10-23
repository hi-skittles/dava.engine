#pragma once

#include "Debug/DebugOverlayItem.h"

namespace DAVA
{
class DebugOverlayItemRenderStats final : public DebugOverlayItem
{
public:
    DebugOverlayItemRenderStats() = default;
    ~DebugOverlayItemRenderStats() = default;

    String GetName() const override;
    void Draw() override;

private:
    void AddUIntStat(const char* name, uint32 value);
    void AddPercentageStat(const char* name, float32 value);
};
}
