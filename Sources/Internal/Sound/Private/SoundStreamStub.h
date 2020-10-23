#pragma once

#include "Sound/SoundStream.h"

namespace DAVA
{
class SoundStreamStub : public SoundStream
{
public:
    SoundStreamStub()
    {
    }

    ~SoundStreamStub()
    {
    }
    void Play()
    {
    }
    void Pause()
    {
    }
};
}