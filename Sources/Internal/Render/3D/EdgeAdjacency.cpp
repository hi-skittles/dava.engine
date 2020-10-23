#include "EdgeAdjacency.h"
#include "PolygonGroup.h"
#include "Render/RenderBase.h"
#include "Math/Math2D.h"

namespace DAVA
{
bool EdgeAdjacency::Edge::IsEqual(const Edge& otherEdge)
{
    if (EdgeAdjacency::IsPointsEqual(points[0], otherEdge.points[0]) && EdgeAdjacency::IsPointsEqual(points[1], otherEdge.points[1]))
    {
        return true;
    }
    else if (EdgeAdjacency::IsPointsEqual(points[0], otherEdge.points[1]) && EdgeAdjacency::IsPointsEqual(points[1], otherEdge.points[0]))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool EdgeAdjacency::IsPointsEqual(const Vector3& p0, const Vector3& p1)
{
    if (FLOAT_EQUAL(p0.x, p1.x) && FLOAT_EQUAL(p0.y, p1.y) && FLOAT_EQUAL(p0.z, p1.z))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void EdgeAdjacency::InitFromPolygonGroup(PolygonGroup* _polygonGroup, int32 indexCount)
{
    polygonGroup = _polygonGroup;

#ifdef __DAVAENGINE_DEBUG__
    DVASSERT(rhi::PRIMITIVE_TRIANGLELIST == polygonGroup->GetPrimitiveType());
#endif

    for (int32 i = 0; i < indexCount; i += 3)
    {
        CreateTriangle(i);
    }
}

void EdgeAdjacency::CreateTriangle(int32 startingI)
{
    int32 i = startingI;

    int32 i0, i1, i2;
    polygonGroup->GetIndex(i, i0);
    polygonGroup->GetIndex(i + 1, i1);
    polygonGroup->GetIndex(i + 2, i2);

    Vector3 p0, p1, p2;
    polygonGroup->GetCoord(i0, p0);
    polygonGroup->GetCoord(i1, p1);
    polygonGroup->GetCoord(i2, p2);

    Edge edge0;
    FillEdge(edge0, i0, i1);
    int32 edgeIndex0 = GetEdgeIndex(edge0);

    Edge edge1;
    FillEdge(edge1, i1, i2);
    int32 edgeIndex1 = GetEdgeIndex(edge1);

    Edge edge2;
    FillEdge(edge2, i2, i0);
    int32 edgeIndex2 = GetEdgeIndex(edge2);

    TriangleData triangleData;
    triangleData.i0 = i0;
    triangleData.i1 = i1;
    triangleData.i2 = i2;

    if (edges[edgeIndex0].sharedTriangles.size() < 2)
    {
        edges[edgeIndex0].sharedTriangles.push_back(triangleData);
    }
    if (edges[edgeIndex1].sharedTriangles.size() < 2)
    {
        edges[edgeIndex1].sharedTriangles.push_back(triangleData);
    }
    if (edges[edgeIndex2].sharedTriangles.size() < 2)
    {
        edges[edgeIndex2].sharedTriangles.push_back(triangleData);
    }
}

void EdgeAdjacency::FillEdge(Edge& edge, int32 index0, int32 index1)
{
    Vector3 position;

    polygonGroup->GetCoord(index0, position);
    edge.points[0] = position;

    polygonGroup->GetCoord(index1, position);
    edge.points[1] = position;
}

int32 EdgeAdjacency::GetEdgeIndex(Edge& edge)
{
    int32 edgesCount = static_cast<int32>(edges.size());
    for (int32 i = 0; i < edgesCount; ++i)
    {
        if (edges[i].IsEqual(edge))
        {
            return i;
        }
    }

    edges.push_back(edge);

    return edgesCount;
}

Vector<EdgeAdjacency::Edge>& EdgeAdjacency::GetEdges()
{
    return edges;
}

int32 EdgeAdjacency::GetEdgesWithTwoTrianglesCount()
{
    int32 ret = 0;

    int32 size = static_cast<int32>(edges.size());
    for (int32 i = 0; i < size; ++i)
    {
        if (edges[i].sharedTriangles.size() == 2)
        {
            ret++;
        }
    }

    return ret;
}
};
