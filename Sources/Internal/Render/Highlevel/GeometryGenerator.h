#ifndef __DAVAENGINE_GEOMETRY_GENERATOR_H__
#define __DAVAENGINE_GEOMETRY_GENERATOR_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"
#include "Render/3D/PolygonGroup.h"

namespace DAVA
{
namespace GeometryGenerator
{
enum eGeometryBasicPrimitive
{
    TYPE_BOX,
    TYPE_ICO_SPHERE,
    TYPE_UV_SPHERE,
    TYPE_TORUS,
    TYPE_CYLINDER,
};

struct GeometryGenOptions
{
    uint32 textureCoordsChannels = 1;
    Vector2 textureCoordsMax[4];

    bool generateNormals = true;
    bool generateTangents = true;
    bool generateBinormals = true;
    bool flipNormals = false;

    GeometryGenOptions()
    {
        textureCoordsMax[0] = Vector2(1.0f, 1.0f);
        textureCoordsMax[1] = Vector2(1.0f, 1.0f);
        textureCoordsMax[2] = Vector2(1.0f, 1.0f);
        textureCoordsMax[3] = Vector2(1.0f, 1.0f);
    }
};

PolygonGroup* GenerateUVSphere(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions = GeometryGenOptions());
PolygonGroup* GenerateIcoSphere(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions = GeometryGenOptions());
PolygonGroup* GenerateBox(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions = GeometryGenOptions());
PolygonGroup* GenerateTorus(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions = GeometryGenOptions());
PolygonGroup* GenerateCylinder(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions = GeometryGenOptions());

PolygonGroup* GenerateGeometry(eGeometryBasicPrimitive geometryType, const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions = GeometryGenOptions());
};
}

#endif // __DAVAENGINE_GEOMETRY_GENERATOR_H__
