#include "Collision/CollisionObject2.h"
#include "Collision/Collisions.h"
#include "Render/RenderHelper.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/Renderer.h"

#include "Engine/Engine.h"

namespace DAVA
{
CollisionObject2::CollisionObject2(eType _type)
{
    type = _type;
    updateFrameIndex = 0;
    basePolygon = NULL;
}

CollisionObject2::~CollisionObject2()
{
}

void CollisionObject2::SetType(eType _type)
{
    type = _type;
}

void CollisionObject2::SetPolygon(Polygon2* _basePolygon)
{
    DVASSERT(_basePolygon != 0);
    //TODO: check Polygon life time (retain/release)
    basePolygon = _basePolygon;
    // TODO: Fix this because it can cause a problems when setting frame during draw
    // when you set the frame it reset polygon to it's original shape what's wrong
    polygon = *_basePolygon;
    basePolygon->CalculateCenterPoint(basePolygonCenter);
    circle.center = basePolygonCenter;
    circle.radius = std::sqrt(basePolygon->CalculateSquareRadius(basePolygonCenter));

    forceUpdate = true;
}
    

#if 0
void CollisionObject2::Update(const SpriteDrawState & state/*const Vector2 & _position, const Vector2 & _pivot, const Vector2 & _scale, float32 _angle*/)
{
    if (!basePolygon)return;
    uint32 globalFrameIndex = Core::Instance()->GetGlobalFrameIndex();
    if (globalFrameIndex == updateFrameIndex)return;
    updateFrameIndex = globalFrameIndex;
    
    position = state.position;
    pivot = state.pivotPoint;
    scale = state.scale;
    angle = state.angle;
    
    bbox.Empty();
    
    if (type == TYPE_POLYGON)
    {
        float32 sinA = std::sin(angle);
        float32 cosA = std::cos(angle);
        for (int k = 0; k < basePolygon->pointCount; ++k)
        {
            Vector2 * v = &polygon.points[k];
            *v = basePolygon->points[k] - pivot;
            v->x *= scale.x;
            v->y *= scale.y;
            float32 nx = (v->x) * cosA  - (v->y) * sinA + position.x;
            float32 ny = (v->x) * sinA  + (v->y) * cosA + position.y;
            v->x = nx;
            v->y = ny;
            bbox.AddPoint(*v);
        }
    }
    circle.center = basePolygonCenter;
    circle.center -= pivot;
    circle.center += position;
}
#endif

void CollisionObject2::UpdatePosition(Vector2 newPos)
{
    Vector2 diff = newPos - position;
    position = newPos;

    circle.center += diff;

    if (type != TYPE_POLYGON)
        return;

    for (int k = 0; k < polygon.pointCount; ++k)
    {
        Vector2& v = polygon.points[k];
        v += diff;
    }

    bbox.min += diff;
    bbox.max += diff;
}

void CollisionObject2::Update(const SpriteDrawState& state /*const Vector2 & _position, const Vector2 & _pivot, const Vector2 & _scale, float32 _angle*/)
{
    if (!basePolygon)
        return;

    uint32 globalFrameIndex = Engine::Instance()->GetGlobalFrameIndex();
    if (globalFrameIndex == updateFrameIndex)
        return;
    updateFrameIndex = globalFrameIndex;

    if (!forceUpdate)
    {
        if ((scale == state.scale) && (angle == state.angle))
        {
            if ((position == state.position) && (pivot == state.pivotPoint))
                return;

            if (state.pivotPoint == pivot)
            {
                UpdatePosition(state.position);
                return;
            }
        }
    }

    position = state.position;
    pivot = state.pivotPoint;

    scale = state.scale;
    angle = state.angle;

    forceUpdate = false;

    // TODO do not recalc if angle and scale did not change

    bbox.Empty();

    float32 sinA = std::sin(angle);
    float32 cosA = std::cos(angle);

    if (type == TYPE_POLYGON)
    {
        for (int k = 0; k < basePolygon->pointCount; ++k)
        {
            Vector2* v = &polygon.points[k];
            *v = basePolygon->points[k] - pivot;
            v->x *= scale.x;
            v->y *= scale.y;
            float32 nx = (v->x) * cosA - (v->y) * sinA + position.x;
            float32 ny = (v->x) * sinA + (v->y) * cosA + position.y;
            v->x = nx;
            v->y = ny;
            bbox.AddPoint(*v);
        }

        Vector2 c;
        polygon.CalculateCenterPoint(c);
        circle.radius = std::sqrt(polygon.CalculateSquareRadius(c));
        circle.center = c;
    }
    else
    {
        circle.center = basePolygonCenter;
        circle.center -= pivot;
        circle.center += position;
    }
}

void CollisionObject2::DebugDraw()
{
    if (!basePolygon)
        return;
    Color red = Color(1.0f, 0.0f, 0.0f, 1.0f);

    RenderSystem2D::Instance()->DrawCircle(circle.center, 2.f, red);
    RenderSystem2D::Instance()->DrawCircle(circle.center, circle.radius, red);

    if (type == TYPE_POLYGON)
    {
        RenderSystem2D::Instance()->DrawPolygon(polygon, true, red);
    }

    Color blue = Color(0.0f, 0.0f, 1.0f, 1.0f);
    for (int32 k = 0; k < manifold.count; ++k)
        RenderSystem2D::Instance()->DrawCircle(manifold.contactPoints[k], 3.f, blue);
}

bool CollisionObject2::IsCollideWith(CollisionObject2* collObject)
{
    // null contact manifold point counts
    this->manifold.count = 0;
    collObject->manifold.count = 0;

    float32 cx = circle.center.x;
    float32 cy = circle.center.y;

    float32 ocx = collObject->circle.center.x;
    float32 ocy = collObject->circle.center.y;

    // no square radius here
    float32 radii = this->circle.radius + collObject->circle.radius;
    if ((cx - ocx) * (cx - ocx) + (cy - ocy) * (cy - ocy) > radii * radii)
    {
        return false;
    }

    if ((type == TYPE_CIRCLE) && (collObject->type == TYPE_CIRCLE))
    {
        Collisions::Instance()->FindIntersectionCircleToCircle(this->circle, collObject->circle, this->manifold);
        collObject->manifold = this->manifold;
        return true;
    }
    else if (((type == TYPE_CIRCLE) && (collObject->type == TYPE_POLYGON))
             || ((type == TYPE_POLYGON) && (collObject->type == TYPE_CIRCLE)))
    {
        //
        // DVASSERT(0 && "Implement SAT code to find intersections between circle & polygon");

        Circle& checkCircle = circle;
        if (collObject->type == TYPE_CIRCLE)
            circle = collObject->circle;

        Polygon2& checkPoly = polygon;
        if (collObject->type == TYPE_POLYGON)
            checkPoly = collObject->polygon;

        Collisions::Instance()->FindIntersectionPolygonToCircle(checkPoly, checkCircle, this->manifold);
        collObject->manifold = this->manifold;

        return (this->manifold.count != 0);
    }
    else if ((type == TYPE_POLYGON) && (collObject->type == TYPE_POLYGON))
    {
        bool inters = Collisions::Instance()->IsPolygonIntersectsPolygon(this->polygon, collObject->polygon);
        Collisions::Instance()->FindIntersectionPolygonToPolygon(this->polygon, collObject->polygon, this->manifold);
        collObject->manifold = this->manifold;
        return inters;
    }
    return false;
}

ContactManifold2* CollisionObject2::GetContactManifold()
{
    return &manifold;
}
}

