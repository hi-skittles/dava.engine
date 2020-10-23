#pragma once

#include "Base/BaseTypes.h"
#include "Entity/Component.h"

namespace DAVA
{
class ParticleEmitter;
class ParticleEffectComponent;
class SkeletonComponent;
class MotionComponent;
class Entity;
class RenderObject;
class Light;
class LightComponent;
class Landscape;
class Camera;
class LodComponent;
class SoundComponent;
class SoundEvent;
class SwitchComponent;
class QualitySettingsComponent;
class TransformComponent;
class RenderComponent;
class VegetationRenderObject;
class CustomPropertiesComponent;
class KeyedArchive;
class SpeedTreeComponent;
class WindComponent;
class WaveComponent;
class SpeedTreeObject;
class AnimationComponent;
class PathComponent;
class WaypointComponent;
class EdgeComponent;
class SnapToLandscapeControllerComponent;
class StaticOcclusionComponent;
class StaticOcclusionDebugDrawComponent;
class GeoDecalComponent;

bool HasComponent(const Entity* fromEntity, const Type* componentType);

ParticleEffectComponent* GetEffectComponent(const Entity* fromEntity);
AnimationComponent* GetAnimationComponent(const Entity* fromEntity);
TransformComponent* GetTransformComponent(const Entity* fromEntity);
RenderComponent* GetRenderComponent(const Entity* fromEntity);
SkeletonComponent* GetSkeletonComponent(const Entity* fromEntity);
MotionComponent* GetMotionComponent(const Entity* fromEntity);
RenderObject* GetRenderObject(const Entity* fromEntity);
VegetationRenderObject* GetVegetation(const Entity* fromEntity);
SpeedTreeObject* GetSpeedTreeObject(const Entity* fromEntity);
SnapToLandscapeControllerComponent* GetSnapToLandscapeControllerComponent(const Entity* fromEntity);
StaticOcclusionComponent* GetStaticOcclusionComponent(const Entity* fromEntity);
StaticOcclusionDebugDrawComponent* GetStaticOcclusionDebugDrawComponent(const Entity* fromEntity);
GeoDecalComponent* GetGeoDecalComponent(const Entity* fromEntity);

Light* GetLight(const Entity* fromEntity);
LightComponent* GetLightComponent(const Entity* fromEntity);
Landscape* GetLandscape(const Entity* fromEntity);

Camera* GetCamera(const Entity* fromEntity);

SoundComponent* GetSoundComponent(const Entity* fromEntity);

LodComponent* GetLodComponent(const Entity* fromEntity);
SwitchComponent* GetSwitchComponent(const Entity* fromEntity);
ParticleEffectComponent* GetParticleEffectComponent(const Entity* fromEntity);

uint32 GetLodLayersCount(const Entity* fromEntity);
uint32 GetLodLayersCount(LodComponent* fromComponent);

void RecursiveProcessMeshNode(Entity* curr, void* userData, void (*process)(Entity*, void*));

Entity* FindLandscapeEntity(Entity* rootEntity);
Landscape* FindLandscape(Entity* rootEntity);
Entity* FindVegetationEntity(Entity* rootEntity);
VegetationRenderObject* FindVegetation(Entity* rootEntity);

SpeedTreeComponent* GetSpeedTreeComponent(const Entity* fromEntity);
WindComponent* GetWindComponent(const Entity* fromEntity);
WaveComponent* GetWaveComponent(const Entity* fromEntity);

QualitySettingsComponent* GetQualitySettingsComponent(const Entity* fromEntity);

CustomPropertiesComponent* GetCustomProperties(const Entity* fromEntity);
CustomPropertiesComponent* GetOrCreateCustomProperties(Entity* fromEntity);
KeyedArchive* GetCustomPropertiesArchieve(const Entity* fromEntity);
VariantType* GetCustomPropertiesValueRecursive(Entity* fromEntity, const String& name);

PathComponent* GetPathComponent(const Entity* fromEntity);
WaypointComponent* GetWaypointComponent(const Entity* fromEntity);
}
