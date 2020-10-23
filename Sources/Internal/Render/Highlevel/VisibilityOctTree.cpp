#include "Render/Highlevel/RenderObject.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/VisibilityOctTree.h"
#include "Render/Highlevel/GeometryOctTree.h"
#include "Logger/Logger.h"

namespace DAVA
{
VisibilityOctTree::VisibilityOctTree()
{
    worldBBox = AABBox3(Vector3(-400.0f, -400.0f, -400.0f), Vector3(400.0f, 400.0f, 400.f));

    subdivLevelInfoArray.resize(maxSubdivisionLevel);
    stats.resize(maxSubdivisionLevel);
    nodeMaxCount = 0;

    uint32 size = 1;
    for (uint32 k = 0; k < maxSubdivisionLevel; ++k)
    {
        subdivLevelInfoArray[k].offset = nodeMaxCount;
        subdivLevelInfoArray[k].size = size;
        nodeMaxCount += size * size * size;

        Logger::FrameworkDebug("Vis Tree Level: %d size: %d x %d x %d", k, size, size, size);
        size *= 2;
    }

    nodeArray.resize(nodeMaxCount);
    for (uint32 k = 0; k < nodeMaxCount; ++k)
    {
        nodeArray[k].objectsInTheNode = 0;
        nodeArray[k].leafIndex = EMPTY_LEAF;
        nodeArray[k].children = 0;
        nodeArray[k].startClippingPlane = 0;
    }

    Logger::FrameworkDebug("Memory Approx Usage: %d Node size: %d", nodeMaxCount * sizeof(VisibilityOctTreeNode), sizeof(VisibilityOctTreeNode));
}

VisibilityOctTree::~VisibilityOctTree()
{
}

const AABBox3& VisibilityOctTree::GetWorldBoundingBox() const
{
    return worldBBox;
}

void VisibilityOctTree::DetermineBoxBoundsInVoxels(RenderObject* renderObject, uint32& level, uint32* voxelMin, uint32* voxelMax)
{
    for (uint32 k = 0; k < 3; ++k)
    {
        voxelMin[k] = 0;
        voxelMax[k] = 0;
    }
    level = 0;

    const AABBox3& objectWorldBBox = renderObject->GetWorldBoundingBox();
    if ((renderObject->GetFlags() & RenderObject::ALWAYS_CLIPPING_VISIBLE) || (!worldBBox.IsInside(objectWorldBBox)))
    {
        return;
    }

    // object is inside
    Vector3 relativeMin = (objectWorldBBox.min - worldBBox.min) / (worldBBox.max - worldBBox.min) * static_cast<float>(oneAxisMaxPosition);
    Vector3 relativeMax = (objectWorldBBox.max - worldBBox.min) / (worldBBox.max - worldBBox.min) * static_cast<float>(oneAxisMaxPosition);

    level = maxSubdivisionLevel - 1;
    float32 currentAxisMaxPosition = static_cast<float32>(oneAxisMaxPosition);

    while (1)
    {
        voxelMin[0] = static_cast<uint32>(Clamp(relativeMin.x, 0.0f, currentAxisMaxPosition - 1.0f));
        voxelMin[1] = static_cast<uint32>(Clamp(relativeMin.y, 0.0f, currentAxisMaxPosition - 1.0f));
        voxelMin[2] = static_cast<uint32>(Clamp(relativeMin.z, 0.0f, currentAxisMaxPosition - 1.0f));
        voxelMax[0] = static_cast<uint32>(Clamp(relativeMax.x, 0.0f, currentAxisMaxPosition - 1.0f));
        voxelMax[1] = static_cast<uint32>(Clamp(relativeMax.y, 0.0f, currentAxisMaxPosition - 1.0f));
        voxelMax[2] = static_cast<uint32>(Clamp(relativeMax.z, 0.0f, currentAxisMaxPosition - 1.0f));

        uint32 voxelsAffected = (voxelMax[2] - voxelMin[2] + 1) * (voxelMax[1] - voxelMin[1] + 1) * (voxelMax[0] - voxelMin[0] + 1);
        if (voxelsAffected <= 2)
            break;

        relativeMin /= 2.0f;
        relativeMax /= 2.0f;
        currentAxisMaxPosition /= 2.0f;
        level--;
    }

    /*
        maxSubdivLevel = 2
        oneAxisMaxPosition = 1 << (maxSubdivLevel - 1) = 2
        oneAxisMaxPosition - 1 = 1;
        rminx = 1.2
        rmaxx = 2.0
        rminx = 0.6;
        rmaxx = 1.0;
        -------------
        |     |     |
        |     |     |
        -------------
        |     |     |
        |     |     |
        -------------
    */
    uint32 levelDiff = maxSubdivisionLevel - 1 - level;
    Vector3 checkMin(static_cast<float>(voxelMin[0] << levelDiff) / static_cast<float>(oneAxisMaxPosition),
                     static_cast<float>(voxelMin[1] << levelDiff) / static_cast<float>(oneAxisMaxPosition),
                     static_cast<float>(voxelMin[2] << levelDiff) / static_cast<float>(oneAxisMaxPosition));
    Vector3 checkMax(static_cast<float>((voxelMax[0] + 1) << levelDiff) / static_cast<float>(oneAxisMaxPosition),
                     static_cast<float>((voxelMax[1] + 1) << levelDiff) / static_cast<float>(oneAxisMaxPosition),
                     static_cast<float>((voxelMax[2] + 1) << levelDiff) / static_cast<float>(oneAxisMaxPosition));

    checkMin *= (worldBBox.max - worldBBox.min);
    checkMin += worldBBox.min;
    checkMax *= (worldBBox.max - worldBBox.min);
    checkMax += worldBBox.min;

    AABBox3 checkBox(checkMin, checkMax);
    DVASSERT(checkBox.IsInside(objectWorldBBox) == true);
}

void VisibilityOctTree::AddRenderObject(RenderObject* renderObject)
{
    uint32 level;
    uint32 rmin[3];
    uint32 rmax[3];

    DetermineBoxBoundsInVoxels(renderObject, level, rmin, rmax);

    for (uint32 nx = rmin[0]; nx <= rmax[0]; ++nx)
        for (uint32 ny = rmin[1]; ny <= rmax[1]; ++ny)
            for (uint32 nz = rmin[2]; nz <= rmax[2]; ++nz)
            {
                VoxelCoord coord;
                coord.x = nx;
                coord.y = ny;
                coord.z = nz;
                coord.level = level;
                InternalAddRenderObject(renderObject, coord);
            }

    roIndices.push_back(renderObject);
    renderObject->SetTreeNodeIndex(static_cast<uint16>(roIndices.size() - 1));
    visibleObjects.Resize(static_cast<uint32>(roIndices.size()));
}

void VisibilityOctTree::RemoveRenderObject(RenderObject* renderObject)
{
    uint32 voxelCount = renderObject->GetVisibilityStructureNodeCount();
    for (uint32 voxelIndex = 0; voxelIndex < voxelCount; ++voxelIndex)
    {
        VoxelCoord coord;
        coord.packedCoord = renderObject->GetVisibilityStructureNode(voxelIndex);

        InternalRemoveRenderObject(renderObject, coord);
        voxelCount--;
        voxelIndex--;
    }

    uint32 index = FindAndRemoveExchangingWithLast(roIndices, renderObject);
    roIndices[index]->SetTreeNodeIndex(index);
    DVASSERT(index != static_cast<uint32>(-1));
}

void VisibilityOctTree::ObjectUpdated(RenderObject* renderObject)
{
    uint32 level;
    uint32 rmin[3];
    uint32 rmax[3];

    DetermineBoxBoundsInVoxels(renderObject, level, rmin, rmax);

    uint32 voxelCount = renderObject->GetVisibilityStructureNodeCount();
    for (uint32 voxelIndex = 0; voxelIndex < voxelCount; ++voxelIndex)
    {
        VoxelCoord coord;
        coord.packedCoord = renderObject->GetVisibilityStructureNode(voxelIndex);

        if (!((coord.x >= rmin[0]) && (coord.x <= rmax[0])
              && (coord.y >= rmin[1]) && (coord.y <= rmax[1])
              && (coord.z >= rmin[2]) && (coord.z <= rmax[2])
              && (coord.level == level)))
        {
            // Either coords differ or level is different. Remove object from this voxel.
            InternalRemoveRenderObject(renderObject, coord);
            voxelIndex--;
            voxelCount--;
        }
    }

    // Here we have removed object from all voxels
    for (uint32 nx = rmin[0]; nx <= rmax[0]; ++nx)
        for (uint32 ny = rmin[1]; ny <= rmax[1]; ++ny)
            for (uint32 nz = rmin[2]; nz <= rmax[2]; ++nz)
            {
                VoxelCoord coord;
                coord.x = nx;
                coord.y = ny;
                coord.z = nz;
                coord.level = level;

                if (!renderObject->IsInVisibilityStructureNode(coord.packedCoord))
                {
                    InternalAddRenderObject(renderObject, coord);
                }
            }
}

void VisibilityOctTree::Clip(Camera* _camera, Vector<RenderObject*>& visibilityArray, uint32 _visibilityCriteria)
{
    camera = _camera;
    visibilityCriteria = _visibilityCriteria;
    frustum = camera->GetFrustum();

    visibleObjects.Clear();
    VoxelCoord rootCoord;
    rootCoord.level = 0;
    rootCoord.x = 0;
    rootCoord.y = 0;
    rootCoord.z = 0;

    InternalClip(rootCoord, worldBBox, 0x3f, visibilityArray);

    uint32 size = static_cast<uint32>(roIndices.size());
    for (uint32 k = 0; k < size; ++k)
        if (visibleObjects.At(k))
        {
            DVASSERT(static_cast<uint16>(k) == roIndices[k]->GetTreeNodeIndex());
            visibilityArray.push_back(roIndices[k]);
#if defined(__DAVAENGINE_RENDERSTATS__)
            ++Renderer::GetRenderStats().visibleRenderObjects;
#endif
        }
}

void VisibilityOctTree::InternalClip(VoxelCoord voxelCoord, const AABBox3& nodeBox, uint8 clipMask, Vector<RenderObject*>& visibilityArray)
{
    VisibilityOctTree::SubdivisionLevelInfo& levelInfo = subdivLevelInfoArray[voxelCoord.level];
    uint32 nodeIndex = levelInfo.offset + voxelCoord.x + (voxelCoord.y + voxelCoord.z * levelInfo.size) * levelInfo.size;
    DVASSERT(nodeIndex < nodeMaxCount);
    VisibilityOctTreeNode& currentNode = nodeArray[nodeIndex];

    uint32 leafIndex = currentNode.leafIndex;
    Frustum::eFrustumResult frustumRes = Frustum::EFR_INSIDE;

    if (clipMask)
    {
        uint8 startClippingPlane = static_cast<uint8>(currentNode.startClippingPlane);
        frustumRes = frustum->Classify(nodeBox, clipMask, startClippingPlane);
        currentNode.startClippingPlane = startClippingPlane;
        if (frustumRes == Frustum::EFR_OUTSIDE)
            return;
    }

    if (currentNode.objectsInTheNode && leafIndex != EMPTY_LEAF)
    {
        // if we are here we already passed box test.
        std::vector<RenderObject*>& leaf = leafs[currentNode.leafIndex];
        uint32 size = static_cast<uint32>(leaf.size());

        if (!clipMask) //node is fully inside frustum - no need to clip anymore
        {
            for (uint32 i = 0; i < size; ++i)
            {
                RenderObject* renderObject = leaf[i];
                uint32 flags = renderObject->GetFlags();

                if ((flags & visibilityCriteria) == visibilityCriteria)
                {
                    visibleObjects.Set(renderObject->GetTreeNodeIndex(), true);
                    //Logger::Debug("oa: %d", renderObject->GetTreeNodeIndex());
                }
            }
        }
        else
        {
            for (uint32 i = 0; i < size; ++i)
            {
                RenderObject* renderObject = leaf[i];
                uint32 flags = renderObject->GetFlags();
                uint32 roIndex = renderObject->GetTreeNodeIndex();

                // Skip object if we already clipped it in another leaf
                if (visibleObjects.At(roIndex))
                    continue;

                if ((flags & visibilityCriteria) == visibilityCriteria)
                {
                    if ((flags & RenderObject::ALWAYS_CLIPPING_VISIBLE) || frustum->IsInside(renderObject->GetWorldBoundingBox(), clipMask, renderObject->startClippingPlane))
                    {
                        visibleObjects.Set(roIndex, true);
                        //Logger::Debug("oa: %d", renderObject->GetTreeNodeIndex());
                    }
                }
            }
        }
    }

    if (currentNode.children)
    {
        Vector3 halfBox = nodeBox.GetSize() / 2.0f;

        for (uint32 childBitIndex = 0; childBitIndex < 8; ++childBitIndex)
        {
            if ((currentNode.children >> childBitIndex) & 1)
            {
                uint32 xdiv = childBitIndex & 1;
                uint32 ydiv = (childBitIndex >> 1) & 1;
                uint32 zdiv = (childBitIndex >> 2) & 1;

                Vector3 childBoxMin(nodeBox.min.x + halfBox.x * static_cast<float>(xdiv),
                                    nodeBox.min.y + halfBox.y * static_cast<float>(ydiv),
                                    nodeBox.min.z + halfBox.z * static_cast<float>(zdiv));

                AABBox3 childBox(childBoxMin, childBoxMin + halfBox);

                VoxelCoord childCoord;
                childCoord.level = voxelCoord.level + 1;
                childCoord.x = voxelCoord.x * 2 + xdiv;
                childCoord.y = voxelCoord.y * 2 + ydiv;
                childCoord.z = voxelCoord.z * 2 + zdiv;

                InternalClip(childCoord, childBox, clipMask, visibilityArray);
            }
        }
    }
}

void VisibilityOctTree::GetAllObjectsInBBox(const AABBox3& bbox, Vector<RenderObject*>& visibilityArray)
{
}

// TODO: Try to collide objects during scene traverse to stop faster then collision is found
void VisibilityOctTree::BroadPhaseCollisions(const Ray3& rayInWorldSpace, Vector<BroadPhaseCollision>& broadPhaseCollisions)
{
    VoxelCoord rootCoord;
    rootCoord.level = 0;
    rootCoord.x = 0;
    rootCoord.y = 0;
    rootCoord.z = 0;
    broadPhaseQueue.push(rootCoord);

    /*    while(broadPhaseQueue.size() > 0)
    {
        VoxelCoord voxelCoord = broadPhaseQueue.front();
        broadPhaseQueue.pop();
        VisibilityOctTreeNode & node = nodeArray[GetNodeIndexByCoord(voxelCoord)];
        node.debugBlue = false;
        for (uint32 childBitIndex = 0; childBitIndex < 8; ++childBitIndex)
        {
            if ((node.children >> childBitIndex) & 1)
            {
                uint32 xdiv = childBitIndex & 1;
                uint32 ydiv = (childBitIndex >> 1) & 1;
                uint32 zdiv = (childBitIndex >> 2) & 1;

                VoxelCoord childCoord;
                childCoord.level = voxelCoord.level + 1;
                childCoord.x = voxelCoord.x * 2 + xdiv;
                childCoord.y = voxelCoord.y * 2 + ydiv;
                childCoord.z = voxelCoord.z * 2 + zdiv;
                broadPhaseQueue.push(childCoord);
            }
        }
    }*/

    //    VoxelCoord rootCoord;
    rootCoord.level = 0;
    rootCoord.x = 0;
    rootCoord.y = 0;
    rootCoord.z = 0;
    broadPhaseQueue.push(rootCoord);

    while (broadPhaseQueue.size() > 0)
    {
        VoxelCoord voxelCoord = broadPhaseQueue.front();
        broadPhaseQueue.pop();
        AABBox3 nodeBox = ReconstructAABBox(voxelCoord);
        VisibilityOctTreeNode& node = nodeArray[GetNodeIndexByCoord(voxelCoord)];

        localRayBoxTraceCount++;
        if (Intersection::RayBox(rayInWorldSpace, nodeBox))
        {
            //node.debugBlue = true;
            if (node.leafIndex != EMPTY_LEAF)
            {
                std::vector<RenderObject*>& leafArray = leafs[node.leafIndex];
                uint32 objectsCount = static_cast<uint32>(leafArray.size());
                localRayBoxTraceCount += objectsCount;
                for (uint32 i = 0; i < objectsCount; ++i)
                {
                    RenderObject* renderObject = leafArray[i];
                    float32 objTMin, objTMax;
                    uint32 renderObjectTreeNodeIndex = renderObject->GetTreeNodeIndex();
                    if (!visibleObjects.At(renderObjectTreeNodeIndex))
                        if (Intersection::RayBox(rayInWorldSpace, renderObject->GetWorldBoundingBox(), objTMin, objTMax))
                        {
                            //
                            //auto it = std::upper_bound(broadPhaseCollisions.begin(), broadPhaseCollisions.end(), tMin, lambda);
                            broadPhaseCollisions.push_back({ objTMin, renderObject });
                            visibleObjects.Set(renderObjectTreeNodeIndex, true);
                        }
                }
            }

            for (uint32 childBitIndex = 0; childBitIndex < 8; ++childBitIndex)
            {
                if ((node.children >> childBitIndex) & 1)
                {
                    uint32 xdiv = childBitIndex & 1;
                    uint32 ydiv = (childBitIndex >> 1) & 1;
                    uint32 zdiv = (childBitIndex >> 2) & 1;

                    VoxelCoord childCoord;
                    childCoord.level = voxelCoord.level + 1;
                    childCoord.x = voxelCoord.x * 2 + xdiv;
                    childCoord.y = voxelCoord.y * 2 + ydiv;
                    childCoord.z = voxelCoord.z * 2 + zdiv;
                    broadPhaseQueue.push(childCoord);
                }
            }
        }
    }
    auto lambda = [](BroadPhaseCollision& pair1, BroadPhaseCollision& pair2) -> bool { return pair1.first < pair2.first; };

    std::sort(broadPhaseCollisions.begin(), broadPhaseCollisions.end(), lambda);
}

bool VisibilityOctTree::RayTrace(const Ray3& ray, RayTraceCollision& collision, const Vector<RenderObject*>& ignoreObjects)
{
    broadPhaseCollisions.clear();
    visibleObjects.Clear();

    localRayBoxTraceCount = 0;
    BroadPhaseCollisions(ray, broadPhaseCollisions);

    // TODO: Remove duplication of this code in SpatialTree and RenderHierarchy
    bool intersectionFound = false;
    float32 closestT = FLOAT_MAX;

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

void VisibilityOctTree::Initialize()
{
}

void VisibilityOctTree::Update()
{
    //CollectAndPrintStats();
}

void VisibilityOctTree::DebugDraw(const Matrix4& cameraMatrix, RenderHelper* renderHelper)
{
    VoxelCoord rootCoord;
    rootCoord.level = 0;
    rootCoord.x = 0;
    rootCoord.y = 0;
    rootCoord.z = 0;
    InternalDebugDraw(rootCoord, cameraMatrix, worldBBox, renderHelper);
}

void VisibilityOctTree::InternalDebugDraw(VoxelCoord voxelCoord, const Matrix4& worldMatrix, const AABBox3& boundingBox, RenderHelper* renderHelper)
{
    VisibilityOctTreeNode& currentNode = nodeArray[GetNodeIndexByCoord(voxelCoord)];

    if (voxelCoord.level == 0)
        renderHelper->DrawAABoxTransformed(boundingBox, worldMatrix, Color(1.0f, 1.0f, 1.0f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);
    else if (currentNode.debugBlue)
        renderHelper->DrawAABoxTransformed(boundingBox, worldMatrix, Color(0.0f, 0.0f, 1.0f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);
    else if (currentNode.objectsInTheNode)
        renderHelper->DrawAABoxTransformed(boundingBox, worldMatrix, Color(1.0f, 0.0f, 0.0f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);
    else
        renderHelper->DrawAABoxTransformed(boundingBox, worldMatrix, Color(1.0f, 1.0f, 0.0f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);

    Vector3 halfBox = boundingBox.GetSize() / 2.0f;

    for (uint32 xdiv = 0; xdiv < 2; ++xdiv)
    {
        for (uint32 ydiv = 0; ydiv < 2; ++ydiv)
        {
            for (uint32 zdiv = 0; zdiv < 2; ++zdiv)
            {
                uint32 childBitIndex = xdiv + (2 * ydiv) + (4 * zdiv);

                if ((currentNode.children >> childBitIndex) & 1)
                {
                    Vector3 childBoxMin(boundingBox.min.x + halfBox.x * static_cast<float>(xdiv),
                                        boundingBox.min.y + halfBox.y * static_cast<float>(ydiv),
                                        boundingBox.min.z + halfBox.z * static_cast<float>(zdiv));
                    //Logger::Debug(Format("Start: %d %d %d", xdiv, ydiv, zdiv).c_str());

                    AABBox3 childBox(childBoxMin, childBoxMin + halfBox);

                    VoxelCoord childCoord;
                    childCoord.level = voxelCoord.level + 1;
                    childCoord.x = voxelCoord.x * 2 + xdiv;
                    childCoord.y = voxelCoord.y * 2 + ydiv;
                    childCoord.z = voxelCoord.z * 2 + zdiv;

                    Frustum::eFrustumResult frustumRes = Frustum::EFR_INSIDE;
                    frustumRes = frustum->Classify(childBox);

                    if (frustumRes != Frustum::EFR_OUTSIDE)
                        InternalDebugDraw(childCoord, worldMatrix, childBox, renderHelper);
                }
            }
        }
    }
}

void VisibilityOctTree::CollectAndPrintStats()
{
    for (uint32 level = 0; level < maxSubdivisionLevel; ++level)
    {
        SubdivisionLevelInfo& levelInfo = subdivLevelInfoArray[level];
        Stats& levelStats = stats[level];
        levelStats.objectsCount = 0;

        for (uint32 x = 0; x < levelInfo.size; ++x)
            for (uint32 y = 0; y < levelInfo.size; ++y)
                for (uint32 z = 0; z < levelInfo.size; ++z)
                {
                    uint32 nodeIndex = GetNodeIndexByLXYZ(level, x, y, z);
                    levelStats.objectsCount += static_cast<uint32>(nodeArray[nodeIndex].objectsInTheNode);
                }
        Logger::Debug("Level: %d Objects: %d", level, levelStats.objectsCount);
    }
}

uint32 VisibilityOctTree::AllocateLeaf()
{
    if (freeLeafs.size() != 0)
    {
        uint32 leafIndex = freeLeafs[freeLeafs.size() - 1];
        freeLeafs.pop_back();
        return leafIndex;
    }

    uint32 leafIndex = static_cast<uint32>(leafs.size());
    leafs.push_back(std::vector<RenderObject*>());
    return leafIndex;
}

void VisibilityOctTree::DeallocateLeaf(uint32 leafIndex)
{
    DVASSERT(leafs[leafIndex].size() == 0);
    freeLeafs.push_back(leafIndex);
}

void VisibilityOctTree::InternalAddRenderObject(RenderObject* renderObject, VoxelCoord voxelCoord)
{
    uint32 nodeIndex = GetNodeIndexByCoord(voxelCoord);

    VisibilityOctTreeNode& node = nodeArray[nodeIndex];

    if (node.leafIndex == EMPTY_LEAF)
    {
        node.leafIndex = AllocateLeaf();
    }

    DVASSERT(node.leafIndex < leafs.size());
    std::vector<RenderObject*>& leafArray = leafs[node.leafIndex];
    leafArray.push_back(renderObject);

    renderObject->AddVisibilityStructureNode(voxelCoord.packedCoord);

    node.objectsInTheNode++;

    uint32 objectsInParent = node.objectsInTheNode;
    uint32 childrenInParent = node.children;

    uint32 x = voxelCoord.x / 2;
    uint32 y = voxelCoord.y / 2;
    uint32 z = voxelCoord.z / 2;

    uint32 cx = voxelCoord.x & 1;
    uint32 cy = voxelCoord.y & 1;
    uint32 cz = voxelCoord.z & 1;

    for (int32 level = voxelCoord.level - 1; level >= 0; level--)
    {
        nodeIndex = GetNodeIndexByLXYZ(level, x, y, z);
        VisibilityOctTreeNode& affectedNode = nodeArray[nodeIndex];
        uint64 childBitIndex = cx + (2 * cy) + (4 * cz);
        if (objectsInParent > 0 || (childrenInParent != 0))
            affectedNode.children |= 1ull << childBitIndex;
        else
            affectedNode.children &= ~(1ull << childBitIndex);

        objectsInParent = affectedNode.objectsInTheNode;
        childrenInParent = affectedNode.children;

        cx = x & 1;
        cy = y & 1;
        cz = z & 1;
        x /= 2;
        y /= 2;
        z /= 2;
    }

    //CollectAndPrintStats();
}

void VisibilityOctTree::InternalRemoveRenderObject(RenderObject* renderObject, VoxelCoord voxelCoord)
{
    uint32 nodeIndex = GetNodeIndexByCoord(voxelCoord);
    VisibilityOctTreeNode& node = nodeArray[nodeIndex];

    std::vector<RenderObject*>& leafArray = leafs[node.leafIndex];

    int32 size = static_cast<int32>(leafArray.size());
    for (int32 k = 0; k < size; ++k)
        if (leafArray[k] == renderObject)
        {
            leafArray[k] = leafArray[size - 1];
            k--;
            size--;
            leafArray.pop_back();
        }

    node.objectsInTheNode--;
    if (node.objectsInTheNode == 0)
    {
        DVASSERT(leafArray.size() == 0);
        DeallocateLeaf(node.leafIndex);
        node.leafIndex = EMPTY_LEAF;
    }

    renderObject->RemoveVisibilityStructureNode(voxelCoord.packedCoord);

    uint32 objectsInParent = node.objectsInTheNode;
    uint32 childrenInParent = node.children;

    uint32 x = voxelCoord.x / 2;
    uint32 y = voxelCoord.y / 2;
    uint32 z = voxelCoord.z / 2;

    uint32 cx = voxelCoord.x & 1;
    uint32 cy = voxelCoord.y & 1;
    uint32 cz = voxelCoord.z & 1;

    for (int32 level = voxelCoord.level - 1; level >= 0; level--)
    {
        nodeIndex = GetNodeIndexByLXYZ(level, x, y, z);
        VisibilityOctTreeNode& affectedNode = nodeArray[nodeIndex];
        uint32 childBitIndex = cx + (2 * cy) + (4 * cz);
        if (objectsInParent > 0 || (childrenInParent != 0))
            affectedNode.children |= 1ull << childBitIndex;
        else
            affectedNode.children &= ~(1ull << childBitIndex);

        objectsInParent = affectedNode.objectsInTheNode;
        childrenInParent = affectedNode.children;

        cx = x & 1;
        cy = y & 1;
        cz = z & 1;
        x /= 2;
        y /= 2;
        z /= 2;
    }
}
};
