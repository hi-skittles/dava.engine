#ifndef __DAVAENGINE_EDGE_ADJACENCY_H__
#define __DAVAENGINE_EDGE_ADJACENCY_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"

namespace DAVA
{
class PolygonGroup;

class EdgeAdjacency
{
public:
    struct TriangleData
    {
        int32 i0;
        int32 i1;
        int32 i2;
    };

    struct Edge
    {
        Vector3 points[2];

        Vector<TriangleData> sharedTriangles;

        bool IsEqual(const Edge& otherEdge);
    };

    void InitFromPolygonGroup(PolygonGroup* polygonGroup, int32 indexCount);

    Vector<Edge>& GetEdges();

    int32 GetEdgesWithTwoTrianglesCount();

    static bool IsPointsEqual(const Vector3& p0, const Vector3& p1);

private:
    PolygonGroup* polygonGroup;

    Vector<Edge> edges;

    void AddEdge(Edge& edge);

    void FillEdge(Edge& edge, int32 index0, int32 index1);
    int32 GetEdgeIndex(Edge& edge);

    void CreateTriangle(int32 startingVertex);
};
};

#endif //__DAVAENGINE_EDGE_ADJACENCY_H__
