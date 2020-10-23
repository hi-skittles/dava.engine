#pragma once

#include "Sound/SoundEvent.h"

namespace DAVA
{
class SoundEventStub : public SoundEvent
{
public:
    SoundEventStub()
    {
    }

    bool IsActive() const
    {
        return false;
    }
    bool Trigger()
    {
        return true;
    }
    void Stop(bool force = false)
    {
    } //force = true: stop the event immediately, ignoring fadeout and etc.
    void SetPaused(bool paused)
    {
    }

    void SetVolume(float32 volume)
    {
    }

    void SetSpeed(float32 speed)
    {
    }

    void SetPosition(const Vector3& position)
    {
    }
    void SetDirection(const Vector3& direction)
    {
    }
    void UpdateInstancesPosition()
    {
    }
    void SetVelocity(const Vector3& velocity)
    {
    }

    float32 GetParameterValue(const FastName& paramName)
    {
        return 0.0f;
    }

    void SetParameterValue(const FastName& paramName, float32 value)
    {
    }

    bool IsParameterExists(const FastName& paramName)
    {
        return false;
    }

    void GetEventParametersInfo(Vector<SoundEventParameterInfo>& paramsInfo) const
    {
    }

    String GetEventName() const
    {
        return String("Nill");
    }
    float32 GetMaxDistance() const
    {
        return 0.0f;
    }
};
}