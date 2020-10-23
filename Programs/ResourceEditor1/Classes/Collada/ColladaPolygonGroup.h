#ifndef __COLLADALOADER_COLLADAPOLYGONGROUP_H__
#define __COLLADALOADER_COLLADAPOLYGONGROUP_H__

#include "ColladaIncludes.h"
#include "Base/BaseMath.h"
#include "ColladaMaterial.h"

namespace DAVA
{
class ColladaMesh;

struct ColladaVertex
{
    const static uint32 COLLADA_MAX_JOINT_WEIGHTS = 8;

    ColladaVertex()
    {
        memset(joint, 0, sizeof(joint));
        memset(weight, 0, sizeof(weight));
    }

    static bool IsEqual(const ColladaVertex& v1, const ColladaVertex& c2, int32 vertexFormat);

    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    Vector3 binormal;
    Vector2 texCoords[4];

    int32 jointCount = 0;
    int32 joint[COLLADA_MAX_JOINT_WEIGHTS];
    float32 weight[COLLADA_MAX_JOINT_WEIGHTS];
};

struct ColladaSortVertex
{
    float sortValue;
    int vertexIndex;
    int faceIndex;
    int pointInFaceIndex;
};

struct ColladaVertexWeight
{
    ColladaVertexWeight()
    {
        memset(jointArray, 0, sizeof(jointArray));
        memset(weightArray, 0, sizeof(weightArray));
    }

    void AddWeight(int32 jointI, float32 weight);
    void Normalize();

    int32 jointCount = 0;
    int32 jointArray[10];
    float32 weightArray[10];
};

class ColladaPolygonGroup
{
public:
    ColladaPolygonGroup(ColladaMesh* _mesh, FCDGeometryPolygons* _polygons, ColladaVertexWeight* wertexWeightArray, uint32 maxVertexInfluence);
    ~ColladaPolygonGroup();

    void Render(ColladaMaterial* material);

    inline fstring GetMaterialSemantic()
    {
        return materialSemantic;
    };
    inline int GetTriangleCount()
    {
        return triangleCount;
    };
    inline std::vector<ColladaVertex>& GetVertices()
    {
        return unoptimizedVerteces;
    }
    inline std::vector<int>& GetIndices()
    {
        return indexArray;
    };
    inline uint32 GetVertexFormat()
    {
        return vertexFormat;
    };

    std::vector<ColladaVertex> unoptimizedVerteces;
    std::vector<ColladaVertex> skinVerteces;

    ColladaMesh* parentMesh;

    uint32 maxVertexInfluenceCount;

protected:
    bool skinned;

    FCDGeometryPolygons* polygons;

    int triangleCount;

    void RenderMesh();

    std::vector<int> indexArray;

    int renderListId;
    AABBox3 bbox;

    fstring materialSemantic;
    uint32 vertexFormat;
};
};

#endif // __COLLADALOADER_COLLADAPOLYGONGROUP_H__
