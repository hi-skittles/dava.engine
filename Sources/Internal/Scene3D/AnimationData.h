#ifndef __DAVAENGINE_ANIMATION_DATA_H__
#define __DAVAENGINE_ANIMATION_DATA_H__

#include "Base/BaseTypes.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneNodeAnimationKey.h"
#include "Scene3D/DataNode.h"
namespace DAVA
{
class AnimationData : public DataNode
{
protected:
    virtual ~AnimationData();

public:
    AnimationData();

    SceneNodeAnimationKey Interpolate(float32 t, uint32& startIdxCache) const;
    SceneNodeAnimationKey GetKeyForFrame(int32 frameIndex) const;

    void AddKey(const SceneNodeAnimationKey& key);

    int32 GetKeyCount() const;

    void SetDuration(float32 _duration);
    float32 GetDuration() const;

    void SetInvPose(const Matrix4& mat);
    const Matrix4& GetInvPose() const;

    void Save(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Load(KeyedArchive* archive, SerializationContext* serializationContext) override;

    AnimationData* Clone() const;

    void BakeTransform(const Matrix4& transform);

public:
    Matrix4 invPose;
    DAVA::Vector<SceneNodeAnimationKey> keys;
    float32 duration;
};

inline float32 AnimationData::GetDuration() const
{
    return duration;
}

inline int32 AnimationData::GetKeyCount() const
{
    return static_cast<int32>(keys.size());
}
};

#endif // __DAVAENGINE_ANIMATION_DATA_H__
