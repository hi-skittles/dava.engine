#include <cfloat>

#include "Render/Highlevel/Vegetation/VegetationRenderObject.h"
#include "Render/Material/NMaterialNames.h"
#include "Render/Material/NMaterial.h"
#include "Utils/Random.h"
#include "Utils/StringFormat.h"
#include "Render/Image/ImageSystem.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Scene3D/Systems/FoliageSystem.h"
#include "Render/RenderHelper.h"
#include "Render/TextureDescriptor.h"
#include "Time/SystemTimer.h"
#include "Job/JobManager.h"

#include "Render/Highlevel/Vegetation/VegetationGeometry.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Render/Renderer.h"

#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "FileSystem/FileSystem.h"

#include "Logger/Logger.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(VegetationRenderObject)
{
    ReflectionRegistrator<VegetationRenderObject>::Begin()
    .Field("density", &VegetationRenderObject::GetLayerClusterLimit, &VegetationRenderObject::SetLayerClusterLimit)[M::DisplayName("Base density")]
    .Field("scaleVariation", &VegetationRenderObject::GetScaleVariation, &VegetationRenderObject::SetScaleVariation)[M::DisplayName("Scale variation")]
    .Field("rotationVariation", &VegetationRenderObject::GetRotationVariation, &VegetationRenderObject::SetRotationVariation)[M::DisplayName("Rotation variation")]
    .Field("lightmap", &VegetationRenderObject::GetLightmapPath, &VegetationRenderObject::SetLightmapAndGenerateDensityMap)[M::DisplayName("Lightmap")]
    .Field("lodRanges", &VegetationRenderObject::GetLodRange, &VegetationRenderObject::SetLodRange)[M::DisplayName("Lod ranges")]
    .Field("visibilityDistance", &VegetationRenderObject::GetVisibilityDistance, &VegetationRenderObject::SetVisibilityDistance)[M::DisplayName("Visibility distances")]
    .Field("maxVisibleQuads", &VegetationRenderObject::GetMaxVisibleQuads, &VegetationRenderObject::SetMaxVisibleQuads)[M::DisplayName("Max visible quads")]
    .Field("customGeometry", &VegetationRenderObject::GetCustomGeometryPath, &VegetationRenderObject::SetCustomGeometryPath)[M::DisplayName("Custom geometry")]
    .Field("cameraBias", &VegetationRenderObject::GetCameraBias, &VegetationRenderObject::SetCameraBias)[M::DisplayName("Camera Bias")]
    .Field("animationAmplitude", &VegetationRenderObject::GetLayersAnimationAmplitude, &VegetationRenderObject::SetLayersAnimationAmplitude)[M::DisplayName("Animation Amplitude")]
    .Field("animationSpring", &VegetationRenderObject::GetLayersAnimationSpring, &VegetationRenderObject::SetLayersAnimationSpring)[M::DisplayName("Animation Spring")]
    .Field("animationDrag", &VegetationRenderObject::GetLayerAnimationDragCoefficient, &VegetationRenderObject::SetLayerAnimationDragCoefficient)[M::DisplayName("Animation Drag")]
    .End();
}

static const uint32 MAX_CLUSTER_TYPES = 4;
static const uint32 MAX_DENSITY_LEVELS = 16;
//static const float32 CLUSTER_SCALE_NORMALIZATION_VALUE = 15.0f;

static const size_t MAX_RENDER_CELLS = 512;
static const float32 MAX_VISIBLE_CLIPPING_DISTANCE = 50.0f * 50.0f; //meters * meters (square length)
static const float32 MAX_VISIBLE_SCALING_DISTANCE = 40.0f * 40.0f;

static const uint32 DENSITY_MAP_SIZE = 128;
static const float32 DENSITY_THRESHOLD = 0.0f;

//static const float32 MAX_VISIBLE_CLIPPING_DISTANCE = 130.0f * 130.0f; //meters * meters (square length)
//static const float32 MAX_VISIBLE_SCALING_DISTANCE = 100.0f * 100.0f;

//static const uint32 FULL_BRUSH_VALUE = 0xFFFFFFFF;

static Vector3 LOD_RANGES_SCALE = Vector3(0.0f, 2.0f, 6.0f);
//static Vector3 LOD_RANGES_SCALE = Vector3(0.0f, 2.0f, 6.0f);

static Vector<float32> RESOLUTION_SCALE =
{
  1.0f,
  2.0f,
  4.0f,
};

static Vector<uint32> RESOLUTION_CELL_SQUARE =
{
  1,
  4,
  16
};

static Vector<uint32> RESOLUTION_TILES_PER_ROW =
{
  4,
  2,
  1
};

static Vector<uint32> RESOLUTION_CLUSTER_STRIDE =
{
  1,
  2,
  4
};

//#define VEGETATION_DRAW_LOD_COLOR

static Vector<Color> RESOLUTION_COLOR =
{
  Color(0.5f, 0.0f, 0.0f, 1.0f),
  Color(0.0f, 0.5f, 0.0f, 1.0f),
  Color(1.0f, 0.0f, 1.0f, 1.0f),
};

#ifdef VEGETATION_DRAW_LOD_COLOR
static const FastName UNIFORM_LOD_COLOR = FastName("lodColor");
#endif

VegetationRenderObject::VegetationRenderObject()
    : heightmap(nullptr)
    , halfWidth(0)
    , halfHeight(0)
    , renderData(nullptr)
    , maxPerturbationDistance(1000000.0f)
    , layerVisibilityMask(0xFF)
    , vegetationVisible(true)
    , vertexCount(0)
    , indexCount(0)
    , vegetationGeometry(NULL)
    , heightmapTexture(NULL)
    , cameraBias(0.0f)
    , layersAnimationSpring(2.f, 2.f, 2.f, 2.f)
    , layersAnimationDrag(1.4f, 1.4f, 1.4f, 1.4f)
{
    bbox.AddPoint(Vector3(0, 0, 0));
    bbox.AddPoint(Vector3(1, 1, 1));

    layerParams.resize(MAX_CLUSTER_TYPES);
    for (size_t i = 0; i < MAX_CLUSTER_TYPES; ++i)
    {
        layerParams[i].maxClusterCount = 1;
        layerParams[i].instanceScaleVariation = 0.0f;
        layerParams[i].instanceRotationVariation = 0.0f;
    }

    type = RenderObject::TYPE_VEGETATION;
    AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
    AddFlag(RenderObject::CUSTOM_PREPARE_TO_RENDER);

    unitWorldSize.resize(RESOLUTION_SCALE.size());
    resolutionRanges.resize(RESOLUTION_CELL_SQUARE.size());

    maxVisibleQuads = MAX_RENDER_CELLS;
    lodRanges = LOD_RANGES_SCALE;
    ResetVisibilityDistance();
    Renderer::GetSignals().needRestoreResources.Connect(this, &VegetationRenderObject::RestoreRenderData);
}

VegetationRenderObject::~VegetationRenderObject()
{
    if (renderData)
    {
        SafeDelete(renderData);
        rhi::DeleteVertexBuffer(vertexBuffer);
        rhi::DeleteIndexBuffer(indexBuffer);
    }

    SafeDelete(vegetationGeometry);

    SafeRelease(heightmap);
    SafeRelease(heightmapTexture);
    Renderer::GetSignals().needRestoreResources.Disconnect(this);
}

