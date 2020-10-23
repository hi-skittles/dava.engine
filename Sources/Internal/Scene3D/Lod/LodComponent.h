#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"
#include "Debug/DVAssert.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"

namespace DAVA
{
class Entity;

/**
Hold distance data for LodSystem.
*/
class LodComponent : public Component
{
public:
    /**
    Set distance for the specified `layerNum`. The behavior is undefined unless `layerNum` is in [0, MAX_LOD_LAYERS] range.
    */
    void SetLodLayerDistance(int32 layerNum, float32 distance);

    /**
    Return distance for the specified `layerNum`. The behavior is undefined unless `layerNum` is in [0, MAX_LOD_LAYERS] range.
    */
    float32 GetLodLayerDistance(int32 layerNum) const;

    /**
    Return current active lod layer number. Return value can be either INVALID_LOD_LAYER or belong to [0, MAX_LOD_LAYERS] range.
    */
    int32 GetCurrentLod() const;

    /**
    Enable _recursive update_ mode. See LodSystem for details.
    */
    void EnableRecursiveUpdate();

    static const int32 MAX_LOD_LAYERS = 4; //!< Maximum number of lod layers in one component.
    static const int32 INVALID_LOD_LAYER = -1; //!< Special value for invalid lod layer.
    static const float32 MIN_LOD_DISTANCE; //!< Lower bound for lod distance.
    static const float32 MAX_LOD_DISTANCE; //!< Upper bound for lod distance.
    static const float32 INVALID_DISTANCE; //!< Special value for invalid lod distance.

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

private:
    int32 currentLod = INVALID_LOD_LAYER;
    bool recursiveUpdate = false;
    Array<float32, MAX_LOD_LAYERS> distances = Array<float32, MAX_LOD_LAYERS>{ { 300.f, 600.f, 900.f, 1000.f } }; //cause list initialization for members not implemented in MSVC https://msdn.microsoft.com/en-us/library/dn793970.aspx

    friend class LodSystem;
    DAVA_VIRTUAL_REFLECTION(LodComponent, Component);
};

REGISTER_CLASS(LodComponent);

inline void LodComponent::SetLodLayerDistance(int32 layerNum, float32 distance)
{
    DVASSERT(0 <= layerNum && layerNum < MAX_LOD_LAYERS);
    distances[layerNum] = distance;
    GlobalEventSystem::Instance()->Event(this, EventSystem::LOD_DISTANCE_CHANGED);
}

inline void LodComponent::EnableRecursiveUpdate()
{
    recursiveUpdate = true;
    GlobalEventSystem::Instance()->Event(this, EventSystem::LOD_RECURSIVE_UPDATE_ENABLED);
}

inline int32 LodComponent::GetCurrentLod() const
{
    return currentLod;
}

inline float32 LodComponent::GetLodLayerDistance(int32 layerNum) const
{
    DVASSERT(0 <= layerNum && layerNum < MAX_LOD_LAYERS);
    return distances[layerNum];
}
};
