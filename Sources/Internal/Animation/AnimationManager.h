#ifndef __DAVAENGINE_ANIMATIONMANAGER_H__
#define __DAVAENGINE_ANIMATIONMANAGER_H__

//#define ANIMATIONS_DEBUG

#include "Base/BaseTypes.h"
#include "Scene2D/GameObject.h"
#include "Animation/Animation.h"

namespace DAVA
{
/**
	\ingroup animationsystem
	\brief AnimationManager is the heart of our animation systems. It does all groundwork. 
	You do not need it at all and SDK do everything to process animations in background. 
*/
class AnimationManager final
{
public:
    ~AnimationManager();
    /**
		\brief Updates all animations in the system on current frame
		This method is called from ApplicationCore::Update function. 
		If you override this function you should call this method in your update or animations will not work correctly. 
		\param[in] timeElapsed time passed from previous frame
	 */
    void Update(float32 timeElapsed);

    /**
		\brief Dump animations state to console
	 */
    void DumpState();

    Animation* FindPlayingAnimation(const AnimatedObject* owner, int32 _groupId) const;

    void StopAnimations();
    void PauseAnimations(bool isPaused, int32 tag = 0);
    void SetAnimationsMultiplier(float32 f, int32 tag = 0);

private:
    Animation* FindLastAnimation(const AnimatedObject* owner, int32 _groupId) const;
    bool IsAnimating(const AnimatedObject* owner, int32 trackId) const;

    void AddAnimation(Animation* _animation);
    void AddAnimationInternal(Animation* animation);

    void RemoveAnimation(const Animation* _animation);
    void RemoveAnimationInternal(const Animation* animation);

    bool HasActiveAnimations(const AnimatedObject* owner) const;
    /*
	 Function remove all animations for given object from update and delete objects and their references
	 */
    void DeleteAnimations(const AnimatedObject* _owner, int32 track = -1);
    void DeleteAnimationInternal(const AnimatedObject* owner, int32 track);
    void DeleteAllFinishedAnimation();

    Vector<Animation*> registeredAnimations;
    Vector<RefPtr<Animation>> animations;

    friend class Animation;
    friend class AnimatedObject;
};
};

#endif // __DAVAENGINE_ANIMATIONMANAGER_H__