RenderBatch* VegetationRenderObject::CreateRenderBatch()
{
    DVASSERT(renderData);

    ScopedPtr<NMaterial> batchMaterial(new NMaterial());
    batchMaterial->SetParent(renderData->GetMaterial());

    float32 fakeData[4];
    Memset(fakeData, 0, sizeof(float32) * 4);
    batchMaterial->AddProperty(VegetationPropertyNames::UNIFORM_SWITCH_LOD_SCALE, fakeData, rhi::ShaderProp::TYPE_FLOAT2);
    batchMaterial->AddProperty(VegetationPropertyNames::UNIFORM_TILEPOS, fakeData, rhi::ShaderProp::TYPE_FLOAT3);
    batchMaterial->AddProperty(VegetationPropertyNames::UNIFORM_VEGWAVEOFFSET_X, fakeData, rhi::ShaderProp::TYPE_FLOAT4);
    batchMaterial->AddProperty(VegetationPropertyNames::UNIFORM_VEGWAVEOFFSET_Y, fakeData, rhi::ShaderProp::TYPE_FLOAT4);

    RenderBatch* batch = new RenderBatch();
    batch->SetMaterial(batchMaterial);
    batch->vertexBuffer = vertexBuffer;
    batch->indexBuffer = indexBuffer;
    batch->vertexCount = vertexCount;
    batch->vertexLayoutId = vertexLayoutUID;

    return batch;
}

RenderObject* VegetationRenderObject::Clone(RenderObject* newObject)
{
    if (!newObject)
    {
        DVASSERT(IsPointerToExactClass<VegetationRenderObject>(this), "Can clone only from VegetationRenderObject");
        newObject = new VegetationRenderObject();
    }
    else
    {
        DVASSERT(IsPointerToExactClass<VegetationRenderObject>(this), "Can clone only from VegetationRenderObject");
        DVASSERT(IsPointerToExactClass<VegetationRenderObject>(newObject), "Can clone only to VegetationRenderObject");
    }

    VegetationRenderObject* vegetationRenderObject = static_cast<VegetationRenderObject*>(newObject);

    vegetationRenderObject->customGeometryData.reset();
    if (customGeometryData)
    {
        vegetationRenderObject->customGeometryData.reset(new VegetationGeometryData(*customGeometryData));
    }

    vegetationRenderObject->densityMap.clear();

    vegetationRenderObject->SetVisibilityDistance(GetVisibilityDistance()); //VI: must be copied before lod ranges
    vegetationRenderObject->SetHeightmap(GetHeightmap());
    vegetationRenderObject->SetLayerClusterLimit(GetLayerClusterLimit());
    vegetationRenderObject->SetScaleVariation(GetScaleVariation());
    vegetationRenderObject->SetRotationVariation(GetRotationVariation());
    vegetationRenderObject->SetHeightmapPath(GetHeightmapPath());
    vegetationRenderObject->SetLightmap(GetLightmapPath());
    vegetationRenderObject->SetWorldSize(GetWorldSize());
    vegetationRenderObject->SetCustomGeometryPathInternal(GetCustomGeometryPath());
    vegetationRenderObject->SetCameraBias(GetCameraBias());
    vegetationRenderObject->SetLayersAnimationAmplitude(GetLayersAnimationAmplitude());
    vegetationRenderObject->SetLayersAnimationSpring(GetLayersAnimationSpring());
    vegetationRenderObject->SetDensityMap(densityMap);
    vegetationRenderObject->SetLayerAnimationDragCoefficient(GetLayerAnimationDragCoefficient());
    vegetationRenderObject->SetLodRange(GetLodRange());

    vegetationRenderObject->AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
    vegetationRenderObject->AddFlag(RenderObject::CUSTOM_PREPARE_TO_RENDER);

    return vegetationRenderObject;
}

void VegetationRenderObject::Save(KeyedArchive* archive, SerializationContext* serializationContext)
{
    //VI: need to remove render batches since they are temporary
    ClearRenderBatches();

    RenderObject::Save(archive, serializationContext);

    archive->SetVector4("vro.clusterLayerLimit", GetLayerClusterLimit());
    archive->SetVector4("vro.scaleVariation", GetScaleVariation());
    archive->SetVector4("vro.rotationVariation", GetRotationVariation());

    if (lightmapTexturePath.IsEmpty() == false)
    {
        archive->SetString("vro.lightmap", lightmapTexturePath.GetRelativePathname(serializationContext->GetScenePath()));
    }

    if (customGeometryPath.IsEmpty() == false)
    {
        archive->SetString("vro.customGeometry", customGeometryPath.GetRelativePathname(serializationContext->GetScenePath()));
    }

    archive->SetFloat("vro.cameraBias", cameraBias);

    if (customGeometryData)
    {
        ScopedPtr<KeyedArchive> customGeometryArchive(new KeyedArchive());
        SaveCustomGeometryData(serializationContext, customGeometryArchive, customGeometryData);
        archive->SetArchive("vro.geometryData", customGeometryArchive);
    }

    archive->SetVector4("vro.layerAnimationAmplitude", GetLayersAnimationAmplitude());
    archive->SetVector4("vro.layersAnimationSpring", GetLayersAnimationSpring());
    archive->SetVector4("vro.layersAnimationDrug", GetLayerAnimationDragCoefficient());

    uint32 densityMapSize = static_cast<uint32>(densityMap.size());
    archive->SetUInt32("vro.densityMapSize", densityMapSize);
    archive->SetByteArray("vro.flippedDensityMap", &densityMap[0], densityMapSize);

    const Vector3& savingLodRanges = GetLodRange();
    archive->SetVector3("vro.lodRanges", savingLodRanges);

    const Vector2& savingVisibilityDistance = GetVisibilityDistance();
    archive->SetVector2("vro.visibilityDistance", savingVisibilityDistance);
}

