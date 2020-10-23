#include "Render/Highlevel/GeometryGenerator.h"
#include "Render/3D/MeshUtils.h"

namespace DAVA
{
namespace GeometryGenerator
{
PolygonGroup* CreatePolygonGroup(uint32 vertexCount, uint32 indexCount, GeometryGenOptions genOptions)
{
    uint32 vertexFormat = EVF_VERTEX;

    if (genOptions.textureCoordsChannels == 1)
        vertexFormat |= EVF_TEXCOORD0;
    if (genOptions.textureCoordsChannels == 2)
        vertexFormat |= EVF_TEXCOORD1;
    if (genOptions.textureCoordsChannels == 3)
        vertexFormat |= EVF_TEXCOORD2;
    if (genOptions.textureCoordsChannels == 4)
        vertexFormat |= EVF_TEXCOORD3;

    if (genOptions.generateNormals)
        vertexFormat |= EVF_NORMAL;
    if (genOptions.generateTangents)
        vertexFormat |= EVF_TANGENT;
    if (genOptions.generateBinormals)
        vertexFormat |= EVF_BINORMAL;

    PolygonGroup* geometry = new PolygonGroup();
    geometry->AllocateData(vertexFormat, vertexCount, indexCount);
    return geometry;
}

PolygonGroup* GenerateUVSphere(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions)
{
    DVASSERT(0, "Not implemented");
    return nullptr;
}

class IcoSphereGenerator
{
public:
    struct Triangle
    {
        uint32 vertex[3];
    };

    static const float32 X;
    static const float32 Z;
    static const float32 N;

    using Lookup = std::map<std::pair<uint32, uint32>, uint32>;

    uint32 VertexForEdge(Lookup& lookup,
                         Vector<Vector3>& vertices, uint32 first, uint32 second)
    {
        Lookup::key_type key(first, second);
        if (key.first > key.second)
            std::swap(key.first, key.second);

        auto inserted = lookup.insert({ key, static_cast<uint32>(vertices.size()) });
        if (inserted.second)
        {
            auto& edge0 = vertices[first];
            auto& edge1 = vertices[second];
            auto point = Normalize(edge0 + edge1);
            vertices.push_back(point);
        }

        return inserted.first->second;
    }

    Vector<Triangle> Subdivide(Vector<Vector3>& vertices,
                               Vector<Triangle> triangles)
    {
        Lookup lookup;
        Vector<Triangle> result;

        for (auto&& each : triangles)
        {
            std::array<uint32, 3> mid;
            for (int edge = 0; edge < 3; ++edge)
            {
                mid[edge] = VertexForEdge(lookup, vertices,
                                          each.vertex[edge], each.vertex[(edge + 1) % 3]);
            }

            result.push_back({ each.vertex[0], mid[0], mid[2] });
            result.push_back({ each.vertex[1], mid[1], mid[0] });
            result.push_back({ each.vertex[2], mid[2], mid[1] });
            result.push_back({ mid[0], mid[1], mid[2] });
        }

        return result;
    }

