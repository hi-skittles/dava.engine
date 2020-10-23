#ifndef __COLLADALOADER_COLLADAMESHINSTANCE_H__
#define __COLLADALOADER_COLLADAMESHINSTANCE_H__

#include "ColladaIncludes.h"
#include "ColladaMesh.h"
#include "ColladaMaterial.h"
#include "ColladaPolygonGroupInstance.h"

namespace DAVA
{
class ColladaSkinnedMesh;
class ColladaMeshInstance
{
public:
    ColladaMeshInstance(ColladaSkinnedMesh* skinnedMesh = nullptr);
    ~ColladaMeshInstance() = default;

    void Render();
    void AddPolygonGroupInstance(ColladaPolygonGroupInstance* instance);
    ColladaSkinnedMesh* GetSkinnedMesh();

    std::vector<ColladaPolygonGroupInstance*> polyGroupInstances;

    FCDGeometryInstance* geometryInstance = nullptr;

private:
    ColladaSkinnedMesh* skinnedMesh = nullptr;
};
};

#endif // __COLLADALOADER_COLLADALIGHT_H__
