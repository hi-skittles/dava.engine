#ifndef __HOOD_COLLISION_OBJECT_H__
#define __HOOD_COLLISION_OBJECT_H__

#include "Scene/SceneTypes.h"

// bullet
#include "bullet/btBulletCollisionCommon.h"

// framework
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"

struct HoodCollObject
{
    HoodCollObject();
    ~HoodCollObject();

    btCollisionObject* btObject;
    btCollisionShape* btShape;

    DAVA::Vector3 baseFrom;
    DAVA::Vector3 baseTo;
    DAVA::Vector3 baseOffset;
    DAVA::Matrix4 baseRotate;
    DAVA::Vector3 scaledOffset;

    DAVA::Vector3 curFrom;
    DAVA::Vector3 curTo;

    DAVA::Vector3 curPos;
    DAVA::float32 curScale;

    ST_Axis axis;

    void UpdatePos(const DAVA::Vector3& pos);
    void UpdateScale(const DAVA::float32& scale);
};

#endif // __HOOD_COLLISION_OBJECT_H__
