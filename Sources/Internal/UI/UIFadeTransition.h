#ifndef __DAVAENGINE_UI_FADE_TRANSITION_H__
#define __DAVAENGINE_UI_FADE_TRANSITION_H__

#include "Base/BaseTypes.h"
#include "UI/UIScreenTransition.h"

namespace DAVA
{
class UIFadeTransition : public UIScreenTransition
{
public:
    UIFadeTransition();
    virtual ~UIFadeTransition();

    enum eType
    {
        FADE_MIX = 0,
        FADE_IN_FADE_OUT,
    };

    virtual void SetType(eType _type);
    virtual void Update(float32 timeElapsed);
    virtual void Draw(const UIGeometricData& geometricData);

private:
    eType type;
};
};



#endif // __DAVAENGINE_UI_MOVEIN_TRANSITION_H__