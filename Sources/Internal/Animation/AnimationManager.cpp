#include "Animation/AnimationManager.h"
#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Job/JobManager.h"
#include "Render/Renderer.h"

#include <typeinfo>

namespace DAVA
{
AnimationManager::~AnimationManager()
{
    DeleteAllFinishedAnimation();
    DVASSERT(animations.empty());
    DVASSERT(registeredAnimations.empty());
}

void AnimationManager::AddAnimation(Animation* animation)
{
    Function<void()> fn = Bind(&AnimationManager::AddAnimationInternal, this, animation);
    GetEngineContext()->jobManager->CreateMainJob(fn);
}

void AnimationManager::AddAnimationInternal(Animation* animation)
{
    registeredAnimations.push_back(animation);
    animations.push_back(RefPtr<Animation>(animation));
}

void AnimationManager::RemoveAnimation(const Animation* animation)
{
    Function<void()> fn = Bind(&AnimationManager::RemoveAnimationInternal, this, animation);
    GetEngineContext()->jobManager->CreateMainJob(fn);
}

void AnimationManager::RemoveAnimationInternal(const Animation* animation)
{
    auto it = std::find(begin(registeredAnimations), end(registeredAnimations), animation);
    if (it != registeredAnimations.end())
    {
        registeredAnimations.erase(it);
    }
}

void AnimationManager::StopAnimations()
{
    DVASSERT(Thread::IsMainThread());

    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        Animation* animation = it->Get();

        animation->owner = 0; // zero owner to avoid any issues (it was a problem with DumpState, when animations was deleted before).
        animation->state &= ~Animation::STATE_IN_PROGRESS;
        animation->state &= ~Animation::STATE_FINISHED;
        animation->state |= Animation::STATE_DELETE_ME;
    }
}

void AnimationManager::DeleteAnimations(const AnimatedObject* owner, int32 track)
{
    Function<void()> fn = Bind(&AnimationManager::DeleteAnimationInternal, this, owner, track);
    GetEngineContext()->jobManager->CreateMainJob(fn);
}

void AnimationManager::DeleteAnimationInternal(const AnimatedObject* owner, int32 track)
{
    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        Animation* animation = it->Get();
        if ((track != -1) && (animation->groupId != track))
        {
            continue;
        }

        if (animation->owner == owner)
        {
            animation->owner = 0; // zero owner to avoid any issues (it was a problem with DumpState, when animations was deleted before).
            animation->state &= ~Animation::STATE_IN_PROGRESS;
            animation->state &= ~Animation::STATE_FINISHED;
            animation->state |= Animation::STATE_DELETE_ME;
        }
    }
}

Animation* AnimationManager::FindLastAnimation(const AnimatedObject* _owner, int32 _groupId) const
{
    DVASSERT(Thread::IsMainThread());

    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        Animation* animation = it->Get();

        if ((animation->owner == _owner) && (animation->groupId == _groupId))
        {
            while (animation->next != 0)
            {
                animation = animation->next;
            }
            return animation; // return latest animation in current group
        }
    }
    return 0;
}

bool AnimationManager::IsAnimating(const AnimatedObject* owner, int32 track) const
{
    DVASSERT(Thread::IsMainThread());

    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        Animation* animation = it->Get();

        if ((track != -1) && (animation->groupId != track))
        {
            continue;
        }

        if ((animation->owner == owner) && (animation->state & Animation::STATE_IN_PROGRESS))
        {
            return true;
        }
    }
    return false;
}

Animation* AnimationManager::FindPlayingAnimation(const AnimatedObject* owner, int32 _groupId) const
{
    DVASSERT(Thread::IsMainThread());

    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        Animation* animation = it->Get();

        if ((_groupId != -1) && (animation->groupId != _groupId))
        {
            continue;
        }

        if ((animation->owner == owner) && (animation->state & Animation::STATE_IN_PROGRESS))
        {
            return animation;
        }
    }

    return 0;
}

