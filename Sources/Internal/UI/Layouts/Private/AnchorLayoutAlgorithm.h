#pragma once

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "UI/Layouts/Private/ControlLayoutData.h"
#include "UI/Layouts/Private/Layouter.h"

namespace DAVA
{
class AnchorLayoutAlgorithm
{
public:
    AnchorLayoutAlgorithm(Layouter& layouter);
    ~AnchorLayoutAlgorithm();

    void Apply(ControlLayoutData& data, Vector2::eAxis axis, bool onlyForIgnoredControls);
    void Apply(ControlLayoutData& data, Vector2::eAxis axis, bool onlyForIgnoredControls, int32 firstIndex, int32 lastIndex);

    static void ApplyAnchor(ControlLayoutData& data, Vector2::eAxis axis, float32 min, float32 max, bool isRtl, const Layouter& layouter);

private:
    Layouter& layouter;
};
}
