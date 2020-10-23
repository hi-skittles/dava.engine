#pragma once

#include "Base/BaseTypes.h"
#include "Base/EventDispatcher.h"
#include "Animation/Interpolation.h"

namespace DAVA
{
class AnimatedObject;
class AnimationManager;

/**
	\ingroup animationsystem
	\brief Animation is a base class that helps to handle animation process inside AnimationManager. 
	All animations inside the system derived from this class. You cannot use the Animation class directly to instantiate animations. 
	It instead defines the common interface and behavioral structure for all its subclasses.
 */
class Animation : public EventDispatcher
{
public:
    static const int32 INFINITE_LOOP = -1;

    enum
    {
        EVENT_ANIMATION_START = 1,
        EVENT_ANIMATION_END,
        EVENT_ANIMATION_CANCELLED,
    };

    /*
		animationState is binary flags that define animation state & it params
	 */
    enum eAnimationState
    {
        STATE_STOPPED = 0,
        STATE_IN_PROGRESS = 1 << 0,
        STATE_FINISHED = 1 << 1,
        STATE_PAUSED = 1 << 2,
        STATE_DELETE_ME = 1 << 3, // flag is set if animation is marked for deletion
        STATE_DELETED = 1 << 4, // flag is set if animation is deleted
    };

public:
    Animation(AnimatedObject* _owner, float32 _animationTimeLength, Interpolation::Func _interpolationFunc, int32 _defaultState = STATE_STOPPED);
    Animation(AnimatedObject* _owner, float32 _animationTimeLength, Interpolation::FuncType _interpolationFuncType = DAVA::Interpolation::LINEAR, int32 _defaultState = STATE_STOPPED);

    virtual void Reset();
    virtual void Start(int32 _groupId);
    virtual void Stop();
    virtual void Pause(bool isPaused);

    virtual void Update(float32 timeElapsed);

    // animation virtual events
    virtual void OnStart();
    virtual void OnStop();
    virtual void OnCancel();

    bool IsPaused() const;
    bool IsPlaying() const;

    inline void EnableReverse();
    void SetRepeatCount(int32 k);

    inline void SetTagId(int32 tag);
    inline int32 GetTagId() const;
    inline void SetTimeMultiplier(float32 m);
    inline float32 GetTimeMultiplier() const;
    inline AnimatedObject* GetOwner() const;
    inline DAVA::float32 GetAnimationLength() const;
    inline DAVA::int32 GetTrackId() const;

protected:
    ~Animation() override;

    int32 state = STATE_STOPPED;
    float32 time = 0.0f; // [0, animationTimeLength]
    float32 timeLength = 0.0f; // length of animation in seconds
    float32 normalizedTime = 0.0f; // [0, 1];
    Interpolation::Func interpolationFunc;
    AnimatedObject* owner = nullptr;

    Animation* next = nullptr;
    int32 groupId = 0; //	animation group id to group animations one after another
    int32 repeatCount = 0;

    int32 tagId = 0; // tag animations with numbers
    float32 timeMultiplier = 1.0f;
    bool reverseMode = false;

    friend class AnimationManager;
};

inline void Animation::EnableReverse()
{
    reverseMode = true;
}

inline void Animation::SetTagId(int32 tag)
{
    tagId = tag;
}

inline int32 Animation::GetTagId() const
{
    return tagId;
}

inline void Animation::SetTimeMultiplier(float32 m)
{
    timeMultiplier = m;
}

inline float32 Animation::GetTimeMultiplier() const
{
    return timeMultiplier;
}

inline AnimatedObject* Animation::GetOwner() const
{
    return owner;
}

inline float32 Animation::GetAnimationLength() const
{
    return timeLength;
}

inline int32 Animation::GetTrackId() const
{
    return groupId;
}
};
