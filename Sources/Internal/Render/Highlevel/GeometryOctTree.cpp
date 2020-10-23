/*==================================================================================
	Copyright (c) 2008, binaryzebra
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.
	* Neither the name of the binaryzebra nor the
	names of its contributors may be used to endorse or promote products
	derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "Render/Highlevel/Light.h"
#include "Render/RenderHelper.h"
#include "Scene3D/Scene.h"
#include "Render/Highlevel/GeometryOctTree.h"
#include "Render/RenderHelper.h"
#include "Base/DynamicBitset.h"
#include "Logger/Logger.h"
#include "Utils/StringFormat.h"

#define OCTREE_CHECK_TRIANGLES 0

namespace DAVA
{
// Implementation

static uint32 counter = 0;

void GeometryOctTree::BuildTree(PolygonGroup* _geometry)
{
    geometry = _geometry;

    uint32 trianglesCount = static_cast<uint32>(geometry->GetIndexCount() / 3);
    Vector<uint16> triangles(trianglesCount);
    for (uint32 triangle = 0; triangle < trianglesCount; ++triangle)
        triangles[triangle] = static_cast<uint16>(triangle);

    nodes.resize(16);
    nextFreeIndex = 1; // count 0 index already busy for root Node
    uint32 maxLevel = BuildTreeRecursive(geometry, 0, geometry->GetBoundingBox(), triangles, 0, static_cast<uint32>(triangles.size()));

#if (OCTREE_CHECK_TRIANGLES)
    nodes.shrink_to_fit();
    leafs.shrink_to_fit();

    DVASSERT(leafs.size() != 0);

    float32 avgTriangleCount = 0.0f;
    uint32 minTriangleCount = static_cast<uint32>(leafs[0].size());
    uint32 maxTriangleCount = static_cast<uint32>(leafs[0].size());

    for (size_t k = 0; k < leafs.size(); ++k)
    {
        avgTriangleCount += (float32)leafs[k].size();
        minTriangleCount = Min((uint32)leafs[k].size(), minTriangleCount);
        maxTriangleCount = Max((uint32)leafs[k].size(), maxTriangleCount);
    }

    avgTriangleCount /= (float32)leafs.size();

    Map<uint16, uint32> overlapCount;
    for (uint16 triangle = 0; triangle < static_cast<uint16>(geometry->GetIndexCount()) / 3; ++triangle)
    {
        overlapCount[triangle] = 0;
    }

    for (auto& leaf : leafs)
    {
        for (uint16& index : leaf)
        {
            overlapCount[index]++;
        }
    }

    for (uint16 triangle = 0; triangle < static_cast<uint16>(geometry->GetIndexCount()) / 3; ++triangle)
    {
        // DVASSERT(overlapCount[triangle] != 0); // triangle should be at least in one leaf.
        if (overlapCount[triangle] == 0)
        {
            Logger::FrameworkDebug("Strange Triangle: %d", triangle);
        }
    }

    uint32 oneCount = 0;
    uint32 allCount = 0;
    Map<uint32, uint32> gistogram;
    for (auto& pair : overlapCount)
    {
        uint16 index = pair.first;
        uint32 count = pair.second;

        allCount++;
        gistogram[count]++;
    }

    DVASSERT((allCount) == geometry->GetIndexCount() / 3);

    float32 triangleDistribution1 = (float32)gistogram[1] / (float32)(allCount)*100.0f;
    float32 triangleDistribution2 = (float32)gistogram[2] / (float32)(allCount)*100.0f;
    float32 triangleDistribution3 = (float32)gistogram[3] / (float32)(allCount)*100.0f;
    Logger::Debug(Format("Node info: triangles: %d, count: %d, maxLevel: %d", triangles.size(), nodes.size(), maxLevel).c_str());
    Logger::Debug(Format("Leaf info: min: %d, max: %d, avg: %f d1: %f d2: %f d3: %f", minTriangleCount, maxTriangleCount, avgTriangleCount, triangleDistribution1, triangleDistribution2, triangleDistribution3).c_str());
    Logger::Debug(Format("Index: %d", counter++).c_str());
#endif
}

uint32 GeometryOctTree::GetAllocatedMemorySize()
{
    uint32 size = 0;
    size += static_cast<uint32>(nodes.size() * sizeof(GeometryOctTreeNode));
    for (auto& vector : leafs)
        size += static_cast<uint32>(vector.size() * sizeof(uint16));
    return size;
}

uint32 GeometryOctTree::BuildTreeRecursive(PolygonGroup* geometry, uint32 nodeIndex, const AABBox3& boundingBox, const Vector<uint16>& triangles, uint32 level, uint32 topLevelTriangles)
{
    uint32 maxLevel = level;

    if (nextFreeIndex >= nodes.size())
        nodes.resize(nodes.size() * 2);

    Vector3 halfBox = boundingBox.GetSize() / 2.0f;

    bool childExceedsParent = (level > 0) && (triangles.size() >= topLevelTriangles);
    float vol = 8.0f * std::abs(halfBox.x * halfBox.y * halfBox.z);
    const float minVolume = 0.1f * 0.1f * 0.1f;
    if ((triangles.size() < MIN_TRIANGLES_IN_LEAF) || (level > 10) || childExceedsParent || (vol <= minVolume))
    {
        DVASSERT(triangles.size() != 0);

        uint32 leafIndex = static_cast<uint32>(leafs.size());
        leafs.push_back(triangles);

        GeometryOctTreeNode& currentNode = nodes[nodeIndex];
        currentNode.leafDataLocation = leafIndex;
        currentNode.isLeaf = 1;
        return level;
    }

    Vector<uint16> childrenTriangles[8];
    for (uint32_t i = 0; i < 8; ++i)
        childrenTriangles[i].reserve(triangles.size());

    uint32 childrenBitmask = 0;
    uint32 nonEmptyCount = 0;

    Vector3 boundingBoxCenter = boundingBox.GetCenter();

    float minX[2] = { boundingBox.min.x, boundingBoxCenter.x };
    float minY[2] = { boundingBox.min.y, boundingBoxCenter.y };
    float minZ[2] = { boundingBox.min.z, boundingBoxCenter.z };
    float maxX[2] = { boundingBoxCenter.x, boundingBox.max.x };
    float maxY[2] = { boundingBoxCenter.y, boundingBox.max.y };
    float maxZ[2] = { boundingBoxCenter.z, boundingBox.max.z };
  
#if (OCTREE_CHECK_TRIANGLES)
    Vector<bool> checkBitset(triangles.size(), false);
#endif

    AABBox3 childrenBoxes[8];
    Vector3 childBoxMin;
    Vector3 childBoxMax;

    uint32 k = 0;
    uint32 triangleCount = static_cast<uint32>(triangles.size());
    for (uint32 xdiv = 0; xdiv < 2; ++xdiv)
    {
        childBoxMin.x = minX[xdiv];
        childBoxMax.x = maxX[xdiv];
        for (uint32 ydiv = 0; ydiv < 2; ++ydiv)
        {
            childBoxMin.y = minY[ydiv];
            childBoxMax.y = maxY[ydiv];
            for (uint32 zdiv = 0; zdiv < 2; ++zdiv)
            {
                childBoxMin.z = minZ[zdiv];
                childBoxMax.z = maxZ[zdiv];

                childrenBoxes[k].min = childBoxMin;
                childrenBoxes[k].max = childBoxMax;

                for (uint16 triangleIndex : triangles)
                {
                    uint16 ptIndex[3];
                    geometry->GetTriangleIndices(3 * triangleIndex, ptIndex);

                    Vector3 ptCoord[3];
                    geometry->GetCoord(ptIndex[0], ptCoord[0]);
                    geometry->GetCoord(ptIndex[1], ptCoord[1]);
                    geometry->GetCoord(ptIndex[2], ptCoord[2]);

                    if (Intersection::BoxTriangle(childrenBoxes[k], ptCoord[0], ptCoord[1], ptCoord[2]))
                        childrenTriangles[k].emplace_back(triangleIndex);
                }

                if (childrenTriangles[k].size() > 0)
                {
                    nonEmptyCount++;
                    uint32 childBitIndex = GetIndex(xdiv, ydiv, zdiv);
                    childrenBitmask |= (1 << childBitIndex);
                }

                ++k;
            }
        }
    }

#if (OCTREE_CHECK_TRIANGLES)
    static uint32 problemTriangleIndex = 0;

    bool isProblemTriangle = false;
    for (uint32 triIndex = 0; triIndex < (uint32)triangles.size(); ++triIndex)
    {
        //DVASSERT(checkBitset.At(triIndex) == true);
        if (!checkBitset.at(triIndex))
        {
            uint32 triangleIndex = triangles[triIndex];
            int32 ptIndex[3];
            Vector3 ptCoord[3];

            geometry->GetIndex(triangleIndex * 3 + 0, ptIndex[0]);
            geometry->GetIndex(triangleIndex * 3 + 1, ptIndex[1]);
            geometry->GetIndex(triangleIndex * 3 + 2, ptIndex[2]);

            geometry->GetCoord(ptIndex[0], ptCoord[0]);
            geometry->GetCoord(ptIndex[1], ptCoord[1]);
            geometry->GetCoord(ptIndex[2], ptCoord[2]);

            bool isInsideMainBox = Intersection::BoxTriangle(boundingBox, ptCoord[0], ptCoord[1], ptCoord[2]);

            Logger::FrameworkDebug("Parent box((%03.9f, %03.9f, %03.9f), (%03.9f, %03.9f, %03.9f)) - %s",
                                   boundingBox.min.x, boundingBox.min.y, boundingBox.min.z,
                                   boundingBox.max.x, boundingBox.max.y, boundingBox.max.z,
                                   (isInsideMainBox) ? "intersects" : "no intersects");

            const uint32 PROBLEM_TRIANGLE_INDEX = 1;

            if (problemTriangleIndex == PROBLEM_TRIANGLE_INDEX)
                debugBoxes.emplace_back(boundingBox);

            for (uint32 xdiv = 0; xdiv < 2; ++xdiv)
            {
                for (uint32 ydiv = 0; ydiv < 2; ++ydiv)
                {
                    for (uint32 zdiv = 0; zdiv < 2; ++zdiv)
                    {
                        const AABBox3& childBox = childrenBoxes[xdiv][ydiv][zdiv];

                        if (problemTriangleIndex == PROBLEM_TRIANGLE_INDEX)
                            debugBoxes.emplace_back(childBox);

                        bool isInsideChildBox = Intersection::BoxTriangle(childrenBoxes[xdiv][ydiv][zdiv], ptCoord[0], ptCoord[1], ptCoord[2]);
                        Logger::FrameworkDebug("AABBox box((%03.9f, %03.9f, %03.9f), (%03.9f, %03.9f, %03.9f)) - %s",
                                               childBox.min.x, childBox.min.y, childBox.min.z,
                                               childBox.max.x, childBox.max.y, childBox.max.z,
                                               (isInsideChildBox) ? "intersects" : "no intersects");
                    }
                }
            }

            Logger::FrameworkDebug("Triangle :((%03.9f, %03.9f, %03.9f), (%03.9f, %03.9f, %03.9f), (%03.9f, %03.9f, %03.9f)) : %d",
                                   ptCoord[0].x, ptCoord[0].y, ptCoord[0].z,
                                   ptCoord[1].x, ptCoord[1].y, ptCoord[1].z,
                                   ptCoord[2].x, ptCoord[2].y, ptCoord[2].z,
                                   triangleIndex);
            isProblemTriangle = true;

            if (problemTriangleIndex == PROBLEM_TRIANGLE_INDEX)
            {
                AddDebugTriangle(ptCoord[0], ptCoord[1], ptCoord[2]);

                bool im = Intersection::BoxTriangle(boundingBox, ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i0 = Intersection::BoxTriangle(childrenBoxes[0][0][0], ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i1 = Intersection::BoxTriangle(childrenBoxes[0][0][1], ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i2 = Intersection::BoxTriangle(childrenBoxes[0][1][0], ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i3 = Intersection::BoxTriangle(childrenBoxes[0][1][1], ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i4 = Intersection::BoxTriangle(childrenBoxes[1][0][0], ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i5 = Intersection::BoxTriangle(childrenBoxes[1][0][1], ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i6 = Intersection::BoxTriangle(childrenBoxes[1][1][0], ptCoord[0], ptCoord[1], ptCoord[2]);
                bool i7 = Intersection::BoxTriangle(childrenBoxes[1][1][1], ptCoord[0], ptCoord[1], ptCoord[2]);
            }
            problemTriangleIndex++;
        }
    }
#endif

    // Do not create tree if min triangle count reached and we have minimum triangle number already
    if ((nonEmptyCount == 1) && (triangles.size() < MIN_TRIANGLES_IN_LEAF))
    {
        uint32 leafIndex = static_cast<uint32>(leafs.size());
        leafs.push_back(triangles);

        GeometryOctTreeNode& currentNode = nodes[nodeIndex];
        currentNode.leafDataLocation = leafIndex;
        currentNode.isLeaf = 1;
        return level;
    }

    uint32 saveFreeIndex = nextFreeIndex;
    nextFreeIndex += nonEmptyCount;
    uint32 childIndex = 0;

    k = 0;
    for (uint32 xdiv = 0; xdiv < 2; ++xdiv)
    {
        for (uint32 ydiv = 0; ydiv < 2; ++ydiv)
        {
            for (uint32 zdiv = 0; zdiv < 2; ++zdiv)
            {
                const Vector<uint16>& childTriangles = childrenTriangles[k];
                if (!childTriangles.empty())
                {
                    uint32 childNodeAbsIndex = saveFreeIndex + childIndex;
                    uint32 childLevel = BuildTreeRecursive(geometry, childNodeAbsIndex, childrenBoxes[k], childTriangles, level + 1, static_cast<uint32>(triangles.size()));
                    maxLevel = Max(childLevel, maxLevel);
                    childIndex++;
                }
                ++k;
            }
        }
    }

    GeometryOctTreeNode& currentNode = nodes[nodeIndex];
    DVASSERT(((saveFreeIndex - nodeIndex) < 256 * 256) && ((saveFreeIndex - nodeIndex) > 0));
    currentNode.childrenPosition = saveFreeIndex - nodeIndex; // store shift to childnode
    currentNode.children = childrenBitmask;

    int checkNonEmptyCount = 0;
    for (int i = 0; i < 8; ++i)
    {
        if ((childrenBitmask >> i) & 1)
            checkNonEmptyCount++;
    }
    DVASSERT(checkNonEmptyCount == nonEmptyCount);
    return maxLevel;
}

void GeometryOctTree::GetTrianglesInBox(const AABBox3& searchBBox, Vector<uint16>& resultTriangles)
{
    const AABBox3& boundingBox = geometry->GetBoundingBox();
    if (Intersection::BoxBox(searchBBox, boundingBox))
    {
        bool isFullyInside = searchBBox.IsInside(boundingBox);
        RecGetTrianglesInBox(searchBBox, 0, boundingBox, resultTriangles, isFullyInside);

        std::sort(resultTriangles.begin(), resultTriangles.end());
        resultTriangles.erase(std::unique(resultTriangles.begin(), resultTriangles.end()), resultTriangles.end());
    }
}

void GeometryOctTree::RecGetTrianglesInBox(const AABBox3& searchBBox, uint32 nodeIndex, const AABBox3& boundingBox, Vector<uint16>& resultTriangles, bool isFullyInside)
{
    DVASSERT(nodeIndex >= 0 && nodeIndex < nodes.size());
    GeometryOctTreeNode& currentNode = nodes[nodeIndex];

    if (currentNode.isLeaf)
    {
        Vector<uint16>& triangles = leafs[currentNode.leafDataLocation];
        if (isFullyInside)
        {
            resultTriangles.insert(resultTriangles.end(), triangles.begin(), triangles.end());
        }
        else
        {
            for (uint16 triangleIndex : triangles)
            {
                int32 ptIndex[3];
                Vector3 ptCoord[3];
                geometry->GetIndex(triangleIndex * 3 + 0, ptIndex[0]);
                geometry->GetIndex(triangleIndex * 3 + 1, ptIndex[1]);
                geometry->GetIndex(triangleIndex * 3 + 2, ptIndex[2]);
                geometry->GetCoord(ptIndex[0], ptCoord[0]);
                geometry->GetCoord(ptIndex[1], ptCoord[1]);
                geometry->GetCoord(ptIndex[2], ptCoord[2]);
                if (Intersection::BoxTriangle(searchBBox, ptCoord[0], ptCoord[1], ptCoord[2]))
                {
                    resultTriangles.emplace_back(triangleIndex);
                }
            }
        }
        return;
    }

    Vector3 boundingBoxCenter = boundingBox.GetCenter();
    uint32 count = 0;
    for (uint32 childBitIndex = 0; childBitIndex < 8; ++childBitIndex)
    {
        if ((currentNode.children >> childBitIndex) & 1)
        {
            AABBox3 childBox = GetChildBox(boundingBox, childBitIndex);
            if (Intersection::BoxBox(searchBBox, childBox))
            {
                bool isFullyInside = searchBBox.IsInside(childBox);
                RecGetTrianglesInBox(searchBBox, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox, resultTriangles, isFullyInside);
            }
            count++;
        }
    }
}

void GeometryOctTree::DebugDraw(const Matrix4& worldMatrix, uint32 flags, RenderHelper* renderHelper)
{
    const AABBox3& boundingBox = geometry->GetBoundingBox();
    DebugDrawRecursive(worldMatrix, 0, boundingBox, renderHelper);
}

void GeometryOctTree::DebugDrawRecursive(const Matrix4& worldMatrix, uint32 nodeIndex, const AABBox3& boundingBox, RenderHelper* renderHelper)
{
    DVASSERT(nodeIndex >= 0 && nodeIndex < nodes.size());

    GeometryOctTreeNode& currentNode = nodes[nodeIndex];

    if (nodeIndex == 0)
        renderHelper->DrawAABoxTransformed(boundingBox, worldMatrix, Color(1.0f, 1.0f, 1.0f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);
    else if (currentNode.isLeaf)
        renderHelper->DrawAABoxTransformed(boundingBox, worldMatrix, Color(1.0f, 0.0f, 0.0f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);
    else
        renderHelper->DrawAABoxTransformed(boundingBox, worldMatrix, Color(1.0f, 1.0f, 0.0f, 1.0f), RenderHelper::DRAW_WIRE_DEPTH);

    Vector3 halfBox = boundingBox.GetSize() / 2.0f;

    uint32 count = 0;
    for (uint32 xdiv = 0; xdiv < 2; ++xdiv)
    {
        for (uint32 ydiv = 0; ydiv < 2; ++ydiv)
        {
            for (uint32 zdiv = 0; zdiv < 2; ++zdiv)
            {
                uint32 childBitIndex = GetIndex(xdiv, ydiv, zdiv);

                if ((currentNode.children >> childBitIndex) & 1)
                {
                    Vector3 childBoxMin(boundingBox.min.x + halfBox.x * static_cast<float32>(xdiv),
                                        boundingBox.min.y + halfBox.y * static_cast<float32>(ydiv),
                                        boundingBox.min.z + halfBox.z * static_cast<float32>(zdiv));

                    AABBox3 childBox(childBoxMin, childBoxMin + halfBox);
                    DebugDrawRecursive(worldMatrix, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox, renderHelper);
                    count++;
                }
            }
        }
    }
}

std::queue<uint32> rayCastQueue;

bool GeometryOctTree::IntersectionWithRay(const Ray3Optimized& ray, float32& result, uint32& resultTriIndex)
{
    const AABBox3& boundingBox = geometry->GetBoundingBox();

    float32 boxTMin;
    if (!Intersection::RayBox(ray, boundingBox, boxTMin))
    {
        return false;
    }

    result = std::numeric_limits<float>::max();
    resultTriIndex = -1;
    return RayCastRecursive(ray, 0, boundingBox, boxTMin, result, resultTriIndex);
}

bool GeometryOctTree::IntersectionWithRay2(const Ray3Optimized& ray, float32& result, uint32& resultTriIndex)
{
    /*
	 * The one who will understand how this traversal works can get a cookie.
	 * Based on: An Efficient Parametric Algorithm for Octree Traversal (J. Revelles, C. Urena, M. Lastra)
	 */
    const AABBox3& boundingBox = geometry->GetBoundingBox();
    Vector3 size = boundingBox.GetSize();
    Vector3 rayOrigin = ray.origin;
    Vector3 rayDirection = ray.direction;

    uint32 a = 0;

    // fixes for rays with negative direction
    if (rayDirection.x < 0.0f)
    {
        rayOrigin.x = size.x - rayOrigin.x;
        rayDirection.x = -rayDirection.x;
        a |= 4; //bitwise OR (latest bits are XYZ)
    }
    if (rayDirection.y < 0.0f)
    {
        rayOrigin.y = size.y - rayOrigin.y;
        rayDirection.y = -rayDirection.y;
        a |= 2;
    }
    if (rayDirection.z < 0.0f)
    {
        rayOrigin.z = size.z - rayOrigin.z;
        rayDirection.z = -rayDirection.z;
        a |= 1;
    }

    float32 divx = 1.0f / rayDirection.x; // IEEE stability fix
    float32 divy = 1.0f / rayDirection.y;
    float32 divz = 1.0f / rayDirection.z;

    float32 dx0 = (boundingBox.min.x - rayOrigin.x);
    float32 dx1 = (boundingBox.max.x - rayOrigin.x);

    float32 dy0 = (boundingBox.min.y - rayOrigin.y);
    float32 dy1 = (boundingBox.max.y - rayOrigin.y);

    float32 dz0 = (boundingBox.min.z - rayOrigin.z);
    float32 dz1 = (boundingBox.max.z - rayOrigin.z);

    float32 tx0 = (dx0 != 0.0f) ? (dx0 * divx) : (0.0f);
    float32 tx1 = (dx1 != 0.0f) ? (dx1 * divx) : (0.0f);

    float32 ty0 = (dy0 != 0.0f) ? (dy0 * divy) : (0.0f);
    float32 ty1 = (dy1 != 0.0f) ? (dy1 * divy) : (0.0f);

    float32 tz0 = (dz0 != 0.0f) ? (dz0 * divz) : (0.0f);
    float32 tz1 = (dz1 != 0.0f) ? (dz1 * divz) : (0.0f);

    result = 1.0f;
    resultTriIndex = -1;
    bool isSuccessfull = false;

    float32 m0 = Max(Max(tx0, ty0), tz0);
    float32 m1 = Min(Min(tx1, ty1), tz1);

    if (Intersection::RayBox(ray, boundingBox))
    {
        isSuccessfull = RayCastRecursive2(ray, 0, boundingBox, tx0, ty0, tz0, tx1, ty1, tz1, a, result, resultTriIndex);
    }

    return isSuccessfull;
}