/*CollisionPrimitive * CollisionPrimitive::CreateFromFile(FILE*file, float cx, float cy)
{
	CollisionPrimitive *cp = new CollisionPrimitive();
	if (cp)
	{
		cp->collisionRadius = 0;

		fscanf(file, "%d", &cp->pointsCount);
		
		cp->points = new float[cp->pointsCount * 2];
		for(int i = 0; i < cp->pointsCount * 2; i += 2)
		{
			fscanf(file, "%f %f", &cp->points[i], &cp->points[i + 1]);
		}
		
		float sumX = 0.0f; 
		float sumY = 0.0f;
		for(int i = 0; i < cp->pointsCount * 2; i += 2)
		{
			sumX += cp->points[i];
			sumY += cp->points[i + 1];
			cp->bbox.AddPoint(DAVA::Vector3(cp->points[i], cp->points[i + 1]));
		}
		sumX /= cp->pointsCount;
		sumY /= cp->pointsCount;
		
		cx = sumX;
		cy = sumY;
		
		for(int i = 0; i < cp->pointsCount * 2; i += 2)
		{
			int m = (cp->points[i] - cx) * (cp->points[i] - cx) + (cp->points[i + 1] - cy) * (cp->points[i + 1] - cy);
			if(m > cp->collisionRadius)
			{
				cp->collisionRadius = m;
			}
		}
		
		cp->collisionVertices = new float[cp->pointsCount * 2];
		cp->centerX = cx;
		cp->centerY = cy;
	}
	return cp;
}


- (id) copy
{
	CollisionPrimitive * copy = [[CollisionPrimitive alloc] init];
	copy->collisionRadius = self->collisionRadius;
	copy->pointsCount = self->pointsCount;
	
	copy->points = new float[copy->pointsCount * 2];
	for (int i = 0; i < self->pointsCount * 2; i ++)
	{
		copy->points[i] = self->points[i];
	}
	copy->collisionVertices = new float[copy->pointsCount * 2];
	copy->collisionRadius = self->collisionRadius;
	copy->centerX = self->centerX;
	copy->centerY = self->centerY;
	
	return copy;
}*/

