#include "stdafx.h"
#include "ColladaMesh.h"

namespace DAVA
{
ColladaMesh::ColladaMesh(FCDGeometryMesh* _mesh, ColladaVertexWeight* vertexWeightArray, uint32 maxVertexInfluence)
    : mesh(_mesh)
{
    name = _mesh->GetDaeId();
    printf("- mesh: %s\n", mesh->GetDaeId().c_str());

    if (!mesh->IsTriangles())
        FCDGeometryPolygonsTools::Triangulate(mesh);

    int polygonsCount = (int)mesh->GetPolygonsCount();
    for (int p = 0; p < polygonsCount; ++p)
    {
        FCDGeometryPolygons* poly = mesh->GetPolygons(p);
        ColladaPolygonGroup* polyGroup = new ColladaPolygonGroup(this, poly, vertexWeightArray, maxVertexInfluence);
        polygons.push_back(polyGroup);

        printf("- polygroup: %p\n", polyGroup);
    }
}

ColladaMesh::~ColladaMesh()
{
}
};