void VegetationRenderObject::Load(KeyedArchive* archive, SerializationContext* serializationContext)
{
    RenderObject::Load(archive, serializationContext);

    if (IsDataLoadNeeded())
    {
        //VI: must be loaded before lod ranges
        if (archive->IsKeyExists("vro.visibilityDistance"))
        {
            Vector2 savedVisibilityDistance = archive->GetVector2("vro.visibilityDistance");
            SetVisibilityDistance(savedVisibilityDistance);
        }

        if (archive->IsKeyExists("vro.lodRanges"))
        {
            Vector3 savedLodRanges = archive->GetVector3("vro.lodRanges");
            SetLodRange(savedLodRanges);
        }

        if (archive->IsKeyExists("vro.geometryData"))
        {
            KeyedArchive* customGeometryArchive = archive->GetArchive("vro.geometryData");
            customGeometryData = LoadCustomGeometryData(serializationContext, customGeometryArchive);
        }

        String customGeometry = archive->GetString("vro.customGeometry");
        if (customGeometry.empty() == false)
        {
            if (customGeometryData)
            {
                SetCustomGeometryPathInternal(serializationContext->GetScenePath() + customGeometry);
            }
            else
            {
                SetCustomGeometryPath(serializationContext->GetScenePath() + customGeometry);
            }
        }

        if (archive->IsKeyExists("vro.clusterLimit"))
        {
            SetClusterLimit(archive->GetUInt32("vro.clusterLimit"));
        }
        else
        {
            SetLayerClusterLimit(archive->GetVector4("vro.clusterLayerLimit"));
            //Vector4 fakeClusterLimit(6, 16, 16, 1);
            //SetLayerClusterLimit(fakeClusterLimit);
        }

        if (archive->IsKeyExists("vro.scaleVariation"))
        {
            SetScaleVariation(archive->GetVector4("vro.scaleVariation"));
        }
        else
        {
            SetScaleVariation(Vector4(0.2f, 0.2f, 0.2f, 0.2f));
        }

        if (archive->IsKeyExists("vro.rotationVariation"))
        {
            SetRotationVariation(archive->GetVector4("vro.rotationVariation"));
        }
        else
        {
            SetRotationVariation(Vector4(180.0f, 180.0f, 180.0f, 180.0f));
        }

        String lightmap = archive->GetString("vro.lightmap");
        if (lightmap.empty() == false)
        {
            SetLightmap(serializationContext->GetScenePath() + lightmap);
        }

        if (archive->IsKeyExists("vro.cameraBias"))
        {
            SetCameraBias(archive->GetFloat("vro.cameraBias"));
        }

        SetLayersAnimationAmplitude(archive->GetVector4("vro.layerAnimationAmplitude", GetLayersAnimationAmplitude()));
        SetLayersAnimationSpring(archive->GetVector4("vro.layersAnimationSpring", GetLayersAnimationSpring()));

        if (serializationContext->GetVersion() < FIXED_VEGETATION_SCENE_VERSION)
        {
            //old trash with each bit in separate record
            Vector<uint8> densityBits;
            if (archive->IsKeyExists("vro.flippedDensityBitCount"))
            {
                uint32 bitCount = archive->GetUInt32("vro.flippedDensityBitCount");
                densityBits.resize(bitCount);
                for (uint32 i = 0; i < bitCount; ++i)
                {
                    densityBits[i] = archive->GetBool(Format("vro.flippedDensityBit.%d", i)) ? 1 : 0;
                }
            }

            if (densityBits.size() == 0)
            {
                densityBits.resize(DENSITY_MAP_SIZE * DENSITY_MAP_SIZE, 1);
            }

            SetDensityMap(densityBits);
        }
        else
        {
            uint32 densityMapSize = archive->GetUInt32("vro.densityMapSize", 0);
            const uint8* byteArray = archive->GetByteArray("vro.flippedDensityMap");
            if ((densityMapSize != 0) && (byteArray != nullptr))
            {
                DVASSERT(archive->GetByteArraySize("vro.flippedDensityMap") == densityMapSize);
                densityMap.resize(densityMapSize);
                Memcpy(&densityMap[0], byteArray, densityMapSize);
            }
            else
            {
                densityMap.resize(DENSITY_MAP_SIZE * DENSITY_MAP_SIZE, 1);
            }
            UpdateVegetationSetup();
        }

        if (archive->IsKeyExists("vro.layersAnimationDrug"))
        {
            SetLayerAnimationDragCoefficient(archive->GetVector4("vro.layersAnimationDrug"));
        }
    }

    AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
    AddFlag(RenderObject::CUSTOM_PREPARE_TO_RENDER);
}

bool VegetationRenderObject::IsDataLoadNeeded()
{
    bool shouldLoadData = IsHardwareCapableToRenderVegetation();

    FastName currentQuality = QualitySettingsSystem::Instance()->GetCurMaterialQuality(VegetationPropertyNames::VEGETATION_QUALITY_GROUP_NAME);
    bool qualityAllowsVegetation = (VegetationPropertyNames::VEGETATION_QUALITY_NAME_HIGH == currentQuality);

    shouldLoadData = shouldLoadData && qualityAllowsVegetation;

    Renderer::GetOptions()->SetOption(RenderOptions::VEGETATION_DRAW, shouldLoadData);

    shouldLoadData |= QualitySettingsSystem::Instance()->GetKeepUnusedEntities();

    return shouldLoadData;
}

void VegetationRenderObject::PrepareToRender(Camera* camera)
{
    activeRenderBatchArray.clear();
    if (!ReadyToRender())
    {
        return;
    }

    size_t visibleCellCount = visibleCells.size();
    size_t renderBatchCount = GetRenderBatchCount();
    while (renderBatchCount < visibleCellCount)
    {
        AddRenderBatch(ScopedPtr<RenderBatch>(CreateRenderBatch()));
        ++renderBatchCount;
    }
    activeRenderBatchArray.clear();
    Vector<Vector<VegetationBufferItem>>& indexRenderDataObject = renderData->GetIndexBuffers();

    Vector3 posScale(0.0f, 0.0f, 0.0f);
    Vector2 switchLodScale;
    Vector4 vegetationAnimationOffset[2];

    for (size_t cellIndex = 0; cellIndex < visibleCellCount; ++cellIndex)
    {
        AbstractQuadTreeNode<VegetationSpatialData>* treeNode = visibleCells[cellIndex];

        RenderBatch* rb = GetRenderBatch(static_cast<uint32>(cellIndex));
        NMaterial* mat = rb->GetMaterial();

        uint32 resolutionIndex = MapCellSquareToResolutionIndex(treeNode->data.width * treeNode->data.height);

        Vector<VegetationBufferItem>& rdoVector = indexRenderDataObject[resolutionIndex];

        uint32 indexBufferIndex = treeNode->data.rdoIndex;
        DVASSERT(indexBufferIndex < rdoVector.size());

        VegetationBufferItem& bufferItem = rdoVector[indexBufferIndex];
        rb->startIndex = bufferItem.startIndex;
        rb->indexCount = bufferItem.indexCount;

        activeRenderBatchArray.emplace_back(rb);

        float32 distanceScale = 1.0f;

        if (treeNode->data.cameraDistance > visibleClippingDistances.y)
        {
            distanceScale = Clamp(1.0f - ((treeNode->data.cameraDistance - visibleClippingDistances.y) / (visibleClippingDistances.x - visibleClippingDistances.y)), 0.0f, 1.0f);
        }

        posScale.x = treeNode->data.bbox.min.x - unitWorldSize[resolutionIndex].x * (indexBufferIndex % RESOLUTION_TILES_PER_ROW[resolutionIndex]);
        posScale.y = treeNode->data.bbox.min.y - unitWorldSize[resolutionIndex].y * (indexBufferIndex / RESOLUTION_TILES_PER_ROW[resolutionIndex]);
        posScale.z = distanceScale;

        switchLodScale.x = float32(resolutionIndex);
        switchLodScale.y = Clamp(1.0f - (treeNode->data.cameraDistance / resolutionRanges[resolutionIndex].y), 0.0f, 1.0f);

        for (uint32 i = 0; i < 4; ++i)
        {
            Vector2 animationOffset = treeNode->data.animationOffset[i] * layersAnimationAmplitude.data[i];
            vegetationAnimationOffset[0].data[i] = animationOffset.x;
            vegetationAnimationOffset[1].data[i] = animationOffset.y;
        }

        mat->SetPropertyValue(VegetationPropertyNames::UNIFORM_SWITCH_LOD_SCALE, switchLodScale.data);
        mat->SetPropertyValue(VegetationPropertyNames::UNIFORM_TILEPOS, posScale.data);
        mat->SetPropertyValue(VegetationPropertyNames::UNIFORM_VEGWAVEOFFSET_X, vegetationAnimationOffset[0].data);
        mat->SetPropertyValue(VegetationPropertyNames::UNIFORM_VEGWAVEOFFSET_Y, vegetationAnimationOffset[1].data);
#ifdef VEGETATION_DRAW_LOD_COLOR
        mat->SetPropertyValue(VegetationPropertyNames::UNIFORM_LOD_COLOR, RESOLUTION_COLOR[resolutionIndex].color);
#endif
    }
}