//unsigned char a; // because an unsigned char is 8 bits

int32 GeometryOctTree::GetFirstNode(float32 tx0, float32 ty0, float32 tz0, float32 txm, float32 tym, float32 tzm)
{
    int32 answer = 0; // initialize to 00000000
    // select the entry plane and set bits
    if (tx0 > ty0)
    {
        if (tx0 > tz0)
        { // PLANE YZ
            if (tym < tx0)
                answer |= 2; // set bit at position 1
            if (tzm < tx0)
                answer |= 1; // set bit at position 0
            return answer;
        }
    }
    else
    {
        if (ty0 > tz0)
        { // PLANE XZ
            if (txm < ty0)
                answer |= 4; // set bit at position 2
            if (tzm < ty0)
                answer |= 1; // set bit at position 0
            return answer;
        }
    }
    // PLANE XY
    if (txm < tz0)
        answer |= 4; // set bit at position 2
    if (tym < tz0)
        answer |= 2; // set bit at position 1
    return answer;
}

int32 GeometryOctTree::GetNewNode(float32 txm, int32 x, float32 tym, int32 y, float32 tzm, int32 z)
{
    if (txm < tym)
    {
        if (txm < tzm)
        {
            return x;
        } // YZ plane
    }
    else
    {
        if (tym < tzm)
        {
            return y;
        } // XZ plane
    }
    return z; // XY plane;
}

