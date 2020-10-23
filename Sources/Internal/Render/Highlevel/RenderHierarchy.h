#pragma once

#include "Base/BaseTypes.h"
#include "Render/UniqueStateSet.h"
#include "Base/BaseMath.h"

namespace DAVA
{
class RenderObject;
class Camera;
class PolygonGroup;
class RenderHelper;

using BroadPhaseCollision = std::pair<float32, RenderObject*>;

class RayTraceCollision
{
public:
    RenderObject* renderObject = nullptr;
    PolygonGroup* geometry = nullptr;
    float32 t = 0.0f;
    uint32 triangleIndex = -1;
};

class RenderHierarchy
{
public:
    virtual ~RenderHierarchy()
    {
    }

    virtual void AddRenderObject(RenderObject* renderObject) = 0;
    virtual void RemoveRenderObject(RenderObject* renderObject) = 0;
    virtual void ObjectUpdated(RenderObject* renderObject) = 0;
    virtual void Clip(Camera* camera, Vector<RenderObject*>& visibilityArray, uint32 visibilityCriteria) = 0;

    virtual void GetAllObjectsInBBox(const AABBox3& bbox, Vector<RenderObject*>& visibilityArray) = 0;
    virtual bool RayTrace(const Ray3& ray, RayTraceCollision& collision,
                          const Vector<RenderObject*>& ignoreObjects) = 0;

    virtual void Initialize()
    {
    }
    virtual void PrepareForShutdown()
    {
    }
    virtual void Update()
    {
    }
    virtual void DebugDraw(const Matrix4& cameraMatrix, RenderHelper* renderHelper)
    {
    }
    virtual const AABBox3& GetWorldBoundingBox() const = 0;
};

class LinearRenderHierarchy : public RenderHierarchy
{
    void AddRenderObject(RenderObject* renderObject) override;
    void RemoveRenderObject(RenderObject* renderObject) override;
    void ObjectUpdated(RenderObject* renderObject) override;
    void Clip(Camera* camera, Vector<RenderObject*>& visibilityArray, uint32 visibilityCriteria) override;
    void GetAllObjectsInBBox(const AABBox3& bbox, Vector<RenderObject*>& visibilityArray) override;
    bool RayTrace(const Ray3& ray, RayTraceCollision& collision,
                  const Vector<RenderObject*>& ignoreObjects) override;
    const AABBox3& GetWorldBoundingBox() const override;

private:
    Vector<RenderObject*> renderObjectArray;
    Vector<BroadPhaseCollision> broadPhaseCollisions;
    AABBox3 worldBBox = AABBox3();
};

inline const AABBox3& LinearRenderHierarchy::GetWorldBoundingBox() const
{
    return worldBBox;
}

} // ns
