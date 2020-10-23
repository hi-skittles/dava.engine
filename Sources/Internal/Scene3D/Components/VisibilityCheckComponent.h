#pragma once

#include "Math/Color.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class VisibilityCheckComponent : public Component
{
public:
    VisibilityCheckComponent();
    Component* Clone(Entity* toEntity) override;

    bool IsEnabled() const;
    void SetEnabled(bool);

    bool ShouldNormalizeColor() const;
    void SetShouldNormalizeColor(bool);

    float32 GetRadius() const;
    void SetRadius(float32);

    float32 GetDistanceBetweenPoints() const;
    void SetDistanceBetweenPoints(float32);

    float32 GetMaximumDistance() const;
    void SetMaximumDistance(float32);

    float32 GetVerticalVariance() const;
    void SetVerticalVariance(float32);

    const Color& GetColor() const;
    void SetColor(const Color&);

    float32 GetUpAngle() const;
    void SetUpAngle(float32);

    float32 GetDownAngle() const;
    void SetDownAngle(float32);

    bool ShouldPlaceOnLandscape() const;
    void SetShouldPlaceOnLandscape(bool);

    float32 GetHeightAboveLandscape() const;
    void SetHeightAboveLandscape(float32);

    bool GetDebugDrawEnabled() const;
    void SetDebugDrawEnabled(bool);

    bool ShouldRebuildPoints() const;
    bool IsValid() const;
    void SetValid();
    void Invalidate();

    void Serialize(DAVA::KeyedArchive* archive, DAVA::SerializationContext* serializationContext) override;
    void Deserialize(DAVA::KeyedArchive* archive, DAVA::SerializationContext* serializationContext) override;

private:
    Color color = Color(1.0f, 0.0f, 0.0f, 1.0f);
    float32 radius = 20.0f;
    float32 distanceBetweenPoints = 4.0f;
    float32 upAngle = 45.0f;
    float32 downAngle = 45.0f;
    float32 verticalVariance = 0.0f;
    float32 maximumDistance = 250.0f;
    float32 heightAboveLandscape = 3.0f;
    bool isValid = false;
    bool isEnabled = true;
    bool shouldNormalizeColor = true;
    bool shouldPlaceOnLandscape = true;
    bool shouldRebuildPointSet = true;
    bool debugDrawEnabled = false;

    DAVA_VIRTUAL_REFLECTION(VisibilityCheckComponent, Component);
};
}