Vector2 VegetationRenderObject::GetVegetationUnitWorldSize(float32 resolution) const
{
    return Vector2((worldSize.x / DENSITY_MAP_SIZE) * resolution,
                   (worldSize.y / DENSITY_MAP_SIZE) * resolution);
}

void VegetationRenderObject::BuildSpatialStructure()
{
    DVASSERT(heightmap);

    uint32 mapSize = DENSITY_MAP_SIZE;
    uint32 heightmapSize = heightmap->Size();

    halfWidth = mapSize / 2;
    halfHeight = mapSize / 2;

    heightmapToVegetationMapScale = Vector2((1.0f * heightmapSize) / mapSize,
                                            (1.0f * heightmapSize) / mapSize);

    uint32 treeDepth = FastLog2(mapSize);

    visibleCells.clear();
    quadTree.Init(treeDepth);
    AbstractQuadTreeNode<VegetationSpatialData>* node = quadTree.GetRoot();

    uint32 halfSize = mapSize >> 1;
    BuildSpatialQuad(node, NULL, -1 * halfSize, -1 * halfSize, mapSize, mapSize, node->data.bbox);
}

void VegetationRenderObject::BuildSpatialQuad(AbstractQuadTreeNode<VegetationSpatialData>* node, AbstractQuadTreeNode<VegetationSpatialData>* firstRenderableParent,
                                              int16 x, int16 y, uint16 width, uint16 height, AABBox3& parentBox)
{
    DVASSERT(node);

    node->data.x = x;
    node->data.y = y;

    if (width <= RESOLUTION_SCALE[RESOLUTION_SCALE.size() - 1])
    {
        node->data.width = width;
        node->data.height = height;
        node->data.isVisible = !IsNodeEmpty(node);

        if (width == RESOLUTION_SCALE[RESOLUTION_SCALE.size() - 1])
        {
            firstRenderableParent = node;
            node->data.rdoIndex = 0;
        }
        else
        {
            int16 offsetX = abs(node->data.x - firstRenderableParent->data.x) / width;
            int16 offsetY = abs(node->data.y - firstRenderableParent->data.y) / height;

            node->data.rdoIndex = offsetX + (offsetY * (firstRenderableParent->data.width / width));
        }
    }
    else
    {
        node->data.width = -1;
        node->data.height = -1;
        node->data.rdoIndex = -1;
    }

    if (node->IsTerminalLeaf())
    {
        int32 mapX = x + halfWidth;
        int32 mapY = y + halfHeight;

        float32 heightmapHeight = SampleHeight(mapX, mapY);
        node->data.bbox.AddPoint(Vector3(x * unitWorldSize[0].x, y * unitWorldSize[0].y, (heightmapHeight - 0.5f)));
        node->data.bbox.AddPoint(Vector3((x + width) * unitWorldSize[0].x, (y + height) * unitWorldSize[0].y, (heightmapHeight + 0.5f)));

        parentBox.AddPoint(node->data.bbox.min);
        parentBox.AddPoint(node->data.bbox.max);
    }
    else
    {
        int16 cellHalfWidth = width >> 1;
        int16 cellHalfHeight = height >> 1;

        BuildSpatialQuad(node->children[0], firstRenderableParent, x, y, cellHalfWidth, cellHalfHeight, node->data.bbox);
        BuildSpatialQuad(node->children[1], firstRenderableParent, x + cellHalfWidth, y, cellHalfWidth, cellHalfHeight, node->data.bbox);
        BuildSpatialQuad(node->children[2], firstRenderableParent, x, y + cellHalfHeight, cellHalfWidth, cellHalfHeight, node->data.bbox);
        BuildSpatialQuad(node->children[3], firstRenderableParent, x + cellHalfWidth, y + cellHalfHeight, cellHalfWidth, cellHalfHeight, node->data.bbox);

        parentBox.AddPoint(node->data.bbox.min);
        parentBox.AddPoint(node->data.bbox.max);
    }
}

Vector<AbstractQuadTreeNode<VegetationSpatialData>*>& VegetationRenderObject::BuildVisibleCellList(Camera* forCamera)
{
    Vector3 camPos = forCamera->GetPosition();
    Vector3 camDir = forCamera->GetDirection();
    camDir.z = 0.0f;

    camDir.Normalize();
    camPos = camPos + camDir * cameraBias;

    uint8 planeMask = 0x3F;
    Vector3 cameraPosXY = camPos;
    cameraPosXY.z = 0.0f;

    visibleCells.clear();

    BuildVisibleCellList(cameraPosXY, forCamera->GetFrustum(), planeMask, quadTree.GetRoot(), visibleCells, true);

    return visibleCells;
}

void VegetationRenderObject::BuildVisibleCellList(const Vector3& cameraPoint, Frustum* frustum, uint8 planeMask,
                                                  AbstractQuadTreeNode<VegetationSpatialData>* node, Vector<AbstractQuadTreeNode<VegetationSpatialData>*>& cellList, bool evaluateVisibility)
{
    static Array<Vector3, 4> corners;
    if (node)
    {
        Frustum::eFrustumResult result = Frustum::EFR_INSIDE;

        if (evaluateVisibility)
        {
            result = frustum->Classify(node->data.bbox, planeMask, node->data.clippingPlane);
        }

        if (Frustum::EFR_OUTSIDE != result)
        {
            bool needEvalClipping = (Frustum::EFR_INTERSECT == result);

            if (node->data.IsRenderable())
            {
                corners[0].x = node->data.bbox.min.x;
                corners[0].y = node->data.bbox.min.y;

                corners[1].x = node->data.bbox.max.x;
                corners[1].y = node->data.bbox.max.y;

                corners[2].x = node->data.bbox.max.x;
                corners[2].y = node->data.bbox.min.y;

                corners[3].x = node->data.bbox.min.x;
                corners[3].y = node->data.bbox.max.y;

                float32& refDistance = node->data.cameraDistance;

                refDistance = FLT_MAX;
                size_t cornersCount = corners.size();
                for (uint32 cornerIndex = 0; cornerIndex < cornersCount; ++cornerIndex)
                {
                    float32 cornerDistance = (cameraPoint - corners[cornerIndex]).SquareLength();
                    if (cornerDistance < refDistance)
                    {
                        refDistance = cornerDistance;
                    }
                }

                uint32 resolutionId = MapToResolution(refDistance);
                if (node->IsTerminalLeaf() || RESOLUTION_CELL_SQUARE[resolutionId] >= uint32(node->data.GetResolutionId()))
                {
                    AddVisibleCell(node, visibleClippingDistances.x, cellList);
                }
                else if (!node->IsTerminalLeaf())
                {
                    BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[0], cellList, needEvalClipping);
                    BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[1], cellList, needEvalClipping);
                    BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[2], cellList, needEvalClipping);
                    BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[3], cellList, needEvalClipping);
                }
            }
            else
            {
                BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[0], cellList, needEvalClipping);
                BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[1], cellList, needEvalClipping);
                BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[2], cellList, needEvalClipping);
                BuildVisibleCellList(cameraPoint, frustum, planeMask, node->children[3], cellList, needEvalClipping);
            }
        }
    }
}

bool VegetationRenderObject::CellByDistanceCompareFunction(const AbstractQuadTreeNode<VegetationSpatialData>* a,
                                                           const AbstractQuadTreeNode<VegetationSpatialData>* b)
{
    return (a->data.cameraDistance > b->data.cameraDistance);
}

