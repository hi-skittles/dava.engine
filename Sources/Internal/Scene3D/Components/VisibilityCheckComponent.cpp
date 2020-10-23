#include "VisibilityCheckComponent.h"
#include "Scene3D/Entity.h"
#include "Render/Texture.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Utils/Random.h"

namespace DAVA
{
REGISTER_CLASS(VisibilityCheckComponent)

DAVA_VIRTUAL_REFLECTION_IMPL(VisibilityCheckComponent)
{
    ReflectionRegistrator<VisibilityCheckComponent>::Begin()
    .ConstructorByPointer()
    .Field("enabled", &VisibilityCheckComponent::IsEnabled, &VisibilityCheckComponent::SetEnabled)[M::DisplayName("Enabled")]
    .Field("radius", &VisibilityCheckComponent::GetRadius, &VisibilityCheckComponent::SetRadius)[M::DisplayName("Radius")]
    .Field("distanceBetweenPoints", &VisibilityCheckComponent::GetDistanceBetweenPoints, &VisibilityCheckComponent::SetDistanceBetweenPoints)[M::DisplayName("Distance Between Points")]
    .Field("maximumDistance", &VisibilityCheckComponent::GetMaximumDistance, &VisibilityCheckComponent::SetMaximumDistance)[M::DisplayName("Maximum Distance")]
    .Field("upAngle", &VisibilityCheckComponent::GetUpAngle, &VisibilityCheckComponent::SetUpAngle)[M::DisplayName("Up Angle")]
    .Field("downAngle", &VisibilityCheckComponent::GetDownAngle, &VisibilityCheckComponent::SetDownAngle)[M::DisplayName("Down Angle")]
    .Field("verticalVariance", &VisibilityCheckComponent::GetVerticalVariance, &VisibilityCheckComponent::SetVerticalVariance)[M::DisplayName("Vertical Variance")]
    .Field("color", &VisibilityCheckComponent::GetColor, &VisibilityCheckComponent::SetColor)[M::DisplayName("Color")]
    .Field("normalizedColor", &VisibilityCheckComponent::ShouldNormalizeColor, &VisibilityCheckComponent::SetShouldNormalizeColor)[M::DisplayName("Normalize Color")]
    .Field("placeOnLandscape", &VisibilityCheckComponent::ShouldPlaceOnLandscape, &VisibilityCheckComponent::SetShouldPlaceOnLandscape)[M::DisplayName("Place on Landscape")]
    .Field("heightAboveLandscape", &VisibilityCheckComponent::GetHeightAboveLandscape, &VisibilityCheckComponent::SetHeightAboveLandscape)[M::DisplayName("Height Above the Landscape")]
    .End();
}

VisibilityCheckComponent::VisibilityCheckComponent()
{
}

Component* VisibilityCheckComponent::Clone(Entity* toEntity)
{
    auto result = new VisibilityCheckComponent();
    result->SetEntity(toEntity);

    result->color = color;
    result->radius = radius;
    result->distanceBetweenPoints = distanceBetweenPoints;
    result->upAngle = upAngle;
    result->downAngle = downAngle;
    result->verticalVariance = verticalVariance;
    result->maximumDistance = maximumDistance;
    result->heightAboveLandscape = heightAboveLandscape;
    result->isEnabled = isEnabled;
    result->shouldNormalizeColor = shouldNormalizeColor;
    result->shouldPlaceOnLandscape = shouldPlaceOnLandscape;
    result->shouldRebuildPointSet = true;
    result->isValid = false;

    return result;
}

float32 VisibilityCheckComponent::GetRadius() const
{
    return radius;
}

void VisibilityCheckComponent::SetRadius(float32 r)
{
    radius = r;
    shouldRebuildPointSet = true;
    Invalidate();
}

float32 VisibilityCheckComponent::GetDistanceBetweenPoints() const
{
    return distanceBetweenPoints;
}

void VisibilityCheckComponent::SetDistanceBetweenPoints(float32 d)
{
    distanceBetweenPoints = d;
    shouldRebuildPointSet = true;
    Invalidate();
}

const Color& VisibilityCheckComponent::GetColor() const
{
    return color;
}

void VisibilityCheckComponent::SetColor(const Color& clr)
{
    color = clr;
    color.a = 1.0f;
    Invalidate();
}

bool VisibilityCheckComponent::ShouldRebuildPoints() const
{
    return shouldRebuildPointSet;
}

bool VisibilityCheckComponent::IsValid() const
{
    return isValid;
}

void VisibilityCheckComponent::SetValid()
{
    isValid = true;
    shouldRebuildPointSet = false;
}

void VisibilityCheckComponent::Invalidate()
{
    isValid = false;
}

float32 VisibilityCheckComponent::GetUpAngle() const
{
    return upAngle;
}

void VisibilityCheckComponent::SetUpAngle(float32 value)
{
    upAngle = std::max(0.0f, std::min(90.0f, value));
    Invalidate();
}

float32 VisibilityCheckComponent::GetDownAngle() const
{
    return downAngle;
}

void VisibilityCheckComponent::SetDownAngle(float32 value)
{
    downAngle = std::max(0.0f, std::min(90.0f, value));
    Invalidate();
}

bool VisibilityCheckComponent::IsEnabled() const
{
    return isEnabled;
}

void VisibilityCheckComponent::SetEnabled(bool value)
{
    isEnabled = value;
    shouldRebuildPointSet = true;
    Invalidate();
}

bool VisibilityCheckComponent::ShouldNormalizeColor() const
{
    return shouldNormalizeColor;
}

void VisibilityCheckComponent::SetShouldNormalizeColor(bool value)
{
    shouldNormalizeColor = value;
}

float32 VisibilityCheckComponent::GetVerticalVariance() const
{
    return verticalVariance;
}

void VisibilityCheckComponent::SetVerticalVariance(float32 value)
{
    verticalVariance = std::max(0.0f, value);
    shouldRebuildPointSet = true;
    Invalidate();
}

float32 VisibilityCheckComponent::GetMaximumDistance() const
{
    return maximumDistance;
}

void VisibilityCheckComponent::SetMaximumDistance(float32 value)
{
    maximumDistance = std::max(0.0f, value);
}

bool VisibilityCheckComponent::ShouldPlaceOnLandscape() const
{
    return shouldPlaceOnLandscape;
}

void VisibilityCheckComponent::SetShouldPlaceOnLandscape(bool value)
{
    shouldPlaceOnLandscape = value;
    Invalidate();
}

float32 VisibilityCheckComponent::GetHeightAboveLandscape() const
{
    return heightAboveLandscape;
}

void VisibilityCheckComponent::SetHeightAboveLandscape(float32 value)
{
    heightAboveLandscape = DAVA::Max(0.0f, value);
    Invalidate();
}

void VisibilityCheckComponent::Serialize(DAVA::KeyedArchive* archive, DAVA::SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    archive->SetColor("vsc.color", color);
    archive->SetFloat("vsc.radius", radius);
    archive->SetFloat("vsc.distanceBetweenPoints", distanceBetweenPoints);
    archive->SetFloat("vsc.upAngle", upAngle);
    archive->SetFloat("vsc.downAngle", downAngle);
    archive->SetFloat("vsc.verticalVariance", verticalVariance);
    archive->SetFloat("vsc.maximumDistance", maximumDistance);
    archive->SetFloat("vsc.heightAboveLandscape", heightAboveLandscape);
    archive->SetBool("vsc.isEnabled", isEnabled);
    archive->SetBool("vsc.shouldNormalizeColor", shouldNormalizeColor);
    archive->SetBool("vsc.shouldPlaceOnLandscape", shouldPlaceOnLandscape);
}

void VisibilityCheckComponent::Deserialize(DAVA::KeyedArchive* archive, DAVA::SerializationContext* serializationContext)
{
    color = archive->GetColor("vsc.color");
    radius = archive->GetFloat("vsc.radius");
    distanceBetweenPoints = archive->GetFloat("vsc.distanceBetweenPoints");
    upAngle = archive->GetFloat("vsc.upAngle");
    downAngle = archive->GetFloat("vsc.downAngle");
    verticalVariance = archive->GetFloat("vsc.verticalVariance");
    maximumDistance = archive->GetFloat("vsc.maximumDistance");
    heightAboveLandscape = archive->GetFloat("vsc.heightAboveLandscape");
    isEnabled = archive->GetBool("vsc.isEnabled");
    shouldNormalizeColor = archive->GetBool("vsc.shouldNormalizeColor");
    shouldPlaceOnLandscape = archive->GetBool("vsc.shouldPlaceOnLandscape");
    shouldRebuildPointSet = true;
    Invalidate();
}

bool VisibilityCheckComponent::GetDebugDrawEnabled() const
{
    return debugDrawEnabled;
}

void VisibilityCheckComponent::SetDebugDrawEnabled(bool value)
{
    debugDrawEnabled = value;
}
}