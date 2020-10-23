#ifndef __DAVAENGINE_SCENE_NODE_ANIMATION_H__
#define __DAVAENGINE_SCENE_NODE_ANIMATION_H__

#include "Base/BaseTypes.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneNodeAnimationKey.h"
namespace DAVA
{
/*
	TODO: efficient pack & unpack of animations (Vector / Quaternion spline approximation)
 */
class SceneNodeAnimationList;
class SceneNodeAnimation : public BaseObject
{
protected:
    ~SceneNodeAnimation() override;

public:
    SceneNodeAnimation(uint32 keyCount);

    SceneNodeAnimationKey& Intepolate(float32 t);

    void SetKey(int32 index, const SceneNodeAnimationKey& key);

    inline int32 GetKeyCount();
    inline SceneNodeAnimationKey* GetKeys();

    void SetDuration(float32 _duration);
    inline float32 GetDuration();

    void SetBindName(const FastName& bindName);
    void SetBindNode(Entity* bindNode);

    void SetInvPose(const Matrix4& mat);
    const Matrix4& GetInvPose() const;

    virtual void Update(float32 timeElapsed);
    virtual void Execute();

    inline float32 GetCurrentTime();
    inline float32 GetNormalDuration();

    Vector3 SetStartPosition(const Vector3& position);
    void ShiftStartPosition(const Vector3& position);

    void SetParent(SceneNodeAnimationList* list);
    SceneNodeAnimationList* GetParent();

    // this is node of animation this animation is supposed for
    Entity* bindNode = nullptr;
    FastName bindName;
    float32 weight = 0.0f;
    float32 delayTime = 0.0f;
    float32 currentTime = 0.0f;
    float32 duration = 0.0f;
    int32 startIdx = 0;
    uint32 keyCount = 0;
    SceneNodeAnimationKey* keys = nullptr;
    SceneNodeAnimationKey currentValue; //-V730_NOINIT
    Matrix4 invPose;
    bool apply = true;

private:
    SceneNodeAnimationList* parent = nullptr;
};

inline float32 SceneNodeAnimation::GetCurrentTime()
{
    return currentTime;
}

inline float32 SceneNodeAnimation::GetDuration()
{
    return duration;
}

inline float32 SceneNodeAnimation::GetNormalDuration()
{
    if (keyCount == 0)
        return 0;

    return keys[keyCount - 1].time;
}

inline int32 SceneNodeAnimation::GetKeyCount()
{
    return keyCount;
}

inline SceneNodeAnimationKey* SceneNodeAnimation::GetKeys()
{
    return keys;
}
};

#endif // __DAVAENGINE_SCENE_NODE_ANIMATION_KEY_H__