void VegetationRenderObject::InitHeightTextureFromHeightmap(Heightmap* heightMap)
{
    SafeRelease(heightmapTexture);

    if (IsDataLoadNeeded())
    {
        uint32 hmSize = uint32(heightmap->Size());
        DVASSERT(IsPowerOf2(hmSize));

        Texture* tx = Texture::CreateFromData(FORMAT_RGBA4444, reinterpret_cast<uint8*>(heightMap->Data()), hmSize, hmSize, false);
        tx->SetWrapMode(rhi::TEXADDR_CLAMP, rhi::TEXADDR_CLAMP);
        tx->SetMinMagFilter(rhi::TEXFILTER_LINEAR, rhi::TEXFILTER_LINEAR, rhi::TEXMIPFILTER_NONE);

        heightmapTexture = SafeRetain(tx);

        if (vegetationGeometry != NULL)
        {
            ScopedPtr<KeyedArchive> props(new KeyedArchive());
            props->SetUInt64(NMaterialTextureName::TEXTURE_HEIGHTMAP.c_str(), uint64(heightmapTexture));

            vegetationGeometry->OnVegetationPropertiesChanged(renderData->GetMaterial(), props);
        }

        SafeRelease(tx);
    }
}

float32 VegetationRenderObject::SampleHeight(int16 x, int16 y)
{
    uint32 hX = uint32(heightmapToVegetationMapScale.x * x);
    uint32 hY = uint32(heightmapToVegetationMapScale.y * y);

    uint16 left = heightmap->GetHeightClamp(Min(hX, hX - 1), hY);
    uint16 right = heightmap->GetHeightClamp(hX + 1, hY);
    uint16 top = heightmap->GetHeightClamp(hX, Min(hY, hY - 1));
    uint16 bottom = heightmap->GetHeightClamp(hX, hY + 1);
    uint32 center = heightmap->GetHeightClamp(hX, hY);

    uint16 heightmapValue = (left + right + top + bottom + center) / 5;

    float32 height = (float32(heightmapValue) / float32(Heightmap::MAX_VALUE)) * worldSize.z;

    return height;
}

bool VegetationRenderObject::IsHardwareCapableToRenderVegetation()
{
    const rhi::RenderDeviceCaps& deviceCaps = rhi::DeviceCaps();
    bool result = deviceCaps.isVertexTextureUnitsSupported && deviceCaps.is32BitIndicesSupported && rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R4G4B4A4, rhi::PROG_VERTEX);

    return result;
}

bool VegetationRenderObject::IsValidGeometryData() const
{
    return (worldSize.Length() > 0 &&
            heightmap != nullptr &&
            heightmap->Size() > 0 &&
            densityMap.size() > 0 &&
            customGeometryData);
}

bool VegetationRenderObject::IsValidSpatialData() const
{
    return (worldSize.Length() > 0 &&
            heightmap != nullptr &&
            heightmap->Size() > 0 &&
            densityMap.size() > 0);
}

void VegetationRenderObject::UpdateVegetationSetup()
{
    if (densityMap.size() > 0)
    {
        size_t resolutionScaleSize = RESOLUTION_SCALE.size();
        for (size_t i = 0; i < resolutionScaleSize; ++i)
        {
            unitWorldSize[i] = GetVegetationUnitWorldSize(RESOLUTION_SCALE[i]);
        }
    }

    if (IsValidGeometryData())
    {
        CreateRenderData();
        renderData->GetMaterial()->PreBuildMaterial(PASS_FORWARD);
    }

    if (IsValidSpatialData())
    {
        BuildSpatialStructure();
    }

    ClearRenderBatches();
}

void VegetationRenderObject::ResetVisibilityDistance()
{
    visibleClippingDistances.x = MAX_VISIBLE_CLIPPING_DISTANCE;
    visibleClippingDistances.y = MAX_VISIBLE_SCALING_DISTANCE;
}

void VegetationRenderObject::ResetLodRanges()
{
    lodRanges = LOD_RANGES_SCALE;

    if (IsValidSpatialData())
    {
        InitLodRanges();
    }
}

void VegetationRenderObject::InitLodRanges()
{
    Vector2 smallestUnitSize = GetVegetationUnitWorldSize(RESOLUTION_SCALE[0]);

    resolutionRanges[0].x = lodRanges.x * smallestUnitSize.x;
    resolutionRanges[0].y = lodRanges.y * smallestUnitSize.x;

    resolutionRanges[1].x = lodRanges.y * smallestUnitSize.x;
    resolutionRanges[1].y = lodRanges.z * smallestUnitSize.x;

    resolutionRanges[2].x = lodRanges.z * smallestUnitSize.x;
    resolutionRanges[2].y = visibleClippingDistances.x; //RESOLUTION_RANGES[2].x * 1000.0f;

    size_t resolutionCount = resolutionRanges.size();
    for (size_t i = 0; i < resolutionCount; ++i)
    {
        resolutionRanges[i].x *= resolutionRanges[i].x;
        resolutionRanges[i].y *= resolutionRanges[i].y;
    }
}

void VegetationRenderObject::GetDataNodes(Set<DataNode*>& dataNodes)
{
    if (customGeometryData)
    {
        size_t layerCount = customGeometryData->GetLayerCount();
        for (uint32 i = 0; i < layerCount; ++i)
        {
            dataNodes.insert(customGeometryData->GetMaterial(i));
        }
    }
}

void VegetationRenderObject::CreateRenderData()
{
    InitLodRanges();

    SafeDelete(vegetationGeometry);
    vegetationGeometry = new VegetationGeometry(layerParams,
                                                MAX_DENSITY_LEVELS,
                                                GetVegetationUnitWorldSize(RESOLUTION_SCALE[0]),
                                                customGeometryPath,
                                                RESOLUTION_CELL_SQUARE.data(),
                                                static_cast<uint32>(RESOLUTION_CELL_SQUARE.size()),
                                                RESOLUTION_SCALE.data(),
                                                static_cast<uint32>(RESOLUTION_SCALE.size()),
                                                RESOLUTION_TILES_PER_ROW.data(),
                                                static_cast<uint32>(RESOLUTION_TILES_PER_ROW.size()),
                                                RESOLUTION_CLUSTER_STRIDE.data(),
                                                static_cast<uint32>(RESOLUTION_CLUSTER_STRIDE.size()),
                                                worldSize,
                                                customGeometryData);

    if (renderData)
    {
        SafeDelete(renderData);
        rhi::DeleteVertexBuffer(vertexBuffer);
        rhi::DeleteIndexBuffer(indexBuffer);
    }

    renderData = new VegetationRenderData();
    vegetationGeometry->Build(renderData);

    const Vector<VegetationVertex>& vertexData = renderData->GetVertices();
    const Vector<VegetationIndex>& indexData = renderData->GetIndices();

    vertexCount = uint32(vertexData.size());
    indexCount = uint32(indexData.size());

    rhi::VertexBuffer::Descriptor vDesc;
    vDesc.size = uint32(vertexData.size() * sizeof(VegetationVertex));
    vDesc.initialData = &vertexData.front();
    vDesc.usage = rhi::USAGE_STATICDRAW;
    vertexBuffer = rhi::CreateVertexBuffer(vDesc);

    rhi::IndexBuffer::Descriptor iDesc;
    iDesc.size = uint32(indexData.size() * sizeof(VegetationIndex));
    iDesc.indexSize = rhi::INDEX_SIZE_32BIT;
    iDesc.initialData = &indexData.front();
    iDesc.usage = rhi::USAGE_STATICDRAW;
    indexBuffer = rhi::CreateIndexBuffer(iDesc);

#if defined(__DAVAENGINE_IPHONE__)
    renderData->ReleaseRenderData(); //release vertex and index buffers data
#endif

    ScopedPtr<KeyedArchive> props(new KeyedArchive());
    props->SetUInt64(NMaterialTextureName::TEXTURE_HEIGHTMAP.c_str(), uint64(heightmapTexture));
    props->SetVector3(VegetationPropertyNames::UNIFORM_PERTURBATION_FORCE.c_str(), perturbationForce);
    props->SetFloat(VegetationPropertyNames::UNIFORM_PERTURBATION_FORCE_DISTANCE.c_str(), maxPerturbationDistance);
    props->SetVector3(VegetationPropertyNames::UNIFORM_PERTURBATION_POINT.c_str(), perturbationPoint);
    props->SetString(VegetationPropertyNames::UNIFORM_SAMPLER_VEGETATIONMAP.c_str(), lightmapTexturePath.GetStringValue());

    vegetationGeometry->OnVegetationPropertiesChanged(renderData->GetMaterial(), props);

    rhi::VertexLayout vertexLayout;
    vertexLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    //vertexLayout.AddElement(rhi::VS_NORMAL, 0, rhi::VDT_FLOAT, 3); uncomment, when normals will be used for vertex lit implementation
    vertexLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    vertexLayout.AddElement(rhi::VS_TEXCOORD, 1, rhi::VDT_FLOAT, 3);
    vertexLayout.AddElement(rhi::VS_TEXCOORD, 2, rhi::VDT_FLOAT, 3);
    vertexLayoutUID = rhi::VertexLayout::UniqueId(vertexLayout);

    ClearRenderBatches();
}

