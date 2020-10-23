#pragma once
#include "../rhi_Public.h"
#include "../rhi_Type.h"
#include "rhi_CommonImpl.h"
#include "Concurrency/Thread.h"

namespace rhi
{
namespace RenderLoop
{
void Present(); // called from main thread

void InitializeRenderLoop(uint32 frameCount, DAVA::Thread::eThreadPriority priority, int32 bindToProcessor);
void UninitializeRenderLoop();

void SuspendRender();
void SuspendRenderAfterFrame();
void ResumeRender();

void IssueImmediateCommand(CommonImpl::ImmediateCommand* command); //blocking until complete
void CheckImmediateCommand(); //called from render thread only

void SetResetPending();

void ScheduleResourceDeletion(Handle handle, ResourceType resourceType);
HSyncObject GetCurrentFrameSyncObject();
}
}