#include "Render/Highlevel/RenderHierarchy.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Frustum.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/GeometryOctTree.h"

namespace DAVA
{
void LinearRenderHierarchy::AddRenderObject(RenderObject* object)
{
    renderObjectArray.push_back(object);
    worldBBox.AddAABBox(object->GetWorldBoundingBox());
}

void LinearRenderHierarchy::RemoveRenderObject(RenderObject* renderObject)
{
    uint32 size = static_cast<uint32>(renderObjectArray.size());
    for (uint32 k = 0; k < size; ++k)
    {
        if (renderObjectArray[k] == renderObject)
        {
            renderObjectArray[k] = renderObjectArray[size - 1];
            renderObjectArray.pop_back();
            return;
        }
    }
    DVASSERT(0 && "Failed to find object");
}

void LinearRenderHierarchy::ObjectUpdated(RenderObject* renderObject)
{
}

void LinearRenderHierarchy::Clip(Camera* camera, Vector<RenderObject*>& visibilityArray, uint32 visibilityCriteria)
{
    Frustum* frustum = camera->GetFrustum();
    uint32 size = static_cast<uint32>(renderObjectArray.size());
    for (uint32 pos = 0; pos < size; ++pos)
    {
        RenderObject* node = renderObjectArray[pos];
        if ((node->GetFlags() & visibilityCriteria) != visibilityCriteria)
            continue;
        //still need to add flags for particles to dicede if to use DefferedUpdate
        if ((RenderObject::ALWAYS_CLIPPING_VISIBLE & node->GetFlags()) || frustum->IsInside(node->GetWorldBoundingBox()))
            visibilityArray.push_back(node);
    }
}

void LinearRenderHierarchy::GetAllObjectsInBBox(const AABBox3& bbox, Vector<RenderObject*>& visibilityArray)
{
    uint32 size = static_cast<uint32>(renderObjectArray.size());
    for (uint32 pos = 0; pos < size; ++pos)
    {
        RenderObject* ro = renderObjectArray[pos];
        if (bbox.IntersectsWithBox(ro->GetWorldBoundingBox()))
        {
            visibilityArray.push_back(ro);
        }
    }
}

bool LinearRenderHierarchy::RayTrace(const Ray3& ray, RayTraceCollision& collision, const Vector<RenderObject*>& ignoreObjects)
{
    broadPhaseCollisions.clear();

    uint32 size = static_cast<uint32>(renderObjectArray.size());
    for (uint32 pos = 0; pos < size; ++pos)
    {
        RenderObject* ro = renderObjectArray[pos];

        const AABBox3& worldBBox = ro->GetWorldBoundingBox();
        float32 tMin, tMax;
        if (Intersection::RayBox(ray, worldBBox, tMin, tMax))
        {
            auto lambda = [](const BroadPhaseCollision& pair, float val) -> bool { return pair.first < val; };
            auto it = std::lower_bound(broadPhaseCollisions.begin(), broadPhaseCollisions.end(), tMin, lambda);
            broadPhaseCollisions.insert(it, { tMin, ro });
        }
    }

    bool intersectionFound = false;
    float32 closestT = 1.0f;

    for (auto& pair : broadPhaseCollisions)
    {
        RenderObject* ro = pair.second;

        if (pair.first > closestT)
            break;

        Vector3 rayOrigin = ray.origin * ro->GetInverseWorldTransform();
        Vector3 rayDirection = MultiplyVectorMat3x3(ray.direction, ro->GetInverseWorldTransform());
        Ray3Optimized rayInObjectSpace(rayOrigin, rayDirection);

        uint32 activeBatchesCount = ro->GetActiveRenderBatchCount();
        for (uint32 bi = 0; bi < activeBatchesCount; ++bi)
        {
            RenderBatch* rb = ro->GetActiveRenderBatch(bi);
            DVASSERT(rb != nullptr);
            PolygonGroup* geo = rb->GetPolygonGroup();

            if (geo)
            {
                GeometryOctTree* geometryOctTree = geo->octTree;
                if (geometryOctTree)
                {
                    float32 currentT;
                    uint32 currentTriangleIndex;

                    if (geometryOctTree->IntersectionWithRay(rayInObjectSpace, currentT, currentTriangleIndex))
                    {
                        if (currentT < closestT)
                        {
                            intersectionFound = true;
                            closestT = currentT;

                            collision.renderObject = ro;
                            collision.geometry = geo;
                            collision.t = currentT;
                            collision.triangleIndex = currentTriangleIndex;
                        }
                    }
                }
            }
        }

        if (ro->GetType() == RenderObject::TYPE_LANDSCAPE)
        {
            Landscape* landscape = static_cast<Landscape*>(ro);
            float32 currentT;
            if (landscape->RayTrace(rayInObjectSpace, currentT))
            {
                if (currentT < closestT)
                {
                    intersectionFound = true;
                    closestT = currentT;

                    collision.renderObject = ro;
                    collision.geometry = 0;
                    collision.t = currentT;
                    collision.triangleIndex = 0;
                }
            }
        }
    }

    return intersectionFound;
}
};