void VegetationRenderObject::RestoreRenderData()
{
    //#if defined(__DAVAENGINE_IPHONE__)
    //    DVASSERT(false, "Should not even try to restore on iphone - render data is released");
    //#endif

    if (renderData == nullptr)
        return;

    if ((vertexBuffer != rhi::InvalidHandle) && rhi::NeedRestoreVertexBuffer(vertexBuffer))
    {
        const Vector<VegetationVertex>& vertexData = renderData->GetVertices();
        uint32 vertexBufferSize = static_cast<uint32>(vertexData.size() * sizeof(VegetationVertex));
        rhi::UpdateVertexBuffer(vertexBuffer, &vertexData.front(), 0, vertexBufferSize);
    }

    if ((indexBuffer != rhi::InvalidHandle) && rhi::NeedRestoreIndexBuffer(indexBuffer))
    {
        const Vector<VegetationIndex>& indexData = renderData->GetIndices();
        uint32 indexBufferSize = static_cast<uint32>(indexData.size() * sizeof(VegetationIndex));
        rhi::UpdateIndexBuffer(indexBuffer, &indexData.front(), 0, indexBufferSize);
    }

    if (heightmap && heightmapTexture) //RHI_COMPLETE later change it to normal restoration and change init heightmap texture to normal logic
    {
        uint32 hmSize = uint32(heightmap->Size());
        DVASSERT(IsPowerOf2(hmSize));

        heightmapTexture->TexImage(0, hmSize, hmSize, reinterpret_cast<uint8*>(heightmap->Data()), hmSize * hmSize * sizeof(uint16), 0);
    }
}

bool VegetationRenderObject::ReadyToRender()
{
    bool renderFlag = IsHardwareCapableToRenderVegetation() && Renderer::GetOptions()->IsOptionEnabled(RenderOptions::VEGETATION_DRAW);

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WINDOWS__)
    //VI: case when vegetation was turned off and then qualit changed from low t high is not a real-world scenario
    //VI: real-world scenario is in resource editor when quality has been changed.
    FastName currentQuality = QualitySettingsSystem::Instance()->GetCurMaterialQuality(VegetationPropertyNames::VEGETATION_QUALITY_GROUP_NAME);
    bool qualityAllowsVegetation = (VegetationPropertyNames::VEGETATION_QUALITY_NAME_HIGH == currentQuality);

    renderFlag = (renderFlag && qualityAllowsVegetation);
#endif

    return renderFlag && vegetationVisible && renderData;
}

void VegetationRenderObject::DebugDrawVisibleNodes(RenderHelper* drawer)
{
    uint32 requestedBatchCount = Min(uint32(visibleCells.size()), maxVisibleQuads);
    for (uint32 i = 0; i < requestedBatchCount; ++i)
    {
        AbstractQuadTreeNode<VegetationSpatialData>* treeNode = visibleCells[i];
        uint32 resolutionIndex = MapCellSquareToResolutionIndex(treeNode->data.width * treeNode->data.height);

        drawer->DrawAABox(treeNode->data.bbox, RESOLUTION_COLOR[resolutionIndex], RenderHelper::DRAW_WIRE_DEPTH);
    }
}

void VegetationRenderObject::ClearRenderBatches()
{
    int32 batchesToRemove = GetRenderBatchCount();
    while (batchesToRemove > 0)
    {
        RemoveRenderBatch(batchesToRemove - 1);
        batchesToRemove = GetRenderBatchCount();
    }
}

void VegetationRenderObject::SetCustomGeometryPath(const FilePath& path)
{
    if (FileSystem::Instance()->Exists(path))
    {
        VegetationGeometryDataPtr fetchedData =
        VegetationGeometryDataReader::ReadScene(path);

        if (fetchedData)
        {
            customGeometryData = std::move(fetchedData);
            SetCustomGeometryPathInternal(path);
        }
    }
}

void VegetationRenderObject::SetCustomGeometryPathInternal(const FilePath& path)
{
    customGeometryPath = path;

    if (IsValidGeometryData())
    {
        CreateRenderData();
    }
}

