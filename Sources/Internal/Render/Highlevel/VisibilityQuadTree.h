#pragma once

#include "Base/BaseObject.h"
#include "Math/AABBox3.h"
#include "Render/Highlevel/RenderHierarchy.h"
#include "Render/UniqueStateSet.h"

#define DAVA_DEBUG_DRAW_OCTREE 0

namespace DAVA
{
class Frustum;
class RenderObject;
class QuadTree : public RenderHierarchy
{
public:
    enum : uint16
    {
        INVALID_TREE_NODE_INDEX = static_cast<uint16>(-1)
    };

public:
    QuadTree(int32 maxTreeDepth);

    void AddRenderObject(RenderObject* renderObject) override;
    void RemoveRenderObject(RenderObject* renderObject) override;
    void ObjectUpdated(RenderObject* renderObject) override;
    void Clip(Camera* camera, Vector<RenderObject*>& visibilityArray, uint32 visibilityCriteria) override;
    void GetAllObjectsInBBox(const AABBox3& bbox, Vector<RenderObject*>& visibilityArray) override;
    bool RayTrace(const Ray3& ray, RayTraceCollision& collision,
                  const Vector<RenderObject*>& ignoreObjects) override;
    const AABBox3& GetWorldBoundingBox() const override;

    void Initialize() override;
    void PrepareForShutdown() override;

    void Update() override;
    void DebugDraw(const Matrix4& cameraMatrix, RenderHelper* renderHelper) override;

    struct QuadTreeNode // still basic implementation - later move it to more compact
    {
        enum eNodeType
        {
            NODE_LB = 0,
            NODE_RB = 1,
            NODE_LT = 2,
            NODE_RT = 3,
            NODE_NONE = 4
        };
        uint16 parent;
        uint16 children[4]; // think about allocating and freeing at groups of for
        AABBox3 bbox;

        const static uint16 NUM_CHILD_NODES_MASK = 0x07;
        const static uint16 DIRTY_Z_MASK = 0x08;
        const static uint16 NODE_DEPTH_MASK = 0xFF00;
        const static uint16 NODE_DEPTH_OFFSET = 8;
        const static uint16 START_CLIP_PLANE_MASK = 0xF0;
        const static uint16 START_CLIP_PLANE_OFFSET = 4;
        uint16 nodeInfo; // format : ddddddddddzccñ where c - numChildNodes, z - dirtyZ, d - depth
        Vector<RenderObject*> objects;
        QuadTreeNode();
        void Reset();
    };

private:
    bool CheckObjectFitNode(const AABBox3& objBox, const AABBox3& nodeBox);
    bool CheckBoxIntersectBranch(const AABBox3& objBox, float32 xmin, float32 ymin, float32 xmax, float32 ymax);
    bool CheckBoxIntersectChild(const AABBox3& objBox, const AABBox3& nodeBox, QuadTreeNode::eNodeType nodeType); //assuming it already fit parent!
    void UpdateChildBox(AABBox3& parentBox, QuadTreeNode::eNodeType childType);
    void UpdateParentBox(AABBox3& childBox, QuadTreeNode::eNodeType childType);

    void ProcessNodeClipping(uint16 nodeId, uint8 clippingFlags, Vector<RenderObject*>& visibilityArray);
    void GetObjects(uint16 nodeId, const AABBox3& bbox, Vector<RenderObject*>& visibilityArray);
    void RecalculateNodeZLimits(uint16 nodeId);
    void MarkNodeDirty(uint16 nodeId);
    void MarkObjectDirty(RenderObject* object);
    void DebugDrawNode(uint16 nodeId);
    void BroadPhaseCollisions(const Ray3& rayInWorldSpace, Vector<BroadPhaseCollision>& broadPhaseCollisions);

    uint16 FindObjectAddNode(uint16 startNodeId, const AABBox3& objBox);

private:
    static const int32 RECALCULATE_Z_PER_FRAME = 10;
    static const int32 RECALCULATE_OBJECTS_PER_FRAME = 10;

    Vector<BroadPhaseCollision> broadPhaseCollisions;
    Vector<QuadTreeNode> nodes;
    Vector<uint32> emptyNodes;
    List<int32> dirtyZNodes;
    List<RenderObject*> dirtyObjects;
    List<RenderObject*> worldInitObjects;
    std::queue<uint16> broadPhaseQueue;

#if (DAVA_DEBUG_DRAW_OCTREE)
    UniqueHandle debugDrawStateHandle = InvalidUniqueHandle;
#endif

    AABBox3 worldBox;
    int32 maxTreeDepth = 0;
    Frustum* currFrustum = nullptr;
    Camera* currCamera = nullptr;
    uint32 currVisibilityCriteria = 0;
    uint32 localRayBoxTraceCount = 0;
    bool worldInitialized = false;
    bool preparedForShutdown = false;
};
}