/*CollisionPrimitive::~CollisionPrimitive()
{
	SafeDeleteArray(collisionVertices);
	SafeDeleteArray(points);
}


bool CollisionPrimitive::IsPointsInsidePoints(float* points1, int cnt1, float*points2, int cnt2)
{
	for(int pnt = 0; pnt < cnt1 * 2; pnt += 2)
	{
		int count = 0;
		float pntY = points1[pnt + 1];
		for(int i = 0; i < cnt2 * 2; i += 2)
		{
			int j = ((i >> 1) + 1) % (cnt2);
			j *= 2;
			float iY = points2[i + 1];
			float jY = points2[j + 1];
			
			
			if( iY == jY)
			{
				continue;
			}
			if(iY > pntY && jY > pntY)
			{
				continue;
			}
			if(iY < pntY && jY < pntY)
			{
				continue;
			}
			if(Max(iY, jY) == pntY)
			{
				count++;
			}
			else if(Min(iY, jY) == pntY)
			{
				continue;
			}
			else
			{
				float fpntY = points1[pnt + 1];
				float fiY = points2[i + 1];
				float fjY = points2[j + 1];
				float t = (float)(fpntY - fiY) / (float)(fjY - fiY);
				if(points2[i] + t * (points2[j] - points2[i]) >= points1[pnt])
				{
					count++;
				}
			}
		}
		if(count & 1)
		{
			collX = points1[pnt];
			collY = pntY;
			return true;
		}
	}
	
	return false;
}

void  CollisionPrimitive::CalcCollisionPoints()
{
	int ctX = centerX;//sprite->centerX;
	int ctY = centerY;//sprite->centerY;

	float rad = angle;
	float cosA = cos(rad);
	float sinA = sin(rad);
	
	for(int i = 0; i < pointsCount * 2; i += 2)
	{
		collisionVertices[i] = (points[i] - ctX) * cosA  - (points[i + 1] - ctY) * sinA + xCoord + ctX;
		collisionVertices[i + 1] = (points[i] - ctX) * sinA  + (points[i + 1] - ctY) * cosA + yCoord + ctY;
	}
}

bool  CollisionPrimitive::IsCollideWith(CollisionPrimitive* collObject)
{
	// if we check collision with null object always return no
	if (collObject == 0)return false;
	
	int cx = xCoord + centerX;
	int cy = yCoord + centerY;
	
	int ocx = collObject->xCoord + collObject->centerX;
	int ocy = collObject->yCoord + collObject->centerY;
	if ( (cx - ocx) * (cx - ocx) + (cy - ocy) * (cy - ocy) 
		> this->collisionRadius + collObject->collisionRadius)
	{
		return false;
	}
	
	CalcCollisionPoints();
	collObject->CalcCollisionPoints();
	
	if(IsPointsInsidePoints(collisionVertices, pointsCount, collObject->collisionVertices, collObject->pointsCount))
	{
		return true;
	}
	return IsPointsInsidePoints(collObject->collisionVertices, collObject->pointsCount, collisionVertices, pointsCount);
} */

/*void DrawCollision();
{
	 glDisable(GL_BLEND);
	 glDisable(GL_TEXTURE_2D);
	 
	 glColor4f(0.0, 1.0, 0.0, 1.0);
	 
	 glVertexPointer(2, GL_FLOAT, 0, collisionVertices);
	 glEnableClientState(GL_VERTEX_ARRAY);
	 
	 
	 glDrawArrays(GL_LINE_LOOP, 0, pointsCount);
	 
	 //	glColor4f(1.0, 1.0, 0.0, 1.0);
	 //	int radius = sqrt(spriteDef->collision[(int)drawFrame]->collisionRadius);
	 //	float vert[32 * 2];
	 //	int i = 0;
	 //	for(float rad = 0; rad < 3.14*2.0; rad += 3.14*2.0 / 32.0)
	 //	{
	 //		vert[i] = radius * cos(rad) + xCoord + spriteDef->centerX;
	 //		vert[i + 1] = radius * sin(rad) + yCoord + spriteDef->centerY;
	 //		i+=2;
	 //	}
	 //	glVertexPointer(2, GL_FLOAT, 0, vert);
	 //	glEnableClientState(GL_VERTEX_ARRAY);
	 //	
	 //	
	 //	glDrawArrays(GL_LINE_LOOP, 0, 32);
	 
	glVertexPointer(2, GL_FLOAT, 0, &centerX);
	glDrawArrays(GL_POINTS, 0, 1);
	
	
	
	 glEnable(GL_BLEND);
	 glEnable(GL_TEXTURE_2D);
	 glColor4f(1.0, 1.0, 1.0, 1.0);
}
*/