VegetationGeometryDataPtr VegetationRenderObject::LoadCustomGeometryData(SerializationContext* context, KeyedArchive* srcArchive)
{
    uint32 layerCount = srcArchive->GetUInt32("cgsd.layerCount");

    Vector<NMaterial*> materials;
    Vector<Vector<Vector<Vector3>>> positions;
    Vector<Vector<Vector<Vector2>>> texCoords;
    Vector<Vector<Vector<Vector3>>> normals;
    Vector<Vector<Vector<VegetationIndex>>> indices;

    for (uint32 layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        KeyedArchive* layerArchive = srcArchive->GetArchive(Format("cgsd.layer.%d", layerIndex));

        uint64 materialId = layerArchive->GetUInt64("cgsd.layer.materialId");
        NMaterial* mat = static_cast<NMaterial*>(context->GetDataBlock(materialId));

        DVASSERT(mat);

        materials.push_back(mat);

        positions.push_back(Vector<Vector<Vector3>>());
        texCoords.push_back(Vector<Vector<Vector2>>());
        normals.push_back(Vector<Vector<Vector3>>());
        indices.push_back(Vector<Vector<VegetationIndex>>());

        Vector<Vector<Vector3>>& layerPositions = positions[positions.size() - 1];
        Vector<Vector<Vector2>>& layerTexCoords = texCoords[texCoords.size() - 1];
        Vector<Vector<Vector3>>& layerNormals = normals[normals.size() - 1];
        Vector<Vector<VegetationIndex>>& layerIndices = indices[indices.size() - 1];

        uint32 lodCount = layerArchive->GetUInt32("cgsd.layer.lodCount");
        for (uint32 lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            layerPositions.push_back(Vector<Vector3>());
            layerTexCoords.push_back(Vector<Vector2>());
            layerNormals.push_back(Vector<Vector3>());
            layerIndices.push_back(Vector<VegetationIndex>());

            Vector<Vector3>& lodPositions = layerPositions[layerPositions.size() - 1];
            Vector<Vector2>& lodTexCoords = layerTexCoords[layerTexCoords.size() - 1];
            Vector<Vector3>& lodNormals = layerNormals[layerNormals.size() - 1];
            Vector<VegetationIndex>& lodIndices = layerIndices[layerIndices.size() - 1];

            KeyedArchive* lodArchive = layerArchive->GetArchive(Format("cgsd.lod.%d", lodIndex));

            uint32 posCount = lodArchive->GetUInt32("cgsd.lod.posCount");
            for (uint32 i = 0; i < posCount; ++i)
            {
                Vector3 pos = lodArchive->GetVector3(Format("cgsd.lod.pos.%d", i));
                lodPositions.push_back(pos);
            }

            uint32 texCount = lodArchive->GetUInt32("cgsd.lod.texCount");
            for (uint32 i = 0; i < texCount; ++i)
            {
                Vector2 texCoord = lodArchive->GetVector2(Format("cgsd.lod.tex.%d", i));
                lodTexCoords.push_back(texCoord);
            }

            uint32 normalCount = lodArchive->GetUInt32("cgsd.lod.normalCount");
            for (uint32 i = 0; i < normalCount; ++i)
            {
                Vector3 normal = lodArchive->GetVector3(Format("cgsd.lod.normal.%d", i));
                lodNormals.push_back(normal);
            }

            uint32 indexCount = lodArchive->GetUInt32("cgsd.lod.indexCount");
            for (uint32 i = 0; i < indexCount; ++i)
            {
                uint32 index = lodArchive->GetInt32(Format("cgsd.lod.index.%d", i));
                lodIndices.push_back(index);
            }
        }
    }

    VegetationGeometryDataPtr data(new VegetationGeometryData(materials, positions, texCoords, normals, indices));

    return data;
}

void VegetationRenderObject::SaveCustomGeometryData(SerializationContext* context, KeyedArchive* dstArchive, const VegetationGeometryDataPtr& data)
{
    uint32 layerCount = data->GetLayerCount();
    dstArchive->SetUInt32("cgsd.layerCount", layerCount);

    for (uint32 layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        uint32 lodCount = data->GetLodCount(layerIndex);
        ScopedPtr<KeyedArchive> layerArchive(new KeyedArchive());

        layerArchive->SetUInt64("cgsd.layer.materialId", data->GetMaterial(layerIndex)->GetNodeID());
        layerArchive->SetUInt32("cgsd.layer.lodCount", lodCount);

        for (uint32 lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            ScopedPtr<KeyedArchive> lodArchive(new KeyedArchive());

            Vector<Vector3>& positions = data->GetPositions(layerIndex, lodIndex);
            Vector<Vector2>& texCoords = data->GetTextureCoords(layerIndex, lodIndex);
            Vector<Vector3>& normals = data->GetNormals(layerIndex, lodIndex);
            Vector<VegetationIndex>& indices = data->GetIndices(layerIndex, lodIndex);

            uint32 posCount = static_cast<uint32>(positions.size());
            lodArchive->SetUInt32("cgsd.lod.posCount", posCount);
            for (uint32 i = 0; i < posCount; ++i)
            {
                lodArchive->SetVector3(Format("cgsd.lod.pos.%d", i), positions[i]);
            }

            uint32 texCount = static_cast<uint32>(texCoords.size());
            lodArchive->SetUInt32("cgsd.lod.texCount", texCount);
            for (uint32 i = 0; i < texCount; ++i)
            {
                lodArchive->SetVector2(Format("cgsd.lod.tex.%d", i), texCoords[i]);
            }

            uint32 normalCount = static_cast<uint32>(normals.size());
            lodArchive->SetUInt32("cgsd.lod.normalCount", normalCount);
            for (uint32 i = 0; i < normalCount; ++i)
            {
                lodArchive->SetVector3(Format("cgsd.lod.normal.%d", i), normals[i]);
            }

            uint32 indexCount = static_cast<uint32>(indices.size());
            lodArchive->SetUInt32("cgsd.lod.indexCount", indexCount);
            for (uint32 i = 0; i < indexCount; ++i)
            {
                lodArchive->SetInt32(Format("cgsd.lod.index.%d", i), indices[i]);
            }

            layerArchive->SetArchive(Format("cgsd.lod.%d", lodIndex), lodArchive);
        }

        dstArchive->SetArchive(Format("cgsd.layer.%d", layerIndex), layerArchive);
    }
}

void VegetationRenderObject::RecalcBoundingBox()
{
}

void VegetationRenderObject::CollectMetrics(VegetationMetrics& metrics)
{
    metrics.renderBatchCount = 0;
    metrics.totalQuadTreeLeafCount = 0;

    metrics.quadTreeLeafCountPerLOD.clear();

    metrics.instanceCountPerLOD.clear();
    metrics.polyCountPerLOD.clear();
    metrics.instanceCountPerLayer.clear();
    metrics.polyCountPerLayer.clear();

    metrics.visibleInstanceCountPerLayer.clear();
    metrics.visibleInstanceCountPerLOD.clear();
    metrics.visiblePolyCountPerLayer.clear();
    metrics.visiblePolyCountPerLOD.clear();

    metrics.polyCountPerLayerPerLod.clear();

    metrics.isValid = false;

    if (renderData)
    {
        metrics.isValid = true;

        size_t visibleCellCount = visibleCells.size();

        metrics.renderBatchCount = static_cast<uint32>(visibleCells.size());
        metrics.totalQuadTreeLeafCount = static_cast<uint32>(visibleCellCount);

        size_t maxLodCount = RESOLUTION_CELL_SQUARE.size();
        metrics.quadTreeLeafCountPerLOD.resize(maxLodCount, 0);
        metrics.instanceCountPerLOD.resize(maxLodCount, 0);
        metrics.polyCountPerLOD.resize(maxLodCount, 0);
        metrics.visibleInstanceCountPerLOD.resize(maxLodCount, 0);
        metrics.visiblePolyCountPerLOD.resize(maxLodCount, 0);

        uint32 maxLayerCount = uint32(renderData->instanceCount.size());

        metrics.visibleInstanceCountPerLayer.resize(maxLayerCount, 0);
        metrics.visiblePolyCountPerLayer.resize(maxLayerCount, 0);
        metrics.instanceCountPerLayer.resize(maxLayerCount, 0);
        metrics.polyCountPerLayer.resize(maxLayerCount, 0);

        for (size_t i = 0; i < visibleCellCount; ++i)
        {
            AbstractQuadTreeNode<VegetationSpatialData>* spatialData = visibleCells[i];
            uint32 lodIndex = MapCellSquareToResolutionIndex(spatialData->data.width * spatialData->data.height);

            metrics.quadTreeLeafCountPerLOD[lodIndex] += 1;
        }

        size_t layerCount = renderData->instanceCount.size();

        if (metrics.polyCountPerLayerPerLod.size() < layerCount)
        {
            metrics.polyCountPerLayerPerLod.resize(layerCount);
        }

        for (size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
        {
            if (metrics.polyCountPerLayerPerLod[layerIndex].size() < renderData->polyCountPerInstance[layerIndex].size())
            {
                metrics.polyCountPerLayerPerLod[layerIndex].resize(renderData->polyCountPerInstance[layerIndex].size());
            }

            for (size_t lodIndex = 0; lodIndex < maxLodCount; ++lodIndex)
            {
                metrics.instanceCountPerLOD[lodIndex] += renderData->instanceCount[layerIndex][lodIndex];
                metrics.polyCountPerLOD[lodIndex] += (renderData->polyCountPerInstance[layerIndex][lodIndex] * renderData->instanceCount[layerIndex][lodIndex]);

                metrics.instanceCountPerLayer[layerIndex] += renderData->instanceCount[layerIndex][lodIndex];
                metrics.polyCountPerLayer[layerIndex] += (renderData->polyCountPerInstance[layerIndex][lodIndex] * renderData->instanceCount[layerIndex][lodIndex]);

                metrics.polyCountPerLayerPerLod[layerIndex][lodIndex] += renderData->polyCountPerInstance[layerIndex][lodIndex];
            }
        }

        for (size_t i = 0; i < visibleCellCount; ++i)
        {
            AbstractQuadTreeNode<VegetationSpatialData>* spatialData = visibleCells[i];
            uint32 lodIndex = MapCellSquareToResolutionIndex(spatialData->data.width * spatialData->data.height);

            for (size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
            {
                metrics.visibleInstanceCountPerLOD[lodIndex] += renderData->instanceCount[layerIndex][lodIndex];
                metrics.visiblePolyCountPerLOD[lodIndex] += (renderData->polyCountPerInstance[layerIndex][lodIndex] * renderData->instanceCount[layerIndex][lodIndex]);

                metrics.visibleInstanceCountPerLayer[layerIndex] += renderData->instanceCount[layerIndex][lodIndex];
                metrics.visiblePolyCountPerLayer[layerIndex] += (renderData->polyCountPerInstance[layerIndex][lodIndex] * renderData->instanceCount[layerIndex][lodIndex]);
            }
        }
    }
}

void VegetationRenderObject::GenerateDensityMapFromTransparencyMask(const FilePath& lightmapPath)
{
    std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(lightmapPath));
    if (!descriptor)
    {
        Logger::Error("[VegetationRenderObject::GenerateDensityMapFromTransparencyMask] Cannot create descriptor from %s", lightmapPath.GetAbsolutePathname().c_str());
        return;
    }

    FilePath imagePath = descriptor->GetSourceTexturePathname();

    ScopedPtr<Image> lightmapImage(ImageSystem::LoadSingleMip(imagePath, 0));
    if (lightmapImage)
    {
        uint32 ratio = lightmapImage->width / DENSITY_MAP_SIZE;

        DVASSERT(lightmapImage->GetPixelFormat() == FORMAT_RGBA8888);
        DVASSERT(ratio > 0);

        if (ratio > 0 && lightmapImage->GetPixelFormat() == FORMAT_RGBA8888)
        {
            densityMap.resize(DENSITY_MAP_SIZE * DENSITY_MAP_SIZE);
            uint32 stride = sizeof(uint32);
            for (uint32 y = 0; y < DENSITY_MAP_SIZE; ++y)
            {
                for (uint32 x = 0; x < DENSITY_MAP_SIZE; ++x)
                {
                    //VI: flip Y in order to match landscape and vegetation light mask
                    uint32 flippedY = DENSITY_MAP_SIZE - y - 1;

                    float32 meanAlpha = GetMeanAlpha(x, flippedY, ratio, stride, lightmapImage);

                    uint32 bitIndex = x + y * DENSITY_MAP_SIZE;
                    densityMap[bitIndex] = (meanAlpha > DENSITY_THRESHOLD) ? 1 : 0;
                }
            }
        }
    }
    else
    {
        Logger::Error("[VegetationRenderObject::GenerateDensityMapFromTransparencyMask] Cannot create image from %s", imagePath.GetAbsolutePathname().c_str());
    }

    /*Image* outputImage = Image::Create(DENSITY_MAP_SIZE, DENSITY_MAP_SIZE, FORMAT_RGBA8888);

    for(size_t i = 0; i < densityMapBits.size(); ++i)
    {
        if(false == densityMapBits[i])
        {
            outputImage->data[i * 4 + 0] = 0xFF;
            outputImage->data[i * 4 + 1] = 0;
            outputImage->data[i * 4 + 2] = 0;
            outputImage->data[i * 4 + 3] = 0xFF;
        }
        else
        {
            outputImage->data[i * 4 + 0] = 0xFF;
            outputImage->data[i * 4 + 1] = 0xFF;
            outputImage->data[i * 4 + 2] = 0xFF;
            outputImage->data[i * 4 + 3] = 0xFF;
        }
    }

    lightmapPath.ReplaceFilename("density_debug_output.png");
    outputImage->Save(lightmapPath);

    SafeRelease(outputImage);*/
}

float32 VegetationRenderObject::GetMeanAlpha(uint32 x, uint32 y, uint32 ratio, uint32 stride, Image* src) const
{
    uint32 actualStartX = x * ratio;
    uint32 actualStartY = y * ratio;
    uint32 actualEndX = actualStartX + ratio;
    uint32 actualEndY = actualStartY + ratio;

    float32 medianAlpha = 0.0f;
    uint32 fragmentCount = 0;
    for (uint32 yy = actualStartY; yy < actualEndY; ++yy)
    {
        for (uint32 xx = actualStartX; xx < actualEndX; ++xx)
        {
            uint32 fragmentIndex = xx + yy * src->width;
            uint32 fragmentOffset = fragmentIndex * stride;

            uint8* fragmentData = src->GetData() + fragmentOffset;

            medianAlpha += float32(fragmentData[3]) / 255.0f;
            fragmentCount++;
        }
    }

    return (medianAlpha / fragmentCount);
}

void VegetationRenderObject::SetDensityMap(const Vector<uint8>& densityBits)
{
    densityMap = densityBits;
    UpdateVegetationSetup();
}

uint32 VegetationRenderObject::MapCellSquareToResolutionIndex(uint32 cellSquare)
{
    uint32 index = 0;
    size_t resolutionCount = RESOLUTION_CELL_SQUARE.size();
    for (uint32 i = 0; i < resolutionCount; ++i)
    {
        if (cellSquare == RESOLUTION_CELL_SQUARE[i])
        {
            index = i;
            break;
        }
    }

    return index;
}

void VegetationRenderObject::BindDynamicParameters(Camera* camera, RenderBatch* batch)
{
    RenderObject::BindDynamicParameters(camera, batch);

    if (heightmap != nullptr)
    {
        Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_LANDSCAPE_HEIGHTMAP_TEXTURE_SIZE, &heightmapSize, pointer_size(&heightmapSize));
    }
}

void VegetationRenderObject::RebuildCustomGeometry()
{
    if (vegetationGeometry != nullptr)
    {
        ScopedPtr<KeyedArchive> props(new KeyedArchive());
        props->SetString(VegetationPropertyNames::UNIFORM_SAMPLER_VEGETATIONMAP.c_str(), lightmapTexturePath.GetStringValue());

        vegetationGeometry->OnVegetationPropertiesChanged(renderData->GetMaterial(), props);
    }
}

void VegetationRenderObject::Rebuild()
{
    RebuildCustomGeometry();

    GenerateDensityMapFromTransparencyMask(lightmapTexturePath);
    UpdateVegetationSetup();
}
};
