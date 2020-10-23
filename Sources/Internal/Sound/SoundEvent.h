#pragma once

#include "Base/BaseTypes.h"
#include "Base/EventDispatcher.h"

namespace DAVA
{
class SoundEvent : public EventDispatcher
{
public:
    struct SoundEventParameterInfo
    {
        String name;
        float32 maxValue;
        float32 minValue;
    };

    enum eSoundEventCallbackType
    {
        EVENT_TRIGGERED,
        EVENT_END, /* Called when an event is stopped for any reason. */

        EVENT_COUNT
    };

    enum SoundEventCreateFlags
    {
        SOUND_EVENT_CREATE_DEFAULT = 0, //2D; static; no loop
        SOUND_EVENT_CREATE_STREAM = (1 << 0),
        SOUND_EVENT_CREATE_3D = (1 << 1),
        SOUND_EVENT_CREATE_LOOP = (1 << 2)
    };

    SoundEvent()
        : volume(1.0f)
        , speed(1.0f)
        , isDirectional(false)
    {
    }

    virtual bool IsActive() const = 0;
    virtual bool Trigger() = 0;
    virtual void Stop(bool force = false) = 0; //force = true: stop the event immediately, ignoring fadeout and etc.
    virtual void SetPaused(bool paused) = 0;

    virtual void SetVolume(float32 volume) = 0;
    inline float32 GetVolume() const;

    virtual void SetSpeed(float32 speed) = 0;
    inline float32 GetSpeed() const;

    virtual void SetPosition(const Vector3& position) = 0;
    virtual void SetDirection(const Vector3& direction) = 0;
    virtual void UpdateInstancesPosition() = 0;
    virtual void SetVelocity(const Vector3& velocity) = 0;

    inline bool IsDirectional() const;

    virtual void SetParameterValue(const FastName& paramName, float32 value) = 0;
    virtual float32 GetParameterValue(const FastName& paramName) = 0;
    virtual bool IsParameterExists(const FastName& paramName) = 0;

    virtual void GetEventParametersInfo(Vector<SoundEventParameterInfo>& paramsInfo) const = 0;

    virtual String GetEventName() const = 0;
    virtual float32 GetMaxDistance() const = 0;

protected:
    float32 volume;
    float32 speed;
    bool isDirectional;
};

inline float32 SoundEvent::GetVolume() const
{
    return volume;
}

inline float32 SoundEvent::GetSpeed() const
{
    return speed;
}

inline bool SoundEvent::IsDirectional() const
{
    return isDirectional;
}
};
