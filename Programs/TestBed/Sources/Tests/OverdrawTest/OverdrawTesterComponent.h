#pragma once

#include "Base/BaseTypes.h"
#include "Entity/Component.h"

namespace DAVA
{
class Entity;
};

namespace OverdrawPerformanceTester
{
using DAVA::uint32;
using DAVA::float32;

class OverdrawTesterRenderObject;

class OverdrawTesterComponent : public DAVA::Component
{
public:
    OverdrawTesterComponent(DAVA::uint16 textureResolution_, DAVA::uint8 overdrawScreenCount);
    ~OverdrawTesterComponent();

    inline OverdrawTesterRenderObject* GetRenderObject() const;
    Component* Clone(DAVA::Entity* toEntity) override;

    inline uint32 GetStepsCount() const;
    inline float32 GetStepOverdraw() const;

private:
    OverdrawTesterRenderObject* renderObject;
    DAVA::uint32 stepsCount = 10;
    DAVA::uint16 textureResolution = 2048;

    static const DAVA::uint8 addOverdrawPercent;

    DAVA_VIRTUAL_REFLECTION(CameraComponent, DAVA::Component);
};

OverdrawTesterRenderObject* OverdrawTesterComponent::GetRenderObject() const
{
    return renderObject;
}

DAVA::uint32 OverdrawTesterComponent::GetStepsCount() const
{
    return stepsCount;
}

DAVA::float32 OverdrawTesterComponent::GetStepOverdraw() const
{
    return addOverdrawPercent;
}
}