    void GenerateIcoSphere(uint32 subdivisionsCount, Vector<Vector3>& verticesOut, Vector<Triangle>& trianglesOut)
    {
        const Vector<Vector3> _vertices =
        {
          Vector3(-X, N, Z),
          Vector3(X, N, Z),
          Vector3(-X, N, -Z),
          Vector3(X, N, -Z),
          Vector3(N, Z, X),
          Vector3(N, Z, -X),
          Vector3(N, -Z, X),
          Vector3(N, -Z, -X),
          Vector3(Z, X, N),
          Vector3(-Z, X, N),
          Vector3(Z, -X, N),
          Vector3(-Z, -X, N)
        };

        const Vector<Triangle> _triangles =
        {
          { 1, 4, 0 }, { 4, 9, 0 }, { 4, 5, 9 }, { 8, 5, 4 }, { 1, 8, 4 },
          { 1, 10, 8 },
          { 10, 3, 8 },
          { 8, 3, 5 },
          { 3, 2, 5 },
          { 3, 7, 2 },
          { 3, 10, 7 },
          { 10, 6, 7 },
          { 6, 11, 7 },
          { 6, 0, 11 },
          { 6, 1, 0 },
          { 10, 1, 6 },
          { 11, 0, 9 },
          { 2, 11, 9 },
          { 5, 2, 9 },
          { 11, 2, 7 }

          //            {0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
          //            {8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
          //            {7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
          //            {6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
        };

        Vector<Vector3> vertices = _vertices;
        Vector<Triangle> triangles = _triangles;

        for (uint32 i = 0; i < subdivisionsCount; ++i)
        {
            triangles = Subdivide(vertices, triangles);
        }

        AABBox3 boundingBox;
        for (const Vector3& v : vertices)
        {
            boundingBox.AddPoint(v);
        }

        verticesOut = vertices;
        trianglesOut = triangles;
        return;
    }
};

const float32 IcoSphereGenerator::X = .525731112119133606f;
const float32 IcoSphereGenerator::Z = .850650808352039932f;
const float32 IcoSphereGenerator::N = 0.f;

PolygonGroup* GenerateIcoSphere(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions)
{
    IcoSphereGenerator icosphereGen;
    Vector<Vector3> vertices;
    Vector<IcoSphereGenerator::Triangle> triangles;
    uint32 subdivisionCount = static_cast<uint32>(objectDimentions[FastName("subdivisionCount")]);
    icosphereGen.GenerateIcoSphere(subdivisionCount, vertices, triangles);

    uint32 vertexCount = static_cast<uint32>(vertices.size());
    uint32 indexCount = static_cast<uint32>(triangles.size() * 3);

    PolygonGroup* geometry = CreatePolygonGroup(vertexCount, indexCount, genOptions);

    for (uint32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
    {
        geometry->SetCoord(vertexIndex, boundingBox.min + (vertices[vertexIndex] * 0.5 + 0.5) * boundingBox.GetSize());

        if (genOptions.generateNormals)
        {
            Vector3 normal = Normalize(vertices[vertexIndex]) * ((genOptions.flipNormals) ? (-1.0f) : (1.0f));
            geometry->SetNormal(vertexIndex, normal);
        }

        for (uint32 tc = 0; tc < genOptions.textureCoordsChannels; ++tc)
        {
            Vector3& v = vertices[vertexIndex];
            float theta = acos(v.z / v.Length());
            float phi = atan2(v.x, v.y);
            geometry->SetTexcoord(tc, vertexIndex, Vector2(theta, phi));
        }
    }

    for (uint32 triIndex = 0; triIndex < indexCount / 3; ++triIndex)
    {
        geometry->SetIndex(triIndex * 3 + 0, triangles[triIndex].vertex[0]);
        geometry->SetIndex(triIndex * 3 + 1, triangles[triIndex].vertex[1]);
        geometry->SetIndex(triIndex * 3 + 2, triangles[triIndex].vertex[2]);
    }

    return geometry;
}

PolygonGroup* GenerateBox(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions)
{
    uint32 segmentsOnX = static_cast<uint32>(objectDimentions[FastName("segments.x")]);
    uint32 segmentsOnY = static_cast<uint32>(objectDimentions[FastName("segments.y")]);
    uint32 segmentsOnZ = static_cast<uint32>(objectDimentions[FastName("segments.z")]);

    //
    uint32 vertexCount = (segmentsOnX + 1) * (segmentsOnY + 1) * 2 + (segmentsOnY + 1) * (segmentsOnZ + 1) * 2 + (segmentsOnZ + 1) * (segmentsOnX + 1) * 2;
    uint32 indexCount = (segmentsOnX) * (segmentsOnY)*4 * 3 + (segmentsOnY) * (segmentsOnZ)*4 * 3 + (segmentsOnZ) * (segmentsOnX)*4 * 3;

    PolygonGroup* geometry = CreatePolygonGroup(vertexCount, indexCount, genOptions);

    Vector3 step = boundingBox.GetSize() / Vector3(static_cast<float32>(segmentsOnX), static_cast<float32>(segmentsOnY), static_cast<float32>(segmentsOnZ));

    uint32 order[6][2] = { { 0, 1 }, { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };

    uint32 vertexIndex = 0;
    uint32 indIndex = 0;
    uint32 baseVertex = 0;
    for (uint32 side = 0; side < 2; ++side)
        for (uint32 sx = 0; sx < segmentsOnX + 1; ++sx)
            for (uint32 sy = 0; sy < segmentsOnY + 1; ++sy)
            {
                float32 fsx = static_cast<float32>(sx);
                float32 fsy = static_cast<float32>(sy);
                Vector3 point(
                boundingBox.min.x + fsx * step.x,
                boundingBox.min.y + fsy * step.y,
                (side == 0) ? (boundingBox.min.z) : (boundingBox.max.z));
                geometry->SetCoord(vertexIndex, point);
                if (genOptions.generateNormals)
                {
                    bool flip = (side == 1);
                    if (genOptions.flipNormals)
                        flip = !flip;
                    Vector3 normal = Vector3(0.0f, 0.0f, -1.0f) * ((flip) ? (1.0f) : (-1.0f));
                    geometry->SetNormal(vertexIndex, normal);
                }
                for (uint32 tc = 0; tc < genOptions.textureCoordsChannels; ++tc)
                {
                    geometry->SetTexcoord(tc, vertexIndex, Vector2(fsx / static_cast<float32>(segmentsOnX), fsy / static_cast<float32>(segmentsOnY)));
                }

                vertexIndex++;
                if (sx < segmentsOnX && sy < segmentsOnY)
                {
                    uint32 sideShift = side * (segmentsOnX + 1) * (segmentsOnY + 1);

                    for (uint32 tri = 0; tri < 6; ++tri)
                    {
                        uint32 finalIndex = (side)*tri + (1 - side) * (5 - tri);
                        geometry->SetIndex(indIndex++, baseVertex + sideShift + (sx + order[finalIndex][0]) * (segmentsOnY + 1) + (sy + order[finalIndex][1]));
                    }
                }
            }

    baseVertex += 2 * (segmentsOnX + 1) * (segmentsOnY + 1);

    for (uint32 side = 0; side < 2; ++side)
        for (uint32 sx = 0; sx < segmentsOnX + 1; ++sx)
            for (uint32 sz = 0; sz < segmentsOnZ + 1; ++sz)
            {
                float32 fsx = static_cast<float32>(sx);
                float32 fsz = static_cast<float32>(sz);

                Vector3 point(boundingBox.min.x + static_cast<float32>(sx) * step.x,
                              (side == 0) ? (boundingBox.min.y) : (boundingBox.max.y),
                              boundingBox.min.z + static_cast<float32>(sz) * step.z);
                geometry->SetCoord(vertexIndex, point);

                if (genOptions.generateNormals)
                {
                    bool flip = (side == 1);
                    if (genOptions.flipNormals)
                        flip = !flip;
                    Vector3 normal = Vector3(0.0f, -1.0f, 0.0f) * ((flip) ? (1.0f) : (-1.0f));
                    geometry->SetNormal(vertexIndex, normal);
                }
                for (uint32 tc = 0; tc < genOptions.textureCoordsChannels; ++tc)
                {
                    geometry->SetTexcoord(tc, vertexIndex, Vector2(fsx / static_cast<float32>(segmentsOnX), fsz / static_cast<float32>(segmentsOnZ)));
                }

                vertexIndex++;
                if (sx < segmentsOnX && sz < segmentsOnZ)
                {
                    uint32 sideShift = side * (segmentsOnX + 1) * (segmentsOnZ + 1);

                    for (uint32 tri = 0; tri < 6; ++tri)
                    {
                        uint32 finalIndex = (1 - side) * tri + (side) * (5 - tri);
                        geometry->SetIndex(indIndex++, baseVertex + sideShift + (sx + order[finalIndex][0]) * (segmentsOnZ + 1) + (sz + order[finalIndex][1]));
                    }
                }
            }

    baseVertex += 2 * (segmentsOnX + 1) * (segmentsOnZ + 1);

    for (uint32 side = 0; side < 2; ++side)
        for (uint32 sy = 0; sy < segmentsOnY + 1; ++sy)
            for (uint32 sz = 0; sz < segmentsOnZ + 1; ++sz)
            {
                float32 fsy = static_cast<float32>(sy);
                float32 fsz = static_cast<float32>(sz);
                Vector3 point((side == 0) ? (boundingBox.min.x) : (boundingBox.max.x),
                              boundingBox.min.y + fsy * step.y,
                              boundingBox.min.z + fsz * step.z);
                geometry->SetCoord(vertexIndex, point);
                if (genOptions.generateNormals)
                {
                    bool flip = (side == 1);
                    if (genOptions.flipNormals)
                        flip = !flip;
                    Vector3 normal = Vector3(1.0f, 0.0f, 0.0f) * ((flip) ? (1.0f) : (-1.0f));
                    geometry->SetNormal(vertexIndex, normal);
                }

                for (uint32 tc = 0; tc < genOptions.textureCoordsChannels; ++tc)
                {
                    geometry->SetTexcoord(tc, vertexIndex, Vector2(fsy / static_cast<float32>(segmentsOnY), fsz / static_cast<float32>(segmentsOnZ)));
                }

                vertexIndex++;

                if (sy < segmentsOnY && sz < segmentsOnZ)
                {
                    uint32 sideShift = side * (segmentsOnY + 1) * (segmentsOnZ + 1);

                    for (uint32 tri = 0; tri < 6; ++tri)
                    {
                        uint32 finalIndex = (side)*tri + (1 - side) * (5 - tri);
                        geometry->SetIndex(indIndex++, baseVertex + sideShift + (sy + order[finalIndex][0]) * (segmentsOnZ + 1) + (sz + order[finalIndex][1]));
                    }
                }
            }

    if (genOptions.generateTangents || genOptions.generateBinormals)
    {
        DVASSERT(genOptions.generateTangents == true);
        MeshUtils::RebuildMeshTangentSpace(geometry, genOptions.generateBinormals);
    }
    return geometry;
}

PolygonGroup* GenerateTorus(const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions)
{
    DVASSERT(0, "Not implemented");
    return nullptr;
}

PolygonGroup* GenerateGeometry(eGeometryBasicPrimitive geometryType, const AABBox3& boundingBox, Map<FastName, float32> objectDimentions, GeometryGenOptions genOptions)
{
    switch (geometryType)
    {
    case TYPE_BOX:
        return GenerateBox(boundingBox, objectDimentions, genOptions);
    case TYPE_ICO_SPHERE:
        return GenerateIcoSphere(boundingBox, objectDimentions, genOptions);
    /*/
    case TYPE_UV_SPHERE:
        return GenerateUVSphere(boundingBox, objectDimentions, genOptions);
    case TYPE_TORUS:
        return GenerateTorus(boundingBox, objectDimentions, genOptions);
    case TYPE_CYLINDER:
        return GenerateCylinder(boundingBox, objectDimentions, genOptions);
    // */
    default:
        DVASSERT(0, "Invalid geometry type specified, box will be generated.");
        return GenerateBox(boundingBox, objectDimentions, genOptions);
    };
}
};
};
