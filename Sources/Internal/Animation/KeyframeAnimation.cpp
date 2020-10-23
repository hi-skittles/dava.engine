#include "Base/BaseMath.h"
#include "Logger/Logger.h"
#include "Animation/KeyframeAnimation.h"

namespace DAVA
{
KeyframeData::KeyframeData()
{
    head = 0;
    tail = 0;
}

KeyframeData::~KeyframeData()
{
    Keyframe* node = head;
    while (node)
    {
        Keyframe* next = node->next;
        delete node;
        node = next;
    }
}

void KeyframeData::AddKeyframe(int frame, float32 time)
{
    Keyframe* keyFrame = new Keyframe(frame, time);
    if (head == 0)
    {
        head = tail = keyFrame;
    }
    else
    {
        tail->next = keyFrame;
    }
}

float32 KeyframeData::GetLength()
{
    return tail->time;
}

KeyframeAnimation::KeyframeAnimation(AnimatedObject* _owner, int* _var, KeyframeData* _data, float32 _animationTimeLength)
    : Animation(_owner, _animationTimeLength, Interpolation::LINEAR)
{
    var = _var;
    data = _data;
    data->Retain();
}

KeyframeAnimation::~KeyframeAnimation()
{
    SafeRelease(data);
}

void KeyframeAnimation::OnStart()
{
    currentFrame = data->head;
}

void KeyframeAnimation::Update(float32 timeElapsed)
{
    //Animation::Update(timeElapsed);
    if (state & STATE_IN_PROGRESS)
    {
        time += timeElapsed;
        if (time >= timeLength)
        {
            if (repeatCount == 0)
            {
                time = timeLength;
                normalizedTime = 1.0f;
                state |= STATE_FINISHED;
            }
            else
            {
                time -= timeLength;
                repeatCount--;
            }
        }

        if (currentFrame->next == 0) // we've found last keyframe stop immediatelly
        {
            state |= STATE_FINISHED;
            return;
        }

        while (time >= currentFrame->next->time)
        {
            currentFrame = currentFrame->next;
            if (currentFrame->next == 0)
            {
                state |= STATE_FINISHED;
                return;
            }
        }

        // check isn't necessary
        if (currentFrame && currentFrame->next)
        {
            float32 frameDelta = time - currentFrame->time;
            float32 duration = currentFrame->next->time - currentFrame->time;
            float32 frame = currentFrame->frame + (currentFrame->next->frame - currentFrame->frame) * frameDelta / duration;
            *var = static_cast<int>(frame);
            //Logger::FrameworkDebug("%0.2f", frame);
        }
    }
    //*var = startValue + (endValue - startValue) * normalizedTime;
}
};