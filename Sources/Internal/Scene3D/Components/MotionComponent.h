#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Reflection/Reflection.h"
#include "Entity/Component.h"

namespace DAVA
{
class AnimationClip;
class MotionSystem;
class SimpleMotion;
class SkeletonAnimation;
class SkeletonComponent;
class MotionLayer;
class YamlNode;
class MotionComponent : public Component
{
public:
    MotionComponent() = default;
    ~MotionComponent();

    void TriggerEvent(const FastName& trigger); //TODO: *Skinning* make adequate naming
    void SetParameter(const FastName& parameterID, float32 value);

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    uint32 GetMotionLayersCount() const;
    MotionLayer* GetMotionLayer(uint32 index) const;

    const FilePath& GetDescriptorPath() const;
    void SetDescriptorPath(const FilePath& path);

    float32 GetPlaybackRate() const;
    void SetPlaybackRate(float32 rate);

    uint32 GetSingleAnimationRepeatsCount() const;
    void SetSingleAnimationRepeatsCount(uint32 repeastCount);

    const Vector3& GetRootOffsetDelta() const;

    Vector<FilePath> GetDependencies() const;

protected:
    void ReloadFromFile();
    void GetDependenciesRecursive(const YamlNode* node, Set<FilePath>* dependencies) const;

    FilePath descriptorPath;
    Vector<MotionLayer*> motionLayers;

    float32 playbackRate = 1.f;
    UnorderedMap<FastName, float32> parameters;

    Vector3 rootOffsetDelta;

    SimpleMotion* simpleMotion = nullptr;
    uint32 simpleMotionRepeatsCount = 0;

    DAVA_VIRTUAL_REFLECTION(MotionComponent, Component);

    friend class MotionSystem;
};

inline float32 MotionComponent::GetPlaybackRate() const
{
    return playbackRate;
}

inline void MotionComponent::SetPlaybackRate(float32 rate)
{
    playbackRate = rate;
}

inline uint32 MotionComponent::GetSingleAnimationRepeatsCount() const
{
    return simpleMotionRepeatsCount;
}

inline void MotionComponent::SetSingleAnimationRepeatsCount(uint32 repeastCount)
{
    simpleMotionRepeatsCount = repeastCount;
}

inline const Vector3& MotionComponent::GetRootOffsetDelta() const
{
    return rootOffsetDelta;
}

} //ns
