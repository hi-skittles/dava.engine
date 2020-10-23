#pragma once

#include "Base/Vector.h"
#include "Base/FastName.h"
#include "Base/Hash.h"
#include "Base/UnordererSet.h"

namespace DAVA
{
class Entity;
class MotionComponent;
class MotionSingleComponent
{
public:
    struct AnimationInfo
    {
        AnimationInfo(MotionComponent* component, const FastName& layer, const FastName& motion, const FastName& marker = FastName())
            : motionComponent(component)
            , layerID(layer)
            , motionID(motion)
            , markerID(marker)
        {
        }

        MotionComponent* motionComponent = nullptr;
        FastName layerID;
        FastName motionID;
        FastName markerID;

        inline bool operator==(const AnimationInfo& info) const
        {
            return motionComponent == info.motionComponent && layerID == info.layerID && motionID == info.motionID && markerID == info.markerID;
        }
    };

    struct AnimationInfoHash
    {
        std::size_t operator()(const AnimationInfo& info) const
        {
            std::size_t seed = 0;
            HashCombine(seed, info.motionComponent);
            HashCombine(seed, info.layerID);
            HashCombine(seed, info.motionID);
            HashCombine(seed, info.markerID);
            return seed;
        }
    };

    Vector<MotionComponent*> startSimpleMotion;
    Vector<MotionComponent*> stopSimpleMotion;
    Vector<MotionComponent*> simpleMotionFinished;

    Vector<MotionComponent*> rebindSkeleton;
    Vector<MotionComponent*> reloadDescriptor;

    UnorderedSet<AnimationInfo, AnimationInfoHash> animationEnd;
    UnorderedSet<AnimationInfo, AnimationInfoHash> animationMarkerReached;

    void Clear();
    void EntityRemoved(const Entity* entity);
};
}
