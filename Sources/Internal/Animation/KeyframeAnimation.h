#ifndef __DAVAENGINE_KEYFRAME_ANIMATION_H__
#define __DAVAENGINE_KEYFRAME_ANIMATION_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Animation/Animation.h"
#include "Animation/AnimatedObject.h"

namespace DAVA
{
class KeyframeData : public BaseObject
{
public:
    struct Keyframe
    {
        Keyframe(int32 _frame, float32 _time)
        {
            frame = _frame;
            time = _time;
            next = 0;
        }

        int32 frame;
        float32 time;
        Keyframe* next;
    };

protected:
    virtual ~KeyframeData();

public:
    KeyframeData();

    void AddKeyframe(int32 frame, float32 time);
    float32 GetLength();

    Keyframe* head;
    Keyframe* tail;
};

class KeyframeAnimation : public Animation
{
protected:
    virtual ~KeyframeAnimation();

public:
    KeyframeAnimation(AnimatedObject* _owner, int32* _var, KeyframeData* data, float32 _animationTimeLength);

    virtual void Update(float32 timeElapsed);
    virtual void OnStart();

private:
    int32* var;
    KeyframeData* data;
    KeyframeData::Keyframe* currentFrame = nullptr;
};
};
#endif // __DAVAENGINE_KEYFRAME_ANIMATION_H__