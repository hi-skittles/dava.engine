#pragma once

#include <memory>

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "Base/BaseMath.h"
#include "Base/AbstractQuadTree.h"
#include "Reflection/Reflection.h"
#include "Render/RenderBase.h"
#include "Render/Image/Image.h"

#include "Render/3D/PolygonGroup.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Material/NMaterial.h"
#include "Render/Highlevel/Heightmap.h"

#include "Scene3D/SceneFile/SerializationContext.h"

#include "Render/Highlevel/Vegetation/VegetationPropertyNames.h"
#include "Render/Highlevel/Vegetation/VegetationRenderData.h"
#include "Render/Highlevel/Vegetation/VegetationSpatialData.h"
#include "Render/Highlevel/Vegetation/VegetationGeometryData.h"
#include "Render/Highlevel/Vegetation/VegetationGeometry.h"

namespace DAVA
{
using VegetationMap = Image;
class FoliageSystem;

/**
 \brief This structure stores various vegetation metrics useful for performance
    analysis.
 */
struct VegetationMetrics
{
    Vector<uint32> visibleInstanceCountPerLayer;
    Vector<uint32> visibleInstanceCountPerLOD;
    Vector<uint32> instanceCountPerLOD;
    Vector<uint32> instanceCountPerLayer;

    Vector<uint32> visiblePolyCountPerLayer;
    Vector<uint32> visiblePolyCountPerLOD;
    Vector<uint32> polyCountPerLOD;
    Vector<uint32> polyCountPerLayer;

    Vector<Vector<uint32>> polyCountPerLayerPerLod; //layer-lod

    uint32 totalQuadTreeLeafCount;
    Vector<uint32> quadTreeLeafCountPerLOD;

    uint32 renderBatchCount;

    bool isValid = false;
};

/**
 \brief Vegetation rendering implementation. Performs frustum culling, selects geometry to render,
    provides list of render batches for render system.
    Use of external vegetation geometry (subclasses of VegetationGeometry) allows to easily switch
    between vegetation render modes.
 */
class VegetationRenderObject : public RenderObject
{
public:
    VegetationRenderObject();
    virtual ~VegetationRenderObject();

