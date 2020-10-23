#ifndef __DAVAENGINE_UI_MOVEIN_TRANSITION_H__
#define __DAVAENGINE_UI_MOVEIN_TRANSITION_H__

#include "Base/BaseTypes.h"
#include "UI/UIScreenTransition.h"

namespace DAVA
{
class UIMoveInTransition : public UIScreenTransition
{
public:
    UIMoveInTransition();
    virtual ~UIMoveInTransition();

    enum eType
    {
        FROM_LEFT = 0,
        FROM_RIGHT,
        FROM_TOP,
        FROM_BOTTOM,
        TO_RIGHT,
        TO_LEFT,
        TO_BOTTOM,
        TO_TOP,
    };

    virtual void SetType(eType _type, bool moveOver = false);
    virtual void Update(float32 timeElapsed);
    virtual void Draw(const UIGeometricData& geometricData);

private:
    eType type;
    bool isOver;
};
};



#endif // __DAVAENGINE_UI_MOVEIN_TRANSITION_H__