#ifndef __DAVAENGINE_UI_HOLE_TRANSITION_H__
#define __DAVAENGINE_UI_HOLE_TRANSITION_H__

#include "Base/BaseTypes.h"
#include "UI/UIScreenTransition.h"
#include "Math/Polygon2.h"

namespace DAVA
{
class UIHoleTransition : public UIScreenTransition
{
public:
    UIHoleTransition();
    virtual ~UIHoleTransition();

    virtual void SetPolygon(const Polygon2& pts);
    virtual void Update(float32 timeElapsed);
    virtual void Draw(const UIGeometricData& geometricData);

private:
    Polygon2 clipPoly;
    Polygon2 realPoly;
};
};



#endif // __DAVAENGINE_UI_MOVEIN_TRANSITION_H__