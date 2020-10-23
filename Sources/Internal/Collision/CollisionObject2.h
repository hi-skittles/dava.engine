#ifndef __DAVAENGINE_COLLISION_OBJECT2_H__
#define __DAVAENGINE_COLLISION_OBJECT2_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Render/2D/Sprite.h"
#include "Collision/Collisions.h"

namespace DAVA
{
class CollisionObject2 : public BaseObject
{
protected:
    virtual ~CollisionObject2();

public:
    enum eType
    {
        TYPE_CIRCLE,
        TYPE_POLYGON,
    };

    CollisionObject2(eType type);

    /*
		Collision object do not hold polygon inside his body
		So you should give him polygon that will be alive till the end of live of collision object
	 */
    void SetType(eType type);

    /// NOTE! Polygon2 pointer must be valid as long as CollisionObject2 lives!
    void SetPolygon(Polygon2* p);

    void Update(const SpriteDrawState& state);
    void DebugDraw();

    bool IsCollideWith(CollisionObject2* collObject);
    ContactManifold2* GetContactManifold();

    eType type;

    Vector2 position; //	Position of the collision object pivot
    Vector2 pivot; //	Shift of the object inside the sprite
    Vector2 scale; //  Current scale
    float32 angle; //  Current angle

    Circle circle; // circle in case if it's circle collision object
    //float32 basePolygonRadius;	//	Square radius of the rotated object
    Vector2 basePolygonCenter; // real center of the collision polygon

    //Vector2 updatedCenter;		// real center of the collision polygon
    ContactManifold2 manifold;

    Polygon2* basePolygon;
    Polygon2 polygon;
    AABBox2 bbox;

    uint32 updateFrameIndex;
    bool collisionOnLastFrame;
    bool forceUpdate;

    void UpdatePosition(Vector2 newPos);
};
};

#endif
