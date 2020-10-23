#ifndef __DAVAENGINE_NAMED_ANIMATIONS_LIST_H__
#define __DAVAENGINE_NAMED_ANIMATIONS_LIST_H__

namespace DAVA
{
class NamedAnimationList : public BaseObject
{
protected:
    ~NamedAnimationList()
    {
    }

public:
    struct CompoundAnimation
    {
        String name;
        Animation* animation;
    };

    std::map < String,
};
};

#endif // __DAVAENGINE_NAMED_ANIMATIONS_LIST_H__