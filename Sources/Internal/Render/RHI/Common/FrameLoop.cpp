#include "FrameLoop.h"
#include "rhi_Pool.h"
#include "../rhi_Public.h"
#include "rhi_Private.h"
#include "rhi_CommonImpl.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Concurrency/Thread.h"

namespace rhi
{
namespace FrameLoop
{
static DAVA::uint32 currentFrameNumber = 0;
static DAVA::Vector<CommonImpl::Frame> frames;
static DAVA::uint32 framePoolSize = 0;
static DAVA::uint32 frameToBuild = 0;
static DAVA::uint32 frameToExecute = 0;
static DAVA::Spinlock frameSync;

void Initialize(DAVA::uint32 _framePoolSize)
{
    framePoolSize = _framePoolSize;
    frames.resize(framePoolSize);
}

void RejectFrames()
{
    DAVA::LockGuard<DAVA::Spinlock> lock(frameSync);
    while (frameToExecute < frameToBuild)
    {
        DispatchPlatform::RejectFrame(frames[frameToExecute % framePoolSize]);
        frames[frameToExecute % framePoolSize].Reset();
        frameToExecute++;
    }
    if (frameToExecute >= framePoolSize)
    {
        frameToBuild -= framePoolSize;
        frameToExecute -= framePoolSize;
    }
    if (frames[frameToBuild].pass.size())
    {
        frames[frameToBuild].discarded = true;
    }
}

void ProcessFrame()
{
    DVASSERT(framePoolSize);

    bool presentResult = true;
    if (NeedRestoreResources())
    {
        RejectFrames();
    }
    else
    {
        bool frameRejected = false;
        if (frames[frameToExecute].discarded)
        {
            DispatchPlatform::RejectFrame(frames[frameToExecute]);
            frameRejected = true;
        }
        else
        {
            DAVA_PROFILER_CPU_SCOPE_WITH_FRAME_INDEX(DAVA::ProfilerCPUMarkerName::RHI_EXECUTE_FRAME, currentFrameNumber);

            frames[frameToExecute].frameNumber = currentFrameNumber++;
            DispatchPlatform::ExecuteFrame(frames[frameToExecute]);
        }
        frames[frameToExecute].Reset();
        frameSync.Lock();
        frameToExecute++;
        if (frameToExecute >= framePoolSize)
        {
            frameToBuild -= framePoolSize;
            frameToExecute -= framePoolSize;
        }
        frameSync.Unlock();

        if (!frameRejected)
        {
            DAVA_PROFILER_CPU_SCOPE(DAVA::ProfilerCPUMarkerName::RHI_DEVICE_PRESENT);
            presentResult = DispatchPlatform::PresentBuffer();
        }
    }

    if (!presentResult)
    {
        RejectFrames();
        DispatchPlatform::ResetBlock();
    }
}

bool FinishFrame(Handle sync)
{
    DispatchPlatform::FinishFrame();
    bool frameValid = false;
    frameSync.Lock();
    uint32 frameSlot = frameToBuild % framePoolSize;
    if (frames[frameSlot].pass.size() != 0)
    {
        frames[frameSlot].readyToExecute = true;
        frames[frameSlot].sync = sync;
        frameToBuild++;
        frameValid = true;
    }
    frameSync.Unlock();
    return frameValid;
}

bool FrameReady()
{
    DAVA::LockGuard<DAVA::Spinlock> lock(frameSync);
    return frames[frameToExecute].readyToExecute;
}

uint32 FramesCount()
{
    DAVA::LockGuard<DAVA::Spinlock> lock(frameSync);
    return frameToBuild - frameToExecute;
}

void AddPass(Handle pass)
{
    DAVA::LockGuard<DAVA::Spinlock> lock(frameSync);
    frames[frameToBuild % framePoolSize].pass.push_back(pass);
}

void SetFramePerfQueries(Handle startQuery, Handle endQuery)
{
    DAVA::LockGuard<DAVA::Spinlock> lock(frameSync);
    CommonImpl::Frame& frame = frames[frameToBuild % framePoolSize];
    frame.perfQueryStart = startQuery;
    frame.perfQueryEnd = endQuery;
}
}
}
