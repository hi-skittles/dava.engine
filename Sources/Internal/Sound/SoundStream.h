#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class SoundStreamDelegate
{
public:
    virtual ~SoundStreamDelegate() = default;
    virtual void PcmDataCallback(uint8* data, uint32 datalen) = 0;
};

class SoundStream
{
public:
    virtual ~SoundStream() = default;

    virtual void Play() = 0;
    virtual void Pause() = 0;

    static uint32 GetDefaultSampleRate();

private:
    static const uint32 outSampleRate = 44100;
};

inline uint32 SoundStream::GetDefaultSampleRate()
{
    return outSampleRate;
}
}