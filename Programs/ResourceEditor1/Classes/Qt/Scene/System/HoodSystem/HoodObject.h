#ifndef __HOOD_OBJECT_H__
#define __HOOD_OBJECT_H__

#include "Scene/System/HoodSystem/HoodCollObject.h"
#include "Scene/SceneTypes.h"
#include "Render/RenderHelper.h"

#include "Render/UniqueStateSet.h"

class TextDrawSystem;

struct HoodObject
{
    HoodObject(DAVA::float32 baseSize);
    virtual ~HoodObject();

    DAVA::float32 baseSize;
    DAVA::float32 objScale;
    DAVA::Color colorX; // axis X
    DAVA::Color colorY; // axis X
    DAVA::Color colorZ; // axis X
    DAVA::Color colorS; // axis selected

    virtual void UpdatePos(const DAVA::Vector3& pos);
    virtual void UpdateScale(const DAVA::float32& scale);
    virtual void Draw(ST_Axis selectedAxis, ST_Axis mouseOverAxis, DAVA::RenderHelper* drawer, TextDrawSystem* textDrawSystem) = 0;

    HoodCollObject* CreateLine(const DAVA::Vector3& from, const DAVA::Vector3& to);
    DAVA::Rect DrawAxisText(TextDrawSystem* textDrawSystem, HoodCollObject* x, HoodCollObject* y, HoodCollObject* z);

    DAVA::Vector<HoodCollObject*> collObjects;

protected:
    DAVA::Vector3 GetAxisTextPos(HoodCollObject* axis);
};

#endif
