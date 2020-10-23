#ifndef __VISIBILITY_OCT_TREE_H__
#define __VISIBILITY_OCT_TREE_H__

#include "Render/Highlevel/RenderHierarchy.h"
#include "Base/DynamicBitset.h"

namespace DAVA
{
class Frustum;

class VisibilityOctTree : public RenderHierarchy
{
public:
    VisibilityOctTree();
    ~VisibilityOctTree();

    void AddRenderObject(RenderObject* renderObject) override;
    void RemoveRenderObject(RenderObject* renderObject) override;
    void ObjectUpdated(RenderObject* renderObject) override;
    void Clip(Camera* camera, Vector<RenderObject*>& visibilityArray, uint32 visibilityCriteria) override;
    void GetAllObjectsInBBox(const AABBox3& bbox, Vector<RenderObject*>& visibilityArray) override;
    bool RayTrace(const Ray3& ray, RayTraceCollision& collision,
                  const Vector<RenderObject*>& ignoreObjects) override;
    void Initialize() override;
    void Update() override;
    void DebugDraw(const Matrix4& cameraMatrix, RenderHelper* renderHelper) override;
    const AABBox3& GetWorldBoundingBox() const override;

private:
    static const uint32 EMPTY_LEAF = 0xFFFF;

    struct VisibilityOctTreeNode
    {
        struct
        {
            uint64 objectsInTheNode : 16;
            uint64 leafIndex : 16;
            uint64 children : 8;
            uint64 startClippingPlane : 3;
            bool debugBlue : 1;
            uint64 reserved : 20;
        };
    };

    union VoxelCoord
    {
        struct
        {
            uint32 level : 5;
            uint32 x : 9;
            uint32 y : 9;
            uint32 z : 9;
        };
        uint32 packedCoord;
    };

    struct SubdivisionLevelInfo
    {
        uint32 offset;
        uint32 size;
    };

    struct Stats
    {
        uint32 objectsCount;
    };

    void DetermineBoxBoundsInVoxels(RenderObject* renderObject, uint32& level, uint32* rmin, uint32* rmax);

    inline uint32 GetNodeIndexByCoord(VoxelCoord voxelCoord) const;
    inline uint32 GetNodeIndexByLXYZ(uint32 level, uint32 x, uint32 y, uint32 z) const;
    inline AABBox3 ReconstructAABBox(VoxelCoord voxelCoord) const;

    void InternalAddRenderObject(RenderObject* renderObject, VoxelCoord voxelCoord);
    void InternalRemoveRenderObject(RenderObject* renderObject, VoxelCoord voxelCoord);
    void InternalClip(VoxelCoord voxelCoord, const AABBox3& nodeBox, uint8 clipMask, Vector<RenderObject*>& visibilityArray);

    void CollectAndPrintStats();

    void InternalDebugDraw(VoxelCoord voxelCoord, const Matrix4& worldMatrix, const AABBox3& bbox, RenderHelper* renderHelper);

    uint32 AllocateLeaf();
    void DeallocateLeaf(uint32 leafIndex);

    Camera* camera = nullptr;
    uint32 visibilityCriteria = 0;
    Frustum* frustum = nullptr;

    AABBox3 worldBBox;
    uint32 maxSubdivisionLevel = 6;
    uint32 oneAxisMaxPosition = 1 << (maxSubdivisionLevel - 1);
    uint32 nodeMaxCount = 0;

    std::vector<SubdivisionLevelInfo> subdivLevelInfoArray;
    std::vector<VisibilityOctTreeNode> nodeArray;
    std::vector<Stats> stats;

    std::vector<std::vector<RenderObject*>> leafs;
    std::vector<uint32> freeLeafs;

    std::vector<RenderObject*> roIndices;
    DynamicBitset visibleObjects;

    // RayTrace related stuff
    void BroadPhaseCollisions(const Ray3& rayInWorldSpace, Vector<BroadPhaseCollision>& broadPhaseCollisions);

    Vector<BroadPhaseCollision> broadPhaseCollisions;
    std::queue<VoxelCoord> broadPhaseQueue;
    uint32 localRayBoxTraceCount = 0;
};

inline AABBox3 VisibilityOctTree::ReconstructAABBox(VoxelCoord voxelCoord) const
{
    Vector3 voxelSizeOnLevel = worldBBox.GetSize() / (static_cast<float32>(1 << voxelCoord.level));
    float32 x = static_cast<float32>(voxelCoord.x);
    float32 y = static_cast<float32>(voxelCoord.y);
    float32 z = static_cast<float32>(voxelCoord.z);

    Vector3 min(worldBBox.min.x + x * voxelSizeOnLevel.x,
                worldBBox.min.y + y * voxelSizeOnLevel.y,
                worldBBox.min.z + z * voxelSizeOnLevel.z);
    Vector3 max(min.x + voxelSizeOnLevel.x,
                min.y + voxelSizeOnLevel.y,
                min.z + voxelSizeOnLevel.z);

    return AABBox3(min, max);
}

inline uint32 VisibilityOctTree::GetNodeIndexByCoord(VoxelCoord voxelCoord) const
{
    const VisibilityOctTree::SubdivisionLevelInfo& levelInfo = subdivLevelInfoArray[voxelCoord.level];
    uint32 nodeIndex = levelInfo.offset + voxelCoord.x + (voxelCoord.y + voxelCoord.z * levelInfo.size) * levelInfo.size;

    DVASSERT(voxelCoord.x < levelInfo.size);
    DVASSERT(voxelCoord.y < levelInfo.size);
    DVASSERT(voxelCoord.z < levelInfo.size);
    DVASSERT(nodeIndex < nodeMaxCount);

    return nodeIndex;
}

inline uint32 VisibilityOctTree::GetNodeIndexByLXYZ(uint32 level, uint32 x, uint32 y, uint32 z) const
{
    const VisibilityOctTree::SubdivisionLevelInfo& levelInfo = subdivLevelInfoArray[level];
    uint32 nodeIndex = levelInfo.offset + x + (y + z * levelInfo.size) * levelInfo.size;

    DVASSERT(x < levelInfo.size);
    DVASSERT(y < levelInfo.size);
    DVASSERT(z < levelInfo.size);
    DVASSERT(nodeIndex < nodeMaxCount);

    return nodeIndex;
}
};

#endif // __VISIBILITY_OCT_TREE_H__
