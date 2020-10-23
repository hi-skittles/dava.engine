#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/AllocatorFactory.h"

namespace DAVA
{
struct Particle
{
    IMPLEMENT_POOL_ALLOCATOR(Particle, 1000);

    Particle* next = nullptr;

    float32 life = 0.0f;
    float32 lifeTime = 0.0f;

    Vector3 position = {};
    Vector3 speed = {};

    float32 angle = 0.0f;
    float32 spin = 0.0f;

    int32 frame = 0;
    float32 animTime = 0.0f;

    float32 baseFlowSpeed = 0.0f;
    float32 currFlowSpeed = 0.0f;
    float32 baseFlowOffset = 0.0f;
    float32 currFlowOffset = 0.0f;

    float32 baseNoiseScale = 0.0f;
    float32 currNoiseScale = 0.0f;
    float32 baseNoiseUScrollSpeed = 0.0f;
    float32 currNoiseUOffset = 0.0f;
    float32 baseNoiseVScrollSpeed = 0.0f;
    float32 currNoiseVOffset = 0.0f;

    float32 currRadius = 0.0f; //for bbox computation
    float32 alphaRemap = 0.0f;
    Vector2 baseSize = {}, currSize = {};

    Color color = {};

    int32 positionTarget = 0; //superemitter particles only
};
}
