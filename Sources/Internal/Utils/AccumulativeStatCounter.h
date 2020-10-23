#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
struct AccumulativeStatCounter
{
    void operator++()
    {
        AccumulativeStatCounter::operator+=(1.0f);
    }

    void operator++(int)
    {
        AccumulativeStatCounter::operator+=(1.0f);
    }

    void operator+=(float32 nextValue)
    {
        count++;
        average = (average * static_cast<float32>(count - 1) + nextValue) / static_cast<float32>(count);
    }

    uint32 count = 0;
    float32 average = 0.0f;
};
}