inline uint32 popcount64c(uint64 x)
{
    /*
     * Taken from https://en.wikipedia.org/wiki/Hamming_weight 
     */
    static const uint64 m1 = 0x5555555555555555; // binary: 0101...
    static const uint64 m2 = 0x3333333333333333; // binary: 00110011..
    static const uint64 m4 = 0x0f0f0f0f0f0f0f0f; // binary:  4 zeros,  4 ones ...
    static const uint64 h01 = 0x0101010101010101; // the sum of 256 to the power of 0,1,2,3...
    x -= (x >> 1) & m1; // put count of each 2 bits into those 2 bits
    x = (x & m2) + ((x >> 2) & m2); // put count of each 4 bits into those 4 bits
    x = (x + (x >> 4)) & m4; // put count of each 8 bits into those 8 bits
    return (x * h01) >> 56; // returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
}

bool GeometryOctTree::RayCastRecursive2(const Ray3Optimized& ray, uint32 nodeIndex, const AABBox3& box, float32 tx0, float32 ty0, float32 tz0, float32 tx1, float32 ty1, float32 tz1, uint32 a, float32& result, uint32& resultTriIndex)
{
    float txm, tym, tzm;
    int32 currNode;

    if (tx1 < 0.0f || ty1 < 0.0f || tz1 < 0.0f)
        return false;

    DVASSERT(nodeIndex >= 0 && nodeIndex < nodes.size());
    bool isIntersection = false;
    GeometryOctTreeNode& currentNode = nodes[nodeIndex];

    if (currentNode.isLeaf)
    {
        Vector<uint16>& triangles = leafs[currentNode.leafDataLocation];
        uint32 triangleCount = static_cast<uint32>(triangles.size());
        for (uint32 k = 0; k < triangleCount; ++k)
        {
            uint32 triangleIndex = triangles[k];

            // TODO: Think about efficient way of non traversing triangles that are in multiple nodes

            int32 ptIndex[3];
            Vector3 ptCoord[3];

            geometry->GetIndex(triangleIndex * 3 + 0, ptIndex[0]);
            geometry->GetIndex(triangleIndex * 3 + 1, ptIndex[1]);
            geometry->GetIndex(triangleIndex * 3 + 2, ptIndex[2]);

            geometry->GetCoord(ptIndex[0], ptCoord[0]);
            geometry->GetCoord(ptIndex[1], ptCoord[1]);
            geometry->GetCoord(ptIndex[2], ptCoord[2]);

            float32 localResultT = 0.0f;
            if (Intersection::RayTriangle(ray, ptCoord[0], ptCoord[1], ptCoord[2], localResultT))
            {
                if (localResultT < result)
                {
                    isIntersection = true;
                    result = localResultT;
                    resultTriIndex = triangleIndex;
                }
            }
        }
        return isIntersection;
    }

    if (std::isinf(tx0) && std::isinf(tx1))
    {
        txm = (ray.origin.x < (box.min.x + box.max.x) / 2.0f) ? (std::numeric_limits<float>::infinity()) : (-std::numeric_limits<float>::infinity());
    }
    else
    {
        txm = 0.5f * (tx0 + tx1);
    }

    if (std::isinf(ty0) && std::isinf(ty1))
    {
        tym = (ray.origin.y < (box.min.y + box.max.y) / 2.0f) ? (std::numeric_limits<float>::infinity()) : (-std::numeric_limits<float>::infinity());
    }
    else
    {
        tym = 0.5f * (ty0 + ty1);
    }

    if (std::isinf(tz0) && std::isinf(tz1))
    {
        tzm = (ray.origin.z < (box.min.z + box.max.z) / 2.0f) ? (std::numeric_limits<float>::infinity()) : (-std::numeric_limits<float>::infinity());
    }
    else
    {
        tzm = 0.5f * (tz0 + tz1);
    }

    currNode = GetFirstNode(tx0, ty0, tz0, txm, tym, tzm);
    do
    {
        bool isIntersection = false;
        switch (currNode)
        {
        case 0:
        {
            uint32 childNodeIndex = a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   tx0, ty0, tz0, txm, tym, tzm, a, result, resultTriIndex);
            }

            currNode = GetNewNode(txm, 4, tym, 2, tzm, 1);
            break;
        }
        case 1:
        {
            uint32 childNodeIndex = 1 ^ a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   tx0, ty0, tzm, txm, tym, tz1, a, result, resultTriIndex);
            }

            currNode = GetNewNode(txm, 5, tym, 3, tz1, 8);
            break;
        }
        case 2:
        {
            uint32 childNodeIndex = 2 ^ a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   tx0, tym, tz0, txm, ty1, tzm, a, result, resultTriIndex);
            }
            currNode = GetNewNode(txm, 6, ty1, 8, tzm, 3);
            break;
        }
        case 3:
        {
            uint32 childNodeIndex = 3 ^ a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   tx0, tym, tzm, txm, ty1, tz1, a, result, resultTriIndex);
            }
            currNode = GetNewNode(txm, 7, ty1, 8, tz1, 8);
            break;
        }
        case 4:
        {
            uint32 childNodeIndex = 4 ^ a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   txm, ty0, tz0, tx1, tym, tzm, a, result, resultTriIndex);
            }
            currNode = GetNewNode(tx1, 8, tym, 6, tzm, 5);
            break;
        }
        case 5:
        {
            uint32 childNodeIndex = 5 ^ a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   txm, ty0, tzm, tx1, tym, tz1, a, result, resultTriIndex);
            }
            currNode = GetNewNode(tx1, 8, tym, 7, tz1, 8);
            break;
        }
        case 6:
        {
            uint32 childNodeIndex = 6 ^ a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   txm, tym, tz0, tx1, ty1, tzm, a, result, resultTriIndex);
            }
            currNode = GetNewNode(tx1, 8, ty1, 8, tzm, 7);
            break;
        }
        case 7:
        {
            uint32 childNodeIndex = 7 ^ a;
            if ((currentNode.children >> childNodeIndex) & 1)
            {
                AABBox3 childBox = GetChildBox(box, childNodeIndex);
                uint32 count = popcount64c(currentNode.children & ((1 << (childNodeIndex + 1)) - 1)) - 1;
                isIntersection = RayCastRecursive2(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox,
                                                   txm, tym, tzm, tx1, ty1, tz1, a, result, resultTriIndex);
            }
            currNode = 8;
            break;
        }
        }

        /*
			if intersection found return fast
		 */
        if (isIntersection)
            return true;

    } while (currNode < 8);

    return false;
}