bool AnimationManager::HasActiveAnimations(const AnimatedObject* owner) const
{
    DVASSERT(Thread::IsMainThread());

    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        const Animation* animation = it->Get();

        if ((animation->owner == owner) && !(animation->state & Animation::STATE_FINISHED))
        {
            return true;
        }
    }
    return false;
}

void AnimationManager::Update(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::ANIMATION_MANAGER);

    DVASSERT(Thread::IsMainThread());

    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::UPDATE_ANIMATIONS))
        return;

    // update animations first
    for (const auto& animation : animations)
    {
        if (animation->state & Animation::STATE_IN_PROGRESS)
        {
            if (!(animation->state & Animation::STATE_PAUSED))
            {
                animation->Update(timeElapsed);
            }
        }
    }

    // process all finish callbacks
    // someone could change animations list on Stop action
    // it produces crash, so we keep that implementation until
    // external code produces crashes here.
    size_t size = animations.size();
    for (size_t k = 0; k < size; ++k)
    {
        Animation* animation = animations[k].Get();

        if (animation->state & Animation::STATE_FINISHED)
        {
            animation->Stop();
        }
    }

    //check all animation and process all callbacks on delete
    for (const auto& animation : animations)
    {
        if (animation->state & Animation::STATE_DELETE_ME)
        {
            if (!(animation->state & Animation::STATE_FINISHED))
            {
                animation->OnCancel();
            }
            else
            {
                animation->state &= ~Animation::STATE_FINISHED;
            }

            if (animation->next && !(animation->next->state & Animation::STATE_DELETE_ME))
            {
                animation->next->state |= Animation::STATE_IN_PROGRESS;
                animation->next->OnStart();
            }
        }
    }

    //we need physically remove animations only after process all callbacks
    DeleteAllFinishedAnimation();
}

void AnimationManager::DeleteAllFinishedAnimation()
{
    Vector<RefPtr<Animation>> finishedAnimations;
    for (const auto& animation : animations)
    {
        if ((animation->state & Animation::STATE_DELETE_ME))
        {
            animation->state &= ~Animation::STATE_DELETE_ME;
            DVASSERT(animation->state == 0);
            animation->state |= Animation::STATE_DELETED;
            finishedAnimations.push_back(animation);
        }
    }

    auto newEndIt = std::remove_if(begin(animations), end(animations), [](const auto& animation) { return (animation->state & Animation::STATE_DELETED); });
    animations.erase(newEndIt, animations.end());
}

void AnimationManager::DumpState()
{
    DVASSERT(Thread::IsMainThread());

    Logger::FrameworkDebug("============================================================");
    Logger::FrameworkDebug("------------ Currently allocated animations - %2d ---------", registeredAnimations.size());

    for (size_t k = 0, end = registeredAnimations.size(); k < end; ++k)
    {
        Animation* animation = registeredAnimations[k];

        String ownerName = "no owner";
        if (animation->owner)
            ownerName = typeid(*animation->owner).name();
        Logger::FrameworkDebug("addr:0x%08x state:%d class: %s ownerClass: %s", animation, animation->state, typeid(*animation).name(), ownerName.c_str());
    }

    Logger::FrameworkDebug("============================================================");
}

void AnimationManager::PauseAnimations(bool isPaused, int tag)
{
    DVASSERT(Thread::IsMainThread());

    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        Animation* a = it->Get();

        if (a->GetTagId() == tag)
        {
            a->Pause(isPaused);
        }
    }
}

void AnimationManager::SetAnimationsMultiplier(float32 f, int tag)
{
    DVASSERT(Thread::IsMainThread());

    auto endIt = animations.end();
    for (auto it = animations.begin(); it != endIt; ++it)
    {
        Animation* a = it->Get();

        if (a->GetTagId() == tag)
        {
            a->SetTimeMultiplier(f);
        }
    }
}
};
