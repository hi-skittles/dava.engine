#pragma once
#include "rhi_Pool.h"
#include "rhi_CommonImpl.h"

namespace rhi
{
namespace FrameLoop
{
void Initialize(uint32 framePoolSize);
void ProcessFrame();
bool FinishFrame(Handle sync); //return false if frame was empty
bool FrameReady();
uint32 FramesCount();
void AddPass(Handle pass);
void RejectFrames();
void SetFramePerfQueries(Handle startQuery, Handle endQuery);
}
}
