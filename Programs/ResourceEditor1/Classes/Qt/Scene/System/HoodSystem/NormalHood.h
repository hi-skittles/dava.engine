#ifndef __NORMAL_HOOD_H__
#define __NORMAL_HOOD_H__

#include "Scene/System/HoodSystem/HoodObject.h"

struct NormalHood : public HoodObject
{
    NormalHood();
    ~NormalHood();

    virtual void Draw(ST_Axis selectedAxis, ST_Axis mouseOverAxis, DAVA::RenderHelper* drawer, TextDrawSystem* textDrawSystem);

    HoodCollObject* axisX;
    HoodCollObject* axisY;
    HoodCollObject* axisZ;
};

#endif // __NORMAL_HOOD_H__
