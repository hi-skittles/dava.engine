#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/**
    FpsMeter is a simple class for measuring FPS.
 
    Right way to use this class:
    1. create instance of FpsMeter. Say, FpsMeter fpsMeter;
    2. call fpsMeter.Update on every update from engine and pass elapsedTime from engine's update to it.
    3. check fpsMeter.IsFpsReady() after each fpsMeter.Update call. If functions returns true, you can get measured FPS through fpsMeter.GetFps() call.
    
    FpsMeter works permanently, which means that after evaluating FPS value it begins to evaluate next FPS value
 
    For example:
 
    class A
    {
        A()
        {
            engine.update.Connect(this, &A::Update);
        }
 
        void Update(float32 elapsedSec)
        {
            fm.Update(elapsedSec);
            if (fm.IsFpsReady())
            {
                float32 currentFps = fm.GetFps();
                ...
            }
        }
 
        FpsMeter fm;
    }
*/

class FpsMeter
{
public:
    /** Create FpsMeter instance which will be calculating FPS values each `duration` seconds. */
    explicit FpsMeter(float32 duration = 1.f);

    /** 
    Pass time in seconds since last Update call.
    Normally this function should be invoked on each update tick from engine.
    */
    void Update(float32 timeElapsed);

    /** Return true if next FPS value is ready. */
    bool IsFpsReady() const;

    /** Return last measured FPS value. */
    float32 GetFps() const;

private:
    float32 measureDurationSec = 0.f;
    float32 elapsedSec = 0.f;
    uint32 elapsedFrames = 0;
    float32 lastFps = 0.f;
    bool fpsIsReady = false;
};

inline bool FpsMeter::IsFpsReady() const
{
    return fpsIsReady;
}

inline float32 FpsMeter::GetFps() const
{
    return lastFps;
}
}
