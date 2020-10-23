#pragma once

#include "Base/BaseTypes.h"
#include "Math/MathConstants.h"

namespace DAVA
{
/**
Calculate Gaussian coefficients. Used mostly for image filtering.
*/
class Gaussian
{
public:
    struct Values
    {
        Vector<float32> weights; //<! weights for each sample
        Vector<float32> offsets; //<! offsets for those samples
    };

    /**
    Return weights for 1D Gaussian function with specified 'halfKernelSize' and sigma 'q'. 
    */
    static Values CalculateWeights(int32 halfKernelSize, float32 q);

    /**
    Return optimized values from 'CalculateWeights' functions. Values are optimized for linear filtering. More details at http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/ 
    Reduces sample count to ((halfKernelSize-1)/2)+1
    */
    static Values CalculateOptimizedWeights(int32 halfKernelSize, float32 q);
};

Gaussian::Values Gaussian::CalculateWeights(int32 halfKernelSize, float32 q)
{
    float32 k1 = 1.f / (sqrtf(PI_2) * q);
    float32 k2 = -1.f / (2 * q * q);

    Vector<float32> weights(halfKernelSize);
    float32 x = 0;

    float32 step = 1.f / float32(halfKernelSize - 1);
    for (int32 i = 1; i <= halfKernelSize; ++i)
    {
        weights[halfKernelSize - i] = k1 * exp(k2 * x * x);
        x += step;
    }

    float32 sum = 0;
    for (float32 value : weights)
    {
        sum += 2.f * value;
    }
    //cause 'middle' weight should be used only once
    sum -= weights[halfKernelSize - 1];

    float32 d = 1.f / sum;
    for (float32& value : weights)
    {
        value *= d;
    }

    Vector<float32> offsets(halfKernelSize);
    float32 offset = float32(halfKernelSize - 1);
    for (int32 i = 0; i < halfKernelSize; ++i)
    {
        offsets[i] = offset;
        offset -= 1.f;
    }

    return Gaussian::Values{ weights, offsets };
}

Gaussian::Values Gaussian::CalculateOptimizedWeights(int32 halfKernelSize, float32 q)
{
    Gaussian::Values raw = CalculateWeights(halfKernelSize, q);
    int32 newSize = ((halfKernelSize - 1) / 2) + 1;

    Vector<float32> weights(newSize);
    for (int32 i = 0, j = 0; i < newSize - 1; i += 1, j += 2)
    {
        weights[i] = raw.weights[j] + raw.weights[j + 1];
    }
    weights[newSize - 1] = raw.weights[halfKernelSize - 1];

    Vector<float32> offsets(newSize);
    for (int32 i = 0, j = 0; i < newSize - 1; i += 1, j += 2)
    {
        offsets[i] = (raw.offsets[j] * raw.weights[j] + raw.offsets[j + 1] * raw.weights[j + 1]) / weights[i];
    }
    offsets[newSize - 1] = raw.offsets[halfKernelSize - 1];

    return Gaussian::Values{ weights, offsets };
}
}