    RenderObject* Clone(RenderObject* newObject) override;
    void Save(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Load(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void BindDynamicParameters(Camera* camera, RenderBatch* batch) override;
    void PrepareToRender(Camera* camera) override;
    void RecalcBoundingBox() override;

    void CollectMetrics(VegetationMetrics& metrics);
    void DebugDrawVisibleNodes(RenderHelper* drawer);
    void GetDataNodes(Set<DataNode*>& dataNodes) override;

    inline void SetHeightmap(Heightmap* _heightmap);
    inline Heightmap* GetHeightmap() const;
    inline const FilePath& GetHeightmapPath() const;
    inline void SetHeightmapPath(const FilePath& path);

    inline void SetLightmap(const FilePath& filePath);
    inline void SetLightmapAndGenerateDensityMap(const FilePath& filePath);
    inline const FilePath& GetLightmapPath() const;

    inline void SetClusterLimit(const uint32& maxClusters);
    inline uint32 GetClusterLimit() const;

    inline void SetWorldSize(const Vector3 size);
    inline const Vector3& GetWorldSize() const;

    inline void SetVisibilityDistance(const Vector2& distances);
    inline const Vector2& GetVisibilityDistance() const;
    void ResetVisibilityDistance();

    inline void SetLodRange(const Vector3& distances);
    inline const Vector3& GetLodRange() const;
    inline void ResetLodRanges();

    inline void SetMaxVisibleQuads(const uint32& _maxVisibleQuads);
    inline const uint32& GetMaxVisibleQuads() const;

    inline void SetPerturbation(const Vector3& point, const Vector3& force, float32 distance);
    inline float32 GetPerturbationDistance() const;
    inline const Vector3& GetPerturbationForce() const;
    inline void SetPerturbationPoint(const Vector3& point);
    inline const Vector3& GetPerturbationPoint() const;

    inline void SetLayerVisibilityMask(const uint8& mask);
    inline const uint8& GetLayerVisibilityMask() const;

    inline void SetVegetationVisible(bool show);
    inline bool GetVegetationVisible() const;

    inline const FilePath& GetCustomGeometryPath() const;
    void SetCustomGeometryPath(const FilePath& path);

    inline void SetCameraBias(const float32& bias);
    inline float32 GetCameraBias() const;

    inline void SetLayerClusterLimit(const Vector4& maxClusters);
    inline Vector4 GetLayerClusterLimit() const;

    inline void SetScaleVariation(const Vector4& scaleVariation);
    inline Vector4 GetScaleVariation() const;

    inline void SetRotationVariation(const Vector4& rotationVariation);
    inline Vector4 GetRotationVariation() const;

    inline void SetLayersAnimationAmplitude(const Vector4& ampitudes);
    inline Vector4 GetLayersAnimationAmplitude() const;

    inline void SetLayersAnimationSpring(const Vector4& spring);
    inline Vector4 GetLayersAnimationSpring() const;

    inline void SetLayerAnimationDragCoefficient(const Vector4& drag);
    inline const Vector4& GetLayerAnimationDragCoefficient() const;

    static bool IsHardwareCapableToRenderVegetation();

    void Rebuild();

private:
    void RebuildCustomGeometry();

    RenderBatch* CreateRenderBatch();

    bool IsValidGeometryData() const;
    bool IsValidSpatialData() const;

    Vector2 GetVegetationUnitWorldSize(float32 resolution) const;

    void BuildSpatialStructure();
    void BuildSpatialQuad(AbstractQuadTreeNode<VegetationSpatialData>* node, AbstractQuadTreeNode<VegetationSpatialData>* firstRenderableParent, int16 x, int16 y, uint16 width, uint16 height, AABBox3& parentBox);

    Vector<AbstractQuadTreeNode<VegetationSpatialData>*>& BuildVisibleCellList(Camera* forCamera);

    void BuildVisibleCellList(const Vector3& cameraPoint, Frustum* frustum, uint8 planeMask, AbstractQuadTreeNode<VegetationSpatialData>* node,
                              Vector<AbstractQuadTreeNode<VegetationSpatialData>*>& cellList, bool evaluateVisibility);

    inline void AddVisibleCell(AbstractQuadTreeNode<VegetationSpatialData>* node, float32 refDistance, Vector<AbstractQuadTreeNode<VegetationSpatialData>*>& cellList);

    static bool CellByDistanceCompareFunction(const AbstractQuadTreeNode<VegetationSpatialData>* a, const AbstractQuadTreeNode<VegetationSpatialData>* b);

    void InitHeightTextureFromHeightmap(Heightmap* heightMap);

    float32 SampleHeight(int16 x, int16 y);

    void UpdateVegetationSetup();
    void InitLodRanges();

    void CreateRenderData();

    void RestoreRenderData();

    bool ReadyToRender();

    inline uint32 MapToResolution(float32 squareDistance);

    inline bool IsNodeEmpty(AbstractQuadTreeNode<VegetationSpatialData>* node) const;

    void ClearRenderBatches();

    void SetCustomGeometryPathInternal(const FilePath& path);
    VegetationGeometryDataPtr LoadCustomGeometryData(SerializationContext* context, KeyedArchive* srcArchive);
    void SaveCustomGeometryData(SerializationContext* context, KeyedArchive* dstArchive, const VegetationGeometryDataPtr& data);

    void GenerateDensityMapFromTransparencyMask(const FilePath& lightmapPath);
    float32 GetMeanAlpha(uint32 x, uint32 y, uint32 ratio, uint32 stride, Image* src) const;

    void SetDensityMap(const Vector<uint8>& densityBits);

    bool IsDataLoadNeeded();

private:
    uint32 MapCellSquareToResolutionIndex(uint32 cellSquare);

    Heightmap* heightmap;
    Vector3 worldSize;
    Vector<Vector2> unitWorldSize;
    Vector2 heightmapToVegetationMapScale;
    uint16 halfWidth;
    uint16 halfHeight;
    float32 heightmapSize;

    VegetationRenderData* renderData;

    AbstractQuadTree<VegetationSpatialData> quadTree;
    Vector<AbstractQuadTreeNode<VegetationSpatialData>*> visibleCells;

    FilePath heightmapPath;
    FilePath lightmapTexturePath;

    FilePath customGeometryPath;

    Vector2 visibleClippingDistances;
    Vector3 lodRanges;
    uint32 maxVisibleQuads;

    Vector3 perturbationForce;
    Vector3 perturbationPoint;
    float32 maxPerturbationDistance;

    uint8 layerVisibilityMask;

    bool vegetationVisible;

    Vector<Vector2> resolutionRanges;

    uint32 vertexLayoutUID;
    rhi::HVertexBuffer vertexBuffer;
    rhi::HIndexBuffer indexBuffer;
    uint32 vertexCount;
    uint32 indexCount;

    VegetationGeometry* vegetationGeometry;

    Texture* heightmapTexture;

    float32 cameraBias;

    VegetationGeometryDataPtr customGeometryData;

    Vector4 layersAnimationAmplitude;
    Vector4 layersAnimationSpring;
    Vector4 layersAnimationDrag;

    Vector<uint8> densityMap;

    Vector<VegetationLayerParams> layerParams;

    DAVA_VIRTUAL_REFLECTION(VegetationRenderObject, RenderObject);

    friend class FoliageSystem;
};

inline void VegetationRenderObject::AddVisibleCell(AbstractQuadTreeNode<VegetationSpatialData>* node,
                                                   float32 refDistance,
                                                   Vector<AbstractQuadTreeNode<VegetationSpatialData>*>& cellList)
{
    if (node->data.isVisible && node->data.cameraDistance <= refDistance)
    {
        cellList.push_back(node);
    }
}

inline uint32 VegetationRenderObject::MapToResolution(float32 squareDistance)
{
    uint32 resolutionId = 0;

    for (size_t i = 0, rangesCount = resolutionRanges.size(); i < rangesCount; ++i)
    {
        if (squareDistance > resolutionRanges[i].x &&
            squareDistance <= resolutionRanges[i].y)
        {
            resolutionId = static_cast<uint32>(i);
            break;
        }
    }

    return resolutionId;
}

inline bool VegetationRenderObject::IsNodeEmpty(AbstractQuadTreeNode<VegetationSpatialData>* node) const
{
    bool nodeEmpty = true;

    int32 maxX = node->data.x + node->data.width;
    int32 maxY = node->data.y + node->data.height;

    uint32 fullWidth = (halfWidth << 1);

    for (int32 y = node->data.y; y < maxY; ++y)
    {
        for (int32 x = node->data.x; x < maxX; ++x)
        {
            int32 mapX = x + halfWidth;
            int32 mapY = y + halfHeight;
            uint32 cellDescriptionIndex = (mapY * fullWidth) + mapX;

            if (densityMap[cellDescriptionIndex])
            {
                nodeEmpty = false;
                break;
            }
        }
    }

    return nodeEmpty;
}

inline void VegetationRenderObject::SetHeightmap(Heightmap* _heightmap)
{
    if (heightmap != _heightmap)
    {
        SafeRelease(heightmap);
        heightmap = (_heightmap != nullptr && _heightmap->Data()) ? SafeRetain(_heightmap) : nullptr;

        if (heightmap)
        {
            heightmapSize = static_cast<float32>(heightmap->Size());
            InitHeightTextureFromHeightmap(heightmap);
        }

        UpdateVegetationSetup();
    }
}

inline Heightmap* VegetationRenderObject::GetHeightmap() const
{
    return heightmap;
}

inline const FilePath& VegetationRenderObject::GetHeightmapPath() const
{
    return heightmapPath;
}

inline void VegetationRenderObject::SetHeightmapPath(const FilePath& path)
{
    heightmapPath = path;
}

inline void VegetationRenderObject::SetLightmap(const FilePath& filePath)
{
    lightmapTexturePath = filePath;
    RebuildCustomGeometry();
}

inline const FilePath& VegetationRenderObject::GetLightmapPath() const
{
    return lightmapTexturePath;
}

inline void VegetationRenderObject::SetLightmapAndGenerateDensityMap(const FilePath& filePath)
{
    SetLightmap(filePath);
    GenerateDensityMapFromTransparencyMask(filePath);

    UpdateVegetationSetup();
}

inline void VegetationRenderObject::SetClusterLimit(const uint32& maxClusters)
{
    float32 maxClustersf = float32(maxClusters);
    Vector4 tmpVec(maxClustersf, maxClustersf, maxClustersf, maxClustersf);
    SetLayerClusterLimit(tmpVec);
}

inline uint32 VegetationRenderObject::GetClusterLimit() const
{
    return layerParams[0].maxClusterCount;
}

inline void VegetationRenderObject::SetWorldSize(const Vector3 size)
{
    worldSize = size;

    UpdateVegetationSetup();
}

inline const Vector3& VegetationRenderObject::GetWorldSize() const
{
    return worldSize;
}

inline void VegetationRenderObject::SetVisibilityDistance(const Vector2& distances)
{
    visibleClippingDistances = distances;
}

inline const Vector2& VegetationRenderObject::GetVisibilityDistance() const
{
    return visibleClippingDistances;
}

inline void VegetationRenderObject::SetLodRange(const Vector3& distances)
{
    lodRanges = distances;

    if (IsValidSpatialData())
    {
        InitLodRanges();
    }
}

inline const Vector3& VegetationRenderObject::GetLodRange() const
{
    return lodRanges;
}

inline void VegetationRenderObject::SetMaxVisibleQuads(const uint32& _maxVisibleQuads)
{
    maxVisibleQuads = _maxVisibleQuads;
}

inline const uint32& VegetationRenderObject::GetMaxVisibleQuads() const
{
    return maxVisibleQuads;
}

inline void VegetationRenderObject::SetPerturbation(const Vector3& point, const Vector3& force, float32 distance)
{
    perturbationForce = force;
    maxPerturbationDistance = distance;
    perturbationPoint = point;

    if (vegetationGeometry != NULL)
    {
        KeyedArchive* props = new KeyedArchive();
        props->SetVector3(VegetationPropertyNames::UNIFORM_PERTURBATION_FORCE.c_str(), perturbationForce);
        props->SetFloat(VegetationPropertyNames::UNIFORM_PERTURBATION_FORCE_DISTANCE.c_str(), maxPerturbationDistance);
        props->SetVector3(VegetationPropertyNames::UNIFORM_PERTURBATION_POINT.c_str(), perturbationPoint);

        vegetationGeometry->OnVegetationPropertiesChanged(renderData->GetMaterial(), props);

        SafeRelease(props);
    }
}

inline float32 VegetationRenderObject::GetPerturbationDistance() const
{
    return maxPerturbationDistance;
}

inline const Vector3& VegetationRenderObject::GetPerturbationForce() const
{
    return perturbationForce;
}

inline const Vector3& VegetationRenderObject::GetPerturbationPoint() const
{
    return perturbationPoint;
}

inline void VegetationRenderObject::SetPerturbationPoint(const Vector3& point)
{
    perturbationPoint = point;

    if (vegetationGeometry != NULL)
    {
        KeyedArchive* props = new KeyedArchive();
        props->SetVector3(VegetationPropertyNames::UNIFORM_PERTURBATION_POINT.c_str(), perturbationPoint);

        vegetationGeometry->OnVegetationPropertiesChanged(renderData->GetMaterial(), props);

        SafeRelease(props);
    }
}

inline void VegetationRenderObject::SetLayerVisibilityMask(const uint8& mask)
{
    layerVisibilityMask = mask;
}

inline const uint8& VegetationRenderObject::GetLayerVisibilityMask() const
{
    return layerVisibilityMask;
}

inline void VegetationRenderObject::SetVegetationVisible(bool show)
{
    vegetationVisible = show;
}

inline bool VegetationRenderObject::GetVegetationVisible() const
{
    return vegetationVisible;
}

inline const FilePath& VegetationRenderObject::GetCustomGeometryPath() const
{
    return customGeometryPath;
}

inline void VegetationRenderObject::SetCameraBias(const float32& bias)
{
    cameraBias = bias;
}

inline float32 VegetationRenderObject::GetCameraBias() const
{
    return cameraBias;
}

inline void VegetationRenderObject::SetLayerClusterLimit(const Vector4& maxClusters)
{
    size_t layerCount = layerParams.size();
    for (size_t i = 0; i < layerCount; ++i)
    {
        layerParams[i].maxClusterCount = Clamp(uint32(Abs(maxClusters.data[i])), 1U, uint32(0x00000FFF));
    }

    UpdateVegetationSetup();
}

inline Vector4 VegetationRenderObject::GetLayerClusterLimit() const
{
    return Vector4(float32(layerParams[0].maxClusterCount),
                   float32(layerParams[1].maxClusterCount),
                   float32(layerParams[2].maxClusterCount),
                   float32(layerParams[3].maxClusterCount));
}

inline void VegetationRenderObject::SetScaleVariation(const Vector4& scaleVariation)
{
    size_t layerCount = layerParams.size();
    for (size_t i = 0; i < layerCount; ++i)
    {
        layerParams[i].instanceScaleVariation = Clamp(scaleVariation.data[i], 0.0f, 1.0f);
    }

    UpdateVegetationSetup();
}

inline Vector4 VegetationRenderObject::GetScaleVariation() const
{
    return Vector4(layerParams[0].instanceScaleVariation,
                   layerParams[1].instanceScaleVariation,
                   layerParams[2].instanceScaleVariation,
                   layerParams[3].instanceScaleVariation);
}

inline void VegetationRenderObject::SetRotationVariation(const Vector4& rotationVariation)
{
    size_t layerCount = layerParams.size();
    for (size_t i = 0; i < layerCount; ++i)
    {
        layerParams[i].instanceRotationVariation = Clamp(rotationVariation.data[i], 0.0f, 360.0f);
    }

    UpdateVegetationSetup();
}

inline Vector4 VegetationRenderObject::GetRotationVariation() const
{
    return Vector4(layerParams[0].instanceRotationVariation,
                   layerParams[1].instanceRotationVariation,
                   layerParams[2].instanceRotationVariation,
                   layerParams[3].instanceRotationVariation);
}

inline void VegetationRenderObject::SetLayersAnimationAmplitude(const Vector4& ampitudes)
{
    layersAnimationAmplitude = ampitudes;
}

inline Vector4 VegetationRenderObject::GetLayersAnimationAmplitude() const
{
    return layersAnimationAmplitude;
}

inline void VegetationRenderObject::SetLayersAnimationSpring(const Vector4& spring)
{
    layersAnimationSpring = spring;
    layersAnimationSpring.Clamp(.5f, 20.f);
}

inline Vector4 VegetationRenderObject::GetLayersAnimationSpring() const
{
    return layersAnimationSpring;
}

inline void VegetationRenderObject::SetLayerAnimationDragCoefficient(const Vector4& drag)
{
    layersAnimationDrag = drag;
}

inline const Vector4& VegetationRenderObject::GetLayerAnimationDragCoefficient() const
{
    return layersAnimationDrag;
}
}