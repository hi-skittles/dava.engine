#ifndef __CUSTOMGEOMETRYSERIALIZATIONDATA_H__
#define __CUSTOMGEOMETRYSERIALIZATIONDATA_H__

#include <memory>

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"

#include "Render/Material/NMaterial.h"

#include "Render/Highlevel/Vegetation/VegetationRenderData.h"

namespace DAVA
{
class Entity;

/**
 \brief This data structure allows to virtualize geometry storage. Geometry data
    is loaded both from the saved to scene KeyedArchive and an external .sc2 file.
    The data is passed to the geometry generator implementation then.
 */
class VegetationGeometryData
{
public:
    VegetationGeometryData(Vector<NMaterial*>& materialsData,
                           Vector<Vector<Vector<Vector3>>>& positionLods,
                           Vector<Vector<Vector<Vector2>>>& texCoordLods,
                           Vector<Vector<Vector<Vector3>>>& normalLods,
                           Vector<Vector<Vector<VegetationIndex>>>& indexLods);
    VegetationGeometryData(VegetationGeometryData& src);
    ~VegetationGeometryData();

    uint32 GetLayerCount() const;
    uint32 GetLodCount(uint32 layerIndex) const;

    NMaterial* GetMaterial(uint32 layerIndex);

    Vector<Vector3>& GetPositions(uint32 layerIndex, uint32 lodIndex);
    Vector<Vector2>& GetTextureCoords(uint32 layerIndex, uint32 lodIndex);
    Vector<Vector3>& GetNormals(uint32 layerIndex, uint32 lodIndex);
    Vector<VegetationIndex>& GetIndices(uint32 layerIndex, uint32 lodIndex);

private:
    void Load(Vector<NMaterial*>& materialsData,
              Vector<Vector<Vector<Vector3>>>& positionLods,
              Vector<Vector<Vector<Vector2>>>& texCoordLods,
              Vector<Vector<Vector<Vector3>>>& normalLods,
              Vector<Vector<Vector<VegetationIndex>>>& indexLods);

private:
    Vector<NMaterial*> materials;
    Vector<Vector<Vector<Vector3>>> positions;
    Vector<Vector<Vector<Vector2>>> texCoords;
    Vector<Vector<Vector<Vector3>>> normals;
    Vector<Vector<Vector<VegetationIndex>>> indices;
};

using VegetationGeometryDataPtr = std::unique_ptr<VegetationGeometryData>;

class VegetationGeometryDataReader
{
public:
    static VegetationGeometryDataPtr ReadScene(const FilePath& scenePath);
};
};

#endif /* defined(__CUSTOMGEOMETRYSERIALIZATIONDATA_H__) */
