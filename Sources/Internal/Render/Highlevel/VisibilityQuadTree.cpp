#include "Render/Highlevel/VisibilityQuadTree.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Highlevel/RenderBatchArray.h"
#include "Render/Highlevel/GeometryOctTree.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/RenderHelper.h"

namespace DAVA
{
QuadTree::QuadTreeNode::QuadTreeNode()
{
    Reset();
}

void QuadTree::QuadTreeNode::Reset()
{
    parent = INVALID_TREE_NODE_INDEX;
    for (int32 i = 0; i < 4; i++)
        children[i] = INVALID_TREE_NODE_INDEX;
    nodeInfo = 0;
}

QuadTree::QuadTree(int32 _maxTreeDepth)
    : maxTreeDepth(_maxTreeDepth)
{
}

bool QuadTree::CheckBoxIntersectBranch(const AABBox3& objBox, float32 xmin, float32 ymin, float32 xmax, float32 ymax)
{
    if ((xmin > objBox.max.x) || (ymin > objBox.max.y) || (xmax < objBox.min.x) || (ymax < objBox.min.y))
        return false;
    return true;
}

bool QuadTree::CheckObjectFitNode(const AABBox3& objBox, const AABBox3& nodeBox)
{
    return (objBox.min.x >= nodeBox.min.x) && (objBox.min.y >= nodeBox.min.y) && (objBox.max.x <= nodeBox.max.x) && (objBox.max.y <= nodeBox.max.y);
}

bool QuadTree::CheckBoxIntersectChild(const AABBox3& objBox, const AABBox3& nodeBox, QuadTreeNode::eNodeType nodeType)
{
    //note - this code assumes box already intersects parent
    switch (nodeType)
    {
    case QuadTreeNode::NODE_LB:
        return (0.5f * (nodeBox.min.x + nodeBox.max.x) >= objBox.min.x) && (0.5f * (nodeBox.min.y + nodeBox.max.y) >= objBox.min.y);
    case QuadTreeNode::NODE_RB:
        return (0.5f * (nodeBox.min.x + nodeBox.max.x) <= objBox.max.x) && (0.5f * (nodeBox.min.y + nodeBox.max.y) >= objBox.min.y);
    case QuadTreeNode::NODE_LT:
        return (0.5f * (nodeBox.min.x + nodeBox.max.x) >= objBox.min.x) && (0.5f * (nodeBox.min.y + nodeBox.max.y) <= objBox.max.y);
    case QuadTreeNode::NODE_RT:
        return (0.5f * (nodeBox.min.x + nodeBox.max.x) <= objBox.max.x) && (0.5f * (nodeBox.min.y + nodeBox.max.y) <= objBox.max.y);
    default:
        break;
    }
    return false;
}

void QuadTree::UpdateChildBox(AABBox3& parentBox, QuadTreeNode::eNodeType childType)
{
    switch (childType)
    {
    case QuadTreeNode::NODE_LB:
        parentBox.max.x = (parentBox.max.x + parentBox.min.x) * 0.5f;
        parentBox.max.y = (parentBox.max.y + parentBox.min.y) * 0.5f;
        break;
    case QuadTreeNode::NODE_RB:
        parentBox.min.x = (parentBox.max.x + parentBox.min.x) * 0.5f;
        parentBox.max.y = (parentBox.max.y + parentBox.min.y) * 0.5f;
        break;
    case QuadTreeNode::NODE_LT:
        parentBox.max.x = (parentBox.max.x + parentBox.min.x) * 0.5f;
        parentBox.min.y = (parentBox.max.y + parentBox.min.y) * 0.5f;
        break;
    case QuadTreeNode::NODE_RT:
        parentBox.min.x = (parentBox.max.x + parentBox.min.x) * 0.5f;
        parentBox.min.y = (parentBox.max.y + parentBox.min.y) * 0.5f;
        break;
    default:
        break;
    }
}

void QuadTree::UpdateParentBox(AABBox3& childtBox, QuadTreeNode::eNodeType childType)
{
    switch (childType)
    {
    case QuadTreeNode::NODE_LB:
        childtBox.max.x += (childtBox.max.x - childtBox.min.x);
        childtBox.max.y += (childtBox.max.y - childtBox.min.y);
        break;
    case QuadTreeNode::NODE_RB:
        childtBox.min.x -= (childtBox.max.x - childtBox.min.x);
        childtBox.max.y += (childtBox.max.y - childtBox.min.y);
        break;
    case QuadTreeNode::NODE_LT:
        childtBox.max.x += (childtBox.max.x - childtBox.min.x);
        childtBox.min.y -= (childtBox.max.y - childtBox.min.y);
        break;
    case QuadTreeNode::NODE_RT:
        childtBox.min.x -= (childtBox.max.x - childtBox.min.x);
        childtBox.min.y -= (childtBox.max.y - childtBox.min.y);
        break;
    default:
        break;
    }
}

uint16 QuadTree::FindObjectAddNode(uint16 startNodeId, const AABBox3& objBox)
{
    uint16 currIndex = startNodeId;

    bool placeHere = false;

    do
    {
        QuadTreeNode& currNode = nodes[currIndex];
        currNode.bbox.min.z = Min(currNode.bbox.min.z, objBox.min.z);
        currNode.bbox.max.z = Max(currNode.bbox.max.z, objBox.max.z);
        placeHere = ((currNode.nodeInfo >> QuadTreeNode::NODE_DEPTH_OFFSET) >= maxTreeDepth);

        QuadTreeNode::eNodeType fitNode = QuadTreeNode::NODE_NONE;
        if (!placeHere)
        {
            float32 midx = 0.5f * (currNode.bbox.min.x + currNode.bbox.max.x);
            float32 midy = 0.5f * (currNode.bbox.min.y + currNode.bbox.max.y);
            if ((midx >= objBox.min.x) && (midy >= objBox.min.y))
                fitNode = QuadTreeNode::NODE_LB;
            if ((midx <= objBox.max.x) && (midy >= objBox.min.y))
            {
                if (fitNode == QuadTreeNode::NODE_NONE)
                    fitNode = QuadTreeNode::NODE_RB;
                else
                    placeHere = true;
            }
            if ((!placeHere) && (midx >= objBox.min.x) && (midy <= objBox.max.y))
            {
                if (fitNode == QuadTreeNode::NODE_NONE)
                    fitNode = QuadTreeNode::NODE_LT;
                else
                    placeHere = true;
            }
            if ((!placeHere) && (midx <= objBox.max.x) && (midy <= objBox.max.y))
            {
                if (fitNode == QuadTreeNode::NODE_NONE)
                    fitNode = QuadTreeNode::NODE_RT;
                else
                    placeHere = true;
            }
        }

        if (!placeHere) //continue downwards
        {
            if (currNode.children[fitNode] == INVALID_TREE_NODE_INDEX) //set child node if not exist
            {
                DVASSERT((nodes[currIndex].nodeInfo & QuadTreeNode::NUM_CHILD_NODES_MASK) != 4);
                uint16 newNodeIndex;
                if (emptyNodes.size()) //take from empty
                {
                    newNodeIndex = static_cast<uint16>(emptyNodes.back());
                    emptyNodes.pop_back();
                    nodes[newNodeIndex].Reset();
                }
                else //or create new node
                {
                    newNodeIndex = uint16(nodes.size());
                    nodes.resize(newNodeIndex + 1); //starting from here currNode may be invalid
                }
                nodes[newNodeIndex].nodeInfo = (nodes[currIndex].nodeInfo & QuadTreeNode::NODE_DEPTH_MASK) + (1 << QuadTreeNode::NODE_DEPTH_OFFSET); //depth
                nodes[newNodeIndex].parent = currIndex;
                nodes[newNodeIndex].bbox = nodes[currIndex].bbox;
                UpdateChildBox(nodes[newNodeIndex].bbox, fitNode);
                nodes[newNodeIndex].bbox.min.z = AABBOX_INFINITY;
                nodes[newNodeIndex].bbox.max.z = -AABBOX_INFINITY;

                nodes[currIndex].children[fitNode] = newNodeIndex;
                nodes[currIndex].nodeInfo++; //numChildNodes++
            }
            currIndex = nodes[currIndex].children[fitNode];
        }
    } while (!placeHere);

    return currIndex;
}

void QuadTree::MarkNodeDirty(uint16 nodeId)
{
    if ((nodes[nodeId].nodeInfo & QuadTreeNode::DIRTY_Z_MASK) != QuadTreeNode::DIRTY_Z_MASK)
    {
        nodes[nodeId].nodeInfo |= QuadTreeNode::DIRTY_Z_MASK;
        dirtyZNodes.push_back(nodeId);
    }
}

void QuadTree::MarkObjectDirty(RenderObject* object)
{
    if (!(object->GetFlags() & RenderObject::TREE_NODE_NEED_UPDATE))
    {
        object->AddFlag(RenderObject::TREE_NODE_NEED_UPDATE);
        dirtyObjects.push_back(object);
    }
}

void QuadTree::RecalculateNodeZLimits(uint16 nodeId)
{
    QuadTreeNode& currNode = nodes[nodeId];
    currNode.bbox.min.z = AABBOX_INFINITY;
    currNode.bbox.max.z = -AABBOX_INFINITY;
    for (int32 i = 0; i < QuadTreeNode::NODE_NONE; i++)
    {
        uint16 childId = currNode.children[i];
        if (childId != INVALID_TREE_NODE_INDEX)
        {
            currNode.bbox.min.z = Min(currNode.bbox.min.z, nodes[childId].bbox.min.z);
            currNode.bbox.max.z = Max(currNode.bbox.max.z, nodes[childId].bbox.max.z);
        }
    }
    for (size_t i = 0, size = currNode.objects.size(); i < size; i++)
    {
        const AABBox3& objBox = currNode.objects[i]->GetWorldBoundingBox();
        currNode.bbox.min.z = Min(currNode.bbox.min.z, objBox.min.z);
        currNode.bbox.max.z = Max(currNode.bbox.max.z, objBox.max.z);
    }
    currNode.nodeInfo &= ~QuadTreeNode::DIRTY_Z_MASK;
    if (currNode.parent != INVALID_TREE_NODE_INDEX)
        MarkNodeDirty(currNode.parent);
}

void QuadTree::AddRenderObject(RenderObject* renderObject)
{
    DVASSERT(renderObject->GetTreeNodeIndex() == INVALID_TREE_NODE_INDEX);
    DVASSERT(preparedForShutdown == false);

    if (!worldInitialized)
    {
        worldInitObjects.push_back(renderObject);
        return;
    }

    const AABBox3& objBox = renderObject->GetWorldBoundingBox();
    DVASSERT(!objBox.IsEmpty());

    //ALWAYS_CLIPPING_VISIBLE should be added to root to prevent being clipped by tree
    //special treatment for root - as it can contain objects outside the world
    if ((renderObject->GetFlags() & RenderObject::ALWAYS_CLIPPING_VISIBLE) || (!worldBox.IsInside(objBox)))
    {
        //object is somehow outside the world - just add to root
        nodes[0].objects.push_back(renderObject);
        renderObject->SetTreeNodeIndex(0);
        renderObject->RemoveFlag(RenderObject::TREE_NODE_NEED_UPDATE);
        return;
    }
    uint16 nodeToAdd = FindObjectAddNode(0, renderObject->GetWorldBoundingBox());
    nodes[nodeToAdd].objects.push_back(renderObject);
    renderObject->SetTreeNodeIndex(nodeToAdd);
    renderObject->RemoveFlag(RenderObject::TREE_NODE_NEED_UPDATE);
}

void QuadTree::RemoveRenderObject(RenderObject* renderObject)
{
    if (preparedForShutdown == true)
    {
        DVASSERT(nodes.empty() == true);
        return;
    }

    if (!worldInitialized)
    {
        List<RenderObject*>::iterator it = std::find(worldInitObjects.begin(), worldInitObjects.end(), renderObject);
        DVASSERT(it != worldInitObjects.end());
        worldInitObjects.erase(it);
        return;
    }
    uint16 currIndex = renderObject->GetTreeNodeIndex();
    DVASSERT(currIndex != INVALID_TREE_NODE_INDEX);
    renderObject->SetTreeNodeIndex(INVALID_TREE_NODE_INDEX);
    Vector<RenderObject*>::iterator it = std::find(nodes[currIndex].objects.begin(), nodes[currIndex].objects.end(), renderObject);
    DVASSERT(it != nodes[currIndex].objects.end());
    nodes[currIndex].objects.erase(it);

    if (renderObject->GetFlags() & RenderObject::TREE_NODE_NEED_UPDATE)
    {
        List<RenderObject*>::iterator it = std::find(dirtyObjects.begin(), dirtyObjects.end(), renderObject);
        DVASSERT(it != dirtyObjects.end());
        dirtyObjects.erase(it);
        renderObject->RemoveFlag(RenderObject::TREE_NODE_NEED_UPDATE);
    }

    //update tree branch info
    while (currIndex != INVALID_TREE_NODE_INDEX)
    {
        QuadTreeNode& currNode = nodes[currIndex];

        if ((currIndex != 0) && //do not remove root node anyway
            currNode.objects.empty() &&
            (!(currNode.nodeInfo & QuadTreeNode::NUM_CHILD_NODES_MASK)))
        { //empty node - just remove it from tree
            emptyNodes.push_back(currIndex);
            for (int32 i = 0; i < 4; i++)
                if (nodes[currNode.parent].children[i] == currIndex)
                    nodes[currNode.parent].children[i] = INVALID_TREE_NODE_INDEX;
            DVASSERT(nodes[currNode.parent].nodeInfo & QuadTreeNode::NUM_CHILD_NODES_MASK);
            nodes[currNode.parent].nodeInfo--; //numChildNodes--
        }
        else
        {
            MarkNodeDirty(currIndex);
            break; //dirty node will automatically translate it
        }
        currIndex = currNode.parent;
    }
}

void QuadTree::Initialize()
{
    DVASSERT(preparedForShutdown == false);
    worldBox = AABBox3();

    for (List<RenderObject *>::iterator it = worldInitObjects.begin(), e = worldInitObjects.end(); it != e; ++it)
    {
        worldBox.AddAABBox((*it)->GetWorldBoundingBox());
        (*it)->SetTreeNodeIndex(INVALID_TREE_NODE_INDEX);
    }
    if (worldBox.IsEmpty())
        worldBox = AABBox3(Vector3(0, 0, 0), Vector3(0, 0, 0));

    QuadTreeNode root;
    root.bbox = worldBox;
    nodes.push_back(root);
    worldInitialized = true;

    for (List<RenderObject *>::iterator it = worldInitObjects.begin(), e = worldInitObjects.end(); it != e; ++it)
    {
        AddRenderObject(*it);
    }
    worldInitObjects.clear();
}

void QuadTree::PrepareForShutdown()
{
    broadPhaseCollisions.clear();
    nodes.clear();
    emptyNodes.clear();
    dirtyZNodes.clear();
    dirtyObjects.clear();
    worldInitObjects.clear();
    preparedForShutdown = true;
}

const AABBox3& QuadTree::GetWorldBoundingBox() const
{
    return worldBox;
}

void QuadTree::ObjectUpdated(RenderObject* renderObject)
{
    if (renderObject->GetFlags() & RenderObject::ALWAYS_CLIPPING_VISIBLE)
        return;

    DVASSERT(worldInitialized);
    //remove object from its current tree node
    uint16 baseIndex = renderObject->GetTreeNodeIndex();
    DVASSERT(baseIndex != INVALID_TREE_NODE_INDEX);

    //climb up
    const AABBox3& objBox = renderObject->GetWorldBoundingBox();
    uint16 reverseIndex = baseIndex;
    while (reverseIndex && (!CheckObjectFitNode(objBox, nodes[reverseIndex].bbox)))
    {
        reverseIndex = nodes[reverseIndex].parent;
    }

    MarkObjectDirty(renderObject);

    if (reverseIndex != baseIndex)
    {
        //remove from base
        int32 objectsSize = static_cast<int32>(nodes[baseIndex].objects.size());
        int32 objIndex = 0;
        for (; objIndex < objectsSize; ++objIndex)
        {
            if (nodes[baseIndex].objects[objIndex] == renderObject)
                break;
        }
        DVASSERT(objIndex < objectsSize);
        if (objectsSize > 1)
            nodes[baseIndex].objects[objIndex] = nodes[baseIndex].objects[objectsSize - 1];
        nodes[baseIndex].objects.resize(objectsSize - 1);
        //and add to target
        nodes[reverseIndex].objects.push_back(renderObject);
        renderObject->SetTreeNodeIndex(reverseIndex);

        /*only now we can climb back and remove/mark nodes*/
        uint16 currIndex = baseIndex;
        while (currIndex != reverseIndex)
        {
            QuadTreeNode& currNode = nodes[currIndex];

            if ((currIndex != 0) && //do not remove root node anyway
                currNode.objects.empty() &&
                (!(currNode.nodeInfo & QuadTreeNode::NUM_CHILD_NODES_MASK)))
            { //empty node - just remove it from tree
                emptyNodes.push_back(currIndex);
                for (int32 i = 0; i < 4; i++)
                    if (nodes[currNode.parent].children[i] == currIndex)
                        nodes[currNode.parent].children[i] = INVALID_TREE_NODE_INDEX;
                DVASSERT(nodes[currNode.parent].nodeInfo & QuadTreeNode::NUM_CHILD_NODES_MASK);
                nodes[currNode.parent].nodeInfo--; //numChildNodes--
            }
            else
            {
                MarkNodeDirty(currIndex);
                break;
            }
            currIndex = currNode.parent;
        }
    }
    else
    {
        MarkNodeDirty(baseIndex);
    }
    //as object change can wrap any of parent boxes
    uint16 currIndex = reverseIndex;
    bool sizeUpdeted;
    do
    {
        sizeUpdeted = false;
        if (nodes[currIndex].bbox.min.z > objBox.min.z)
        {
            nodes[currIndex].bbox.min.z = objBox.min.z;
            sizeUpdeted = true;
        }
        if (nodes[currIndex].bbox.max.z < objBox.max.z)
        {
            nodes[currIndex].bbox.max.z = objBox.max.z;
            sizeUpdeted = true;
        }
        currIndex = nodes[currIndex].parent;
    } while (sizeUpdeted && (currIndex != INVALID_TREE_NODE_INDEX));
}

void QuadTree::ProcessNodeClipping(uint16 nodeId, uint8 clippingFlags, Vector<RenderObject*>& visibilityArray)
{
    QuadTreeNode& currNode = nodes[nodeId];
    int32 objectsSize = static_cast<int32>(currNode.objects.size());
    int32 clipBoxCount = (currNode.nodeInfo & QuadTreeNode::NUM_CHILD_NODES_MASK) + objectsSize; //still can sometime try to clip node with only invisible objects

    if (clippingFlags && (clipBoxCount > 1) && nodeId) //root node is considered as always pass  - as objects out of worldBox are added here
    {
        uint8 startClipPlane = (currNode.nodeInfo & QuadTreeNode::START_CLIP_PLANE_MASK) >> QuadTreeNode::START_CLIP_PLANE_OFFSET;
        if (currFrustum->Classify(currNode.bbox, clippingFlags, startClipPlane) == Frustum::EFR_OUTSIDE)
            return; //node box is outside - return
        currNode.nodeInfo &= ~QuadTreeNode::START_CLIP_PLANE_MASK;
        currNode.nodeInfo |= (uint16(startClipPlane)) << QuadTreeNode::START_CLIP_PLANE_OFFSET;
    }
    //process objects in current node
    if (!clippingFlags) //node is fully inside frustum - no need to clip anymore
    {
        for (int32 i = 0; i < objectsSize; ++i)
        {
            RenderObject* obj = currNode.objects[i];
            uint32 flags = obj->GetFlags();
            if ((flags & currVisibilityCriteria) == currVisibilityCriteria)
            {
                visibilityArray.push_back(obj);
#if defined(__DAVAENGINE_RENDERSTATS__)
                ++Renderer::GetRenderStats().visibleRenderObjects;
#endif
            }
        }
    }
    else
    {
        for (int32 i = 0; i < objectsSize; ++i)
        {
            RenderObject* obj = currNode.objects[i];
            uint32 flags = obj->GetFlags();
            if ((flags & currVisibilityCriteria) == currVisibilityCriteria)
            {
                if ((flags & RenderObject::ALWAYS_CLIPPING_VISIBLE)
                    || currFrustum->IsInside(obj->GetWorldBoundingBox(), clippingFlags, obj->startClippingPlane))
                {
                    visibilityArray.push_back(obj);
#if defined(__DAVAENGINE_RENDERSTATS__)
                    ++Renderer::GetRenderStats().visibleRenderObjects;
#endif
                }
            }
        }
    }

    //process children
    for (int32 i = 0; i < QuadTreeNode::NODE_NONE; ++i)
    {
        uint16 childNodeId = currNode.children[i];
        if (childNodeId != INVALID_TREE_NODE_INDEX)
        {
            ProcessNodeClipping(childNodeId, clippingFlags, visibilityArray);
        }
    }
}

void QuadTree::Clip(Camera* camera, Vector<RenderObject*>& visibilityArray, uint32 visibilityCriteria)
{
    DVASSERT(worldInitialized);
    currCamera = camera;
    currVisibilityCriteria = visibilityCriteria;
    currFrustum = camera->GetFrustum();
    ProcessNodeClipping(0, 0x3f, visibilityArray);
}

void QuadTree::GetObjects(uint16 nodeId, const AABBox3& bbox, Vector<RenderObject*>& visibilityArray)
{
    QuadTreeNode& currNode = nodes[nodeId];
    int32 objectsSize = static_cast<int32>(currNode.objects.size());

    if (bbox.IntersectsWithBox(currNode.bbox))
    {
        for (int32 i = 0; i < objectsSize; ++i)
        {
            RenderObject* renderObject = currNode.objects[i];
            if (bbox.IntersectsWithBox(renderObject->GetWorldBoundingBox()))
            {
                visibilityArray.push_back(renderObject);
            }
        }

        //process children
        for (int32 i = 0; i < QuadTreeNode::NODE_NONE; ++i)
        {
            uint16 childNodeId = currNode.children[i];
            if (childNodeId != INVALID_TREE_NODE_INDEX)
            {
                GetObjects(childNodeId, bbox, visibilityArray);
            }
        }
    }
}

void QuadTree::GetAllObjectsInBBox(const AABBox3& bbox, Vector<RenderObject*>& visibilityArray)
{
    GetObjects(0, bbox, visibilityArray);
}

void QuadTree::BroadPhaseCollisions(const Ray3& rayInWorldSpace, Vector<BroadPhaseCollision>& broadPhaseCollisions)
{
    broadPhaseQueue.push(0);

    while (broadPhaseQueue.size() > 0)
    {
        uint16 nodeId = broadPhaseQueue.front();
        broadPhaseQueue.pop();

        QuadTreeNode& currNode = nodes[nodeId];
        int32 objectsSize = static_cast<int32>(currNode.objects.size());
        //    int32 clipBoxCount = (currNode.nodeInfo & QuadTreeNode::NUM_CHILD_NODES_MASK) + objectsSize; //still can sometime try to clip node with only invisible objects

        float32 tMin, tMax;
        localRayBoxTraceCount++;
        if (Intersection::RayBox(rayInWorldSpace, currNode.bbox, tMin, tMax))
        {
            localRayBoxTraceCount += objectsSize;
            for (int32 i = 0; i < objectsSize; ++i)
            {
                RenderObject* renderObject = currNode.objects[i];
                float32 objTMin, objTMax;
                if (Intersection::RayBox(rayInWorldSpace, renderObject->GetWorldBoundingBox(), objTMin, objTMax))
                {
                    //
                    //auto it = std::upper_bound(broadPhaseCollisions.begin(), broadPhaseCollisions.end(), tMin, lambda);
                    broadPhaseCollisions.push_back({ objTMin, renderObject });
                }
            }

            //process children
            for (int32 i = 0; i < QuadTreeNode::NODE_NONE; ++i)
            {
                uint16 childNodeId = currNode.children[i];
                if (childNodeId != INVALID_TREE_NODE_INDEX)
                {
                    broadPhaseQueue.push(childNodeId);
                }
            }
        }
    }
    auto lambda = [](BroadPhaseCollision& pair1, BroadPhaseCollision& pair2) -> bool { return pair1.first < pair2.first; };

    std::sort(broadPhaseCollisions.begin(), broadPhaseCollisions.end(), lambda);
}

bool QuadTree::RayTrace(const Ray3& ray, RayTraceCollision& collision, const Vector<RenderObject*>& ignoreObjects)
{
    broadPhaseCollisions.clear();

    localRayBoxTraceCount = 0;
    BroadPhaseCollisions(ray, broadPhaseCollisions);

    // TODO: Remove duplication of this code in SpatialTree and RenderHierarchy
    bool intersectionFound = false;
    float32 closestT = FLOAT_MAX;

    for (auto& pair : broadPhaseCollisions)
    {
        RenderObject* ro = pair.second;
        if (std::find(std::begin(ignoreObjects), std::end(ignoreObjects), ro)
            != std::end(ignoreObjects))
        {
            continue;
        }

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
                GeometryOctTree* geometryOctTree = geo->GetGeometryOctTree();
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

void QuadTree::Update()
{
    DVASSERT(worldInitialized);
    int32 count = 0;
    for (List<int32>::iterator it = dirtyZNodes.begin(), e = dirtyZNodes.end(); (it != e) && (count < RECALCULATE_Z_PER_FRAME); ++count)
    {
        RecalculateNodeZLimits(*it);
        it = dirtyZNodes.erase(it);
    }
    count = 0;
    for (List<RenderObject *>::iterator it = dirtyObjects.begin(), e = dirtyObjects.end(); (it != e) && (count < RECALCULATE_OBJECTS_PER_FRAME); ++count)
    {
        RenderObject* object = *it;
        it = dirtyObjects.erase(it);
        //as now invisible render objects are updeted after becoming visible no no need to store them in this list for all that time
        object->RemoveFlag(RenderObject::TREE_NODE_NEED_UPDATE);
        if ((object->GetFlags() & RenderObject::CLIPPING_VISIBILITY_CRITERIA) == RenderObject::CLIPPING_VISIBILITY_CRITERIA)
        {
            uint16 startNode = object->GetTreeNodeIndex();
            if ((!startNode) && (!worldBox.IsInside(object->GetWorldBoundingBox())))
                continue; //object is out of world - leave it in root node

            uint16 targetNode = FindObjectAddNode(startNode, object->GetWorldBoundingBox());
            if (startNode != targetNode)
            {
                //remove from base
                int32 objectsSize = static_cast<int32>(nodes[startNode].objects.size());
                int32 objIndex = 0;
                for (; objIndex < objectsSize; ++objIndex)
                {
                    if (nodes[startNode].objects[objIndex] == object)
                        break;
                }
                DVASSERT(objIndex < objectsSize);
                if (objectsSize > 1)
                    nodes[startNode].objects[objIndex] = nodes[startNode].objects[objectsSize - 1];
                nodes[startNode].objects.resize(objectsSize - 1);
                //and add to target
                nodes[targetNode].objects.push_back(object);
                object->SetTreeNodeIndex(targetNode);
            }
        }
    }
}

void QuadTree::DebugDraw(const Matrix4& cameraMatrix, RenderHelper* renderHelper)
{
#if (DAVA_DEBUG_DRAW_OCTREE)
    if (!worldInitialized)
        return;

    if (debugDrawStateHandle == InvalidUniqueHandle) //create debug draw state
    {
        RenderStateData debugStateData;
        debugStateData.state = RenderStateData::STATE_BLEND | RenderStateData::STATE_COLORMASK_ALL | RenderStateData::STATE_DEPTH_TEST;
        debugStateData.cullMode = FACE_BACK;
        debugStateData.depthFunc = CMP_LESS;
        debugStateData.sourceFactor = BLEND_SRC_ALPHA;
        debugStateData.destFactor = BLEND_ONE_MINUS_SRC_ALPHA;
        debugStateData.fillMode = FILLMODE_SOLID;
        debugDrawStateHandle = RenderManager::Instance()->CreateRenderState(debugStateData);
    }

    Renderer::GetDynamicBindings().SetDynamicParam(PARAM_WORLD, &Matrix4::IDENTITY, (pointer_size)&Matrix4::IDENTITY);
    RenderSystem2D::Instance()->SetColor(0.2f, 1.0f, 0.2f, 1.0f);
    DebugDrawNode(0);
    RenderSystem2D::Instance()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
#endif
}

void QuadTree::DebugDrawNode(uint16 nodeId)
{
#if (DAVA_DEBUG_DRAW_OCTREE)
    RenderSystem2D::Instance()->SetColor(0.2f, 0.2f, 1.0f, 1.0f);
    for (int32 i = 0, size = static_cast<int32>(nodes[nodeId].objects.size()); i < size; ++i)
    {
        RenderHelper::Instance()->DrawBox(nodes[nodeId].objects[i]->GetWorldBoundingBox(), 1.0f, debugDrawStateHandle);
    }
    RenderSystem2D::Instance()->SetColor(0.2f, 1.0f, 0.2f, 1.0f);
    RenderHelper::Instance()->DrawBox(nodes[nodeId].bbox, 1.0f, debugDrawStateHandle);
    for (int32 i = 0; i < QuadTreeNode::NODE_NONE; ++i)
    {
        if (nodes[nodeId].children[i] != INVALID_TREE_NODE_INDEX)
        {
            DebugDrawNode(nodes[nodeId].children[i]);
        }
    }
#endif
}

} //namespace DAVA