bool GeometryOctTree::RayCastRecursive(const Ray3Optimized& ray, uint32 nodeIndex, const AABBox3& boundingBox, float32 currentBoxT, float32& result, uint32& resultTriIndex)
{
    DVASSERT(nodeIndex >= 0 && nodeIndex < nodes.size());
    bool isIntersection = false;
    GeometryOctTreeNode& currentNode = nodes[nodeIndex];

    if (currentNode.isLeaf)
    {
        Vector<uint16>& triangles = leafs[currentNode.leafDataLocation];
        uint32 triangleCount = static_cast<uint32>(triangles.size());
        for (uint32 k = 0; k < triangleCount; ++k)
        {
            uint32 triangleIndex = triangles[k];

            // TODO: Think about efficient way of non traversing triangles that are in multiple nodes

            int32 ptIndex[3];
            Vector3 ptCoord[3];

            geometry->GetIndex(triangleIndex * 3 + 0, ptIndex[0]);
            geometry->GetIndex(triangleIndex * 3 + 1, ptIndex[1]);
            geometry->GetIndex(triangleIndex * 3 + 2, ptIndex[2]);

            geometry->GetCoord(ptIndex[0], ptCoord[0]);
            geometry->GetCoord(ptIndex[1], ptCoord[1]);
            geometry->GetCoord(ptIndex[2], ptCoord[2]);

            float32 localResultT = 0.0f;
            if (Intersection::RayTriangle(ray, ptCoord[0], ptCoord[1], ptCoord[2], localResultT))
            {
                if (localResultT < result)
                {
                    isIntersection = true;
                    result = localResultT;
                    resultTriIndex = triangleIndex;
                }
            }
        }
        return isIntersection;
    }

    Vector3 halfBox = boundingBox.GetSize() / 2.0f;

    uint32 count = 0;
    for (uint32 xdiv = 0; xdiv < 2; ++xdiv)
    {
        for (uint32 ydiv = 0; ydiv < 2; ++ydiv)
        {
            for (uint32 zdiv = 0; zdiv < 2; ++zdiv)
            {
                uint32 childBitIndex = GetIndex(xdiv, ydiv, zdiv);

                if ((currentNode.children >> childBitIndex) & 1)
                {
                    Vector3 childBoxMin(boundingBox.min.x + halfBox.x * static_cast<float32>(xdiv),
                                        boundingBox.min.y + halfBox.y * static_cast<float32>(ydiv),
                                        boundingBox.min.z + halfBox.z * static_cast<float32>(zdiv));

                    AABBox3 childBox(childBoxMin, childBoxMin + halfBox);
                    if (Intersection::RayBox(ray, childBox))
                    {
                        float32 boxTMin = 0.0f;
                        isIntersection |= RayCastRecursive(ray, nodeIndex + static_cast<uint32>(currentNode.childrenPosition) + count, childBox, boxTMin, result, resultTriIndex);
                    }
                    count++;
                }
            }
        }
    }

    //DVASSERT(childBitIndexCheck2 == childBitIndexCheck1);

    return isIntersection;
}
};
