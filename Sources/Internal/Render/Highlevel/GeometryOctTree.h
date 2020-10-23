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


#pragma once

#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Render/3D/PolygonGroup.h"
#include <bitset>

namespace DAVA
{
class RenderHelper;

struct GeometryOctTreeNode
{
    uint64 reserved : 6;
    uint64 childrenPosition : 16;
    uint64 leafDataLocation : 32;
    uint64 isLeaf : 1;
    uint64 children : 8;
    uint64 isPointer : 1;

    inline bool IsChildAvailable(uint32 index)
    {
        return ((children >> index) & 1) != 0;
    }
};

class GeometryOctTree
{
public:
    struct Triangle
    {
        Vector3 v1;
        Vector3 v2;
        Vector3 v3;
        Triangle(const Vector3& vx1, const Vector3& vx2, const Vector3& vx3)
            : v1(vx1)
            , v2(vx2)
            , v3(vx3)
        {
        }
    };
    const Vector<Triangle>& GetDebugTriangles() const;
    const Vector<AABBox3>& GetDebugBoxes() const;

    void CleanDebugTriangles();
    void AddDebugTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3);

public:
    const uint32 MIN_TRIANGLES_IN_LEAF = 20;

    void BuildTree(PolygonGroup* geometry);
    void DebugDraw(const Matrix4& worldMatrix, uint32 flags, RenderHelper* renderHelper);
    bool IntersectionWithRay(const Ray3Optimized& ray, float32& result, uint32& resultTriIndex);
    bool IntersectionWithRay2(const Ray3Optimized& ray, float32& result, uint32& resultTriIndex);

    void GetTrianglesInBox(const AABBox3& searchBox, Vector<uint16>& resultTriangles);

    uint32 GetAllocatedMemorySize();

private:
    uint32 BuildTreeRecursive(PolygonGroup* geometry, uint32 nodeIndex, const AABBox3& boundingBox, const Vector<uint16>& triangles, uint32 level, uint32 topLevelTriangles);

    void DebugDrawRecursive(const Matrix4& worldMatrix, uint32 nodeIndex, const AABBox3& boundingBox, RenderHelper* renderHelper);
    bool RayCastRecursive(const Ray3Optimized& ray, uint32 nodeIndex, const AABBox3& boundingBox, float32 currentBoxT, float32& result, uint32& resultTriIndex);

    bool RayCastRecursive2(const Ray3Optimized& ray, uint32 nodeIndex, const AABBox3& boundingBox,
                           float32 tx0, float32 ty0, float32 tz0, float32 tx1, float32 ty1, float32 tz1, uint32 a,
                           float32& result, uint32& resultTriIndex);

    int32 GetFirstNode(float32 tx0, float32 ty0, float32 tz0, float32 txm, float32 tym, float32 tzm);
    int32 GetNewNode(float32 txm, int32 x, float32 tym, int32 y, float32 tzm, int32 z);

    inline uint32 GetIndex(uint32 xdiv, uint32 ydiv, uint32 zdiv) const;
    inline AABBox3 GetChildBox(const AABBox3& parentBox, uint32 childNodeIndex) const;

    void RecGetTrianglesInBox(const AABBox3& searchBBox, uint32 nodeIndex, const AABBox3& boundingBox, Vector<uint16>& resultTriangles, bool isFullyInside);

private:
    Vector<Triangle> debugTriangles;
    Vector<AABBox3> debugBoxes;
    Vector<GeometryOctTreeNode> nodes;
    Vector<Vector<uint16>> leafs;
    uint32 nextFreeIndex = 0;
    PolygonGroup* geometry = nullptr;
};

inline uint32 GeometryOctTree::GetIndex(uint32 xdiv, uint32 ydiv, uint32 zdiv) const
{
    return zdiv + (ydiv * 2) + (xdiv * 4);
}

inline AABBox3 GeometryOctTree::GetChildBox(const AABBox3& parentBox, uint32 childNodeIndex) const
{
    Vector3 boundingBoxCenter = parentBox.min + parentBox.GetSize() / 2.0f;

    uint32 xdiv = (childNodeIndex >> 2) & 1;
    uint32 ydiv = (childNodeIndex >> 1) & 1;
    uint32 zdiv = (childNodeIndex)&1;

    Vector3 childBoxMin(
    (xdiv == 0) ? (parentBox.min.x) : (boundingBoxCenter.x),
    (ydiv == 0) ? (parentBox.min.y) : (boundingBoxCenter.y),
    (zdiv == 0) ? (parentBox.min.z) : (boundingBoxCenter.z));

    Vector3 childBoxMax(
    (xdiv == 0) ? (boundingBoxCenter.x) : (parentBox.max.x),
    (ydiv == 0) ? (boundingBoxCenter.y) : (parentBox.max.y),
    (zdiv == 0) ? (boundingBoxCenter.z) : (parentBox.max.z));

    return AABBox3(childBoxMin, childBoxMax);
}

inline const Vector<GeometryOctTree::Triangle>& GeometryOctTree::GetDebugTriangles() const
{
    return debugTriangles;
}

inline void GeometryOctTree::AddDebugTriangle(const Vector3& v1, const Vector3& v2, const Vector3& v3)
{
    debugTriangles.emplace_back(v1, v2, v3);
}

inline void GeometryOctTree::CleanDebugTriangles()
{
    debugTriangles.clear();
}

inline const Vector<AABBox3>& GeometryOctTree::GetDebugBoxes() const
{
    return debugBoxes;
}
};
