#include <cfloat>

#include "Render/Material/NMaterial.h"
#include "Render/Material/NMaterialNames.h"
#include "Render/Highlevel/Vegetation/VegetationGeometry.h"
#include "Utils/Random.h"
#include "FileSystem/KeyedArchive.h"
#include "Render/Highlevel/Vegetation/VegetationPropertyNames.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Render/Texture.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

namespace DAVA
{
void VegetationGeometry::CustomGeometryLayerData::BuildBBox()
{
    bbox.Empty();

    size_t sourcePositionsCount = sourcePositions.size();
    for (size_t i = 0; i < sourcePositionsCount; ++i)
    {
        bbox.AddPoint(sourcePositions[i]);
    }
}

VegetationGeometry::CustomGeometryEntityData::CustomGeometryEntityData()
{
}

VegetationGeometry::CustomGeometryEntityData::CustomGeometryEntityData(const CustomGeometryEntityData& src)
{
    SetMaterial(src.material);

    lods = src.lods;
}

VegetationGeometry::CustomGeometryEntityData::~CustomGeometryEntityData()
{
    SafeRelease(material);
}

void VegetationGeometry::CustomGeometryEntityData::SetMaterial(NMaterial* mat)
{
    if (mat != material)
    {
        SafeRelease(material);
        material = SafeRetain(mat);
    }
}

bool VegetationGeometry::ClusterByMatrixCompareFunction(const ClusterResolutionData& a,
                                                        const ClusterResolutionData& b)
{
    return a.cellIndex < b.cellIndex;
}

VegetationGeometry::VegetationGeometry(const Vector<VegetationLayerParams>& _maxClusters,
                                       uint32 _maxDensityLevels,
                                       const Vector2& _unitSize,
                                       const FilePath& _dataPath,
                                       const uint32* _resolutionCellSquare,
                                       uint32 _resolutionCellSquareCount,
                                       const float32* _resolutionScale,
                                       uint32 _resolutionScaleCount,
                                       const uint32* _resolutionTilesPerRow,
                                       uint32 _resolutionTilesPerRowCount,
                                       const uint32* _resolutionClusterStride,
                                       uint32 _resolutionClusterStrideCount,
                                       const Vector3& _worldSize,
                                       const VegetationGeometryDataPtr& geometryData)
{
    maxClusters = _maxClusters;

    maxDensityLevels = _maxDensityLevels;
    unitSize = _unitSize;
    sourceDataPath = _dataPath;

    resolutionCellSquare.reserve(_resolutionCellSquareCount);
    for (uint32 i = 0; i < _resolutionCellSquareCount; ++i)
    {
        resolutionCellSquare.push_back(_resolutionCellSquare[i]);
    }

    resolutionScale.reserve(_resolutionScaleCount);
    for (uint32 i = 0; i < _resolutionScaleCount; ++i)
    {
        resolutionScale.push_back(_resolutionScale[i]);
    }

    resolutionTilesPerRow.reserve(_resolutionTilesPerRowCount);
    for (uint32 i = 0; i < _resolutionTilesPerRowCount; ++i)
    {
        resolutionTilesPerRow.push_back(_resolutionTilesPerRow[i]);
    }

    resolutionClusterStride.reserve(_resolutionClusterStrideCount);
    for (uint32 i = 0; i < _resolutionClusterStrideCount; ++i)
    {
        resolutionClusterStride.push_back(_resolutionClusterStride[i]);
    }

    worldSize = _worldSize;
    resolutionCount = static_cast<uint32>(resolutionClusterStride.size());

    InitCustomGeometry(geometryData);
}

VegetationGeometry::~VegetationGeometry()
{
}

void VegetationGeometry::Build(VegetationRenderData* renderData)
{
    size_t customGeometryDataCount = customGeometryData.size();
    if (customGeometryDataCount == 0)
    {
        return;
    }

    PrepareBoundingBoxes();

    Vector<VegetationVertex>& vertexData = renderData->GetVertices();
    Vector<VegetationIndex>& indexData = renderData->GetIndices();

    Vector<ClusterPositionData> clusterPositions;
    Vector<VertexRangeData> layerClusterRanges;
    GenerateClusterPositionData(maxClusters, clusterPositions, layerClusterRanges);

    Vector<Vector<BufferData>> resolutionDataArray;
    for (uint32 resolutionIndex = 0; resolutionIndex < resolutionCount; ++resolutionIndex)
    {
        resolutionDataArray.emplace_back();
        Vector<BufferData>& cellDataArray = resolutionDataArray.back();

        Vector<ClusterResolutionData> clusterResolution;
        Vector<BufferCellData> cellOffsets;
        GenerateClusterResolutionData(resolutionIndex, maxClusters, clusterPositions, layerClusterRanges, clusterResolution);

        std::stable_sort(clusterResolution.begin(), clusterResolution.end(), ClusterByMatrixCompareFunction);

        GenerateVertexData(customGeometryData, clusterResolution, vertexData, cellOffsets);

        size_t cellCount = cellOffsets.size();
        for (size_t cellIndex = 0; cellIndex < cellCount; ++cellIndex)
        {
            cellDataArray.emplace_back();
            BufferData& bufferOffsets = cellDataArray.back();
            GenerateIndexData(customGeometryData, clusterResolution, cellOffsets[cellIndex], vertexData, indexData, bufferOffsets);
        }
    }

    Vector<Vector<VegetationBufferItem>>& indexBuffers = renderData->GetIndexBuffers();
    for (size_t resolutionIndex = 0; resolutionIndex < resolutionCount; ++resolutionIndex)
    {
        Vector<BufferData>& indexBuffersOffsets = resolutionDataArray[resolutionIndex];

        indexBuffers.emplace_back();
        Vector<VegetationBufferItem>& currentResolutionIndexBuffers = indexBuffers.back();

        size_t cellCount = indexBuffersOffsets.size();
        for (size_t cellIndex = 0; cellIndex < cellCount; ++cellIndex)
        {
            currentResolutionIndexBuffers.emplace_back();
            VegetationBufferItem& indexBufferItem = currentResolutionIndexBuffers.back();

            BufferData& indexBufferOffset = indexBuffersOffsets[cellIndex];

            indexBufferItem.indexCount = indexBufferOffset.size;
            indexBufferItem.startIndex = indexBufferOffset.indexOffset;
        }
    }

    NMaterial* material = customGeometryData[0].material;
    material->SetFXName(NMaterialName::GRASS);
    renderData->SetMaterial(material);

    if (material->HasLocalProperty(VegetationPropertyNames::UNIFORM_WORLD_SIZE))
        material->SetPropertyValue(VegetationPropertyNames::UNIFORM_WORLD_SIZE, worldSize.data);
    else
        material->AddProperty(VegetationPropertyNames::UNIFORM_WORLD_SIZE, worldSize.data, rhi::ShaderProp::TYPE_FLOAT3);

#ifdef VEGETATION_DRAW_LOD_COLOR
    material->AddFlag(VegetationPropertyNames::FLAG_VEGETATION_DRAW_LOD_COLOR, 1);
    material->AddProperty(VegetationPropertyNames::UNIFORM_LOD_COLOR, fakeData, rhi::ShaderProp::TYPE_FLOAT3);
#endif

    //fill in metrics data
    size_t layerCount = customGeometryData.size();
    for (size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        renderData->instanceCount.push_back(Vector<uint32>());
        renderData->vertexCountPerInstance.push_back(Vector<uint32>());
        renderData->polyCountPerInstance.push_back(Vector<uint32>());

        Vector<uint32>& layerInstanceCount = renderData->instanceCount[renderData->instanceCount.size() - 1];
        Vector<uint32>& layerVertexCount = renderData->vertexCountPerInstance[renderData->vertexCountPerInstance.size() - 1];
        Vector<uint32>& polyVertexCount = renderData->polyCountPerInstance[renderData->polyCountPerInstance.size() - 1];

        CustomGeometryEntityData& layerGeometryInfo = customGeometryData[layerIndex];
        size_t lodCount = layerGeometryInfo.lods.size();
        for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            layerInstanceCount.push_back(maxClusters[layerIndex].maxClusterCount * maxClusters[layerIndex].maxClusterCount);
            layerVertexCount.push_back(static_cast<uint32>(layerGeometryInfo.lods[lodIndex].sourcePositions.size()));
            polyVertexCount.push_back(static_cast<uint32>(layerGeometryInfo.lods[lodIndex].sourceIndices.size() / 3));
        }
    }
}

void VegetationGeometry::OnVegetationPropertiesChanged(NMaterial* mat, KeyedArchive* props)
{
    if (mat)
    {
        String lightmapKeyName = VegetationPropertyNames::UNIFORM_SAMPLER_VEGETATIONMAP.c_str();
        if (props->IsKeyExists(lightmapKeyName))
        {
            FilePath lightmapPath = props->GetString(lightmapKeyName);

            ScopedPtr<Texture> lightmapTexture(Texture::CreateFromFile(lightmapPath));
            if (mat->HasLocalTexture(VegetationPropertyNames::UNIFORM_SAMPLER_VEGETATIONMAP))
                mat->SetTexture(VegetationPropertyNames::UNIFORM_SAMPLER_VEGETATIONMAP, lightmapTexture);
            else
                mat->AddTexture(VegetationPropertyNames::UNIFORM_SAMPLER_VEGETATIONMAP, lightmapTexture);
        }

        String heightmapKeyName = NMaterialTextureName::TEXTURE_HEIGHTMAP.c_str();
        if (props->IsKeyExists(heightmapKeyName))
        {
            Texture* heightmap = reinterpret_cast<Texture*>(props->GetUInt64(heightmapKeyName));
            if (mat->HasLocalTexture(NMaterialTextureName::TEXTURE_HEIGHTMAP))
                mat->SetTexture(NMaterialTextureName::TEXTURE_HEIGHTMAP, heightmap);
            else
                mat->AddTexture(NMaterialTextureName::TEXTURE_HEIGHTMAP, heightmap);
        }
    }
}

void VegetationGeometry::GenerateClusterPositionData(const Vector<VegetationLayerParams>& layerClusterCount, Vector<ClusterPositionData>& clusters, Vector<VertexRangeData>& layerRanges)
{
    clusters.clear();

    size_t layerCount = layerClusterCount.size();
    uint32 clustersSize = 0;
    for (size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        uint32 clusterRowSize = resolutionTilesPerRow[0] * layerClusterCount[layerIndex].maxClusterCount;
        clustersSize += clusterRowSize * clusterRowSize;
    }

    clusters.reserve(clustersSize);
    layerRanges.resize(layerCount);

    uint32 currentIndex = 0;
    Random* random = GetEngineContext()->random;
    for (size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        const VegetationLayerParams& layerParamsData = layerClusterCount[layerIndex];

        uint32 layerMaxClusters = layerParamsData.maxClusterCount;

        uint32 clusterRowSize = resolutionTilesPerRow[0] * layerMaxClusters;
        uint32 totalClusterCount = clusterRowSize * clusterRowSize;
        Vector2 vegetationInstanceSize(unitSize.x / layerMaxClusters, unitSize.y / layerMaxClusters);

        Vector<uint32> densityId;
        densityId.resize(totalClusterCount);
        for (uint32 i = 0; i < totalClusterCount; ++i)
        {
            densityId[i] = (i % maxDensityLevels);
        }
        std::random_shuffle(densityId.begin(), densityId.end(), [random](Vector<uint32>::difference_type diff) {
            return (random->Rand() % diff);
        });

        layerRanges[layerIndex].index = currentIndex;
        layerRanges[layerIndex].size = totalClusterCount;
        layerRanges[layerIndex].rowSize = clusterRowSize;

        for (uint32 clusterIndex = 0; clusterIndex < totalClusterCount; ++clusterIndex)
        {
            uint32 cellX = clusterIndex % clusterRowSize;
            uint32 cellY = clusterIndex / clusterRowSize;

            uint32 matrixCellX = cellX / layerMaxClusters;
            uint32 matrixCellY = cellY / layerMaxClusters;

            float32 randomX = unitSize.x * float32(random->RandFloat());
            float32 randomY = unitSize.y * float32(random->RandFloat());

            clusters.push_back(ClusterPositionData());
            ClusterPositionData& cluster = clusters[clusters.size() - 1];

            cluster.pos = Vector3((matrixCellX * unitSize.x) + randomX,
                                  (matrixCellY * unitSize.y) + randomY,
                                  0.0f);
            cluster.rotation = layerParamsData.instanceRotationVariation * (float32(random->RandFloat()) - 0.5f);
            cluster.scale = 1.0f - layerParamsData.instanceScaleVariation * float32(random->RandFloat());
            cluster.densityId = densityId[clusterIndex];
            cluster.layerId = static_cast<uint32>(layerIndex);

            currentIndex++;
        }
    }
}

void VegetationGeometry::GenerateClusterResolutionData(uint32 resolutionId, const Vector<VegetationLayerParams>& layerClusterCount, const Vector<ClusterPositionData>& clusterPosition,
                                                       const Vector<VertexRangeData>& layerRanges, Vector<ClusterResolutionData>& clusterResolution)
{
    uint32 layerCount = static_cast<uint32>(layerRanges.size());
    uint32 totalClusterCountInResolution = 0;
    for (size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        const VertexRangeData& layerRangeData = layerRanges[layerIndex];
        uint32 resolutionStride = resolutionClusterStride[resolutionId];

        totalClusterCountInResolution += layerRangeData.size / (resolutionStride * resolutionStride);
    }

    clusterResolution.clear();
    clusterResolution.reserve(totalClusterCountInResolution);

    uint32 currentTilesPerRowCount = resolutionTilesPerRow[resolutionId];

    for (size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        const VertexRangeData& layerRangeData = layerRanges[layerIndex];

        uint32 clusterRowCount = layerRangeData.size / layerRangeData.rowSize;
        uint32 resolutionRowSize = (resolutionTilesPerRow[0] / currentTilesPerRowCount) * layerClusterCount[layerIndex].maxClusterCount;

        DVASSERT(layerRangeData.rowSize == clusterRowCount);

        uint32 resolutionStride = resolutionClusterStride[resolutionId];
        ;
        for (uint32 clusterY = 0; clusterY < clusterRowCount; clusterY += resolutionStride)
        {
            for (uint32 clusterX = 0; clusterX < layerRangeData.rowSize; clusterX += resolutionStride)
            {
                clusterResolution.push_back(ClusterResolutionData());
                ClusterResolutionData& resolutionData = clusterResolution[clusterResolution.size() - 1];

                size_t clusterIndex = clusterX + clusterY * layerRangeData.rowSize;

                uint32 absoluteCellX = clusterIndex % layerRangeData.rowSize;
                uint32 absoluteCellY = static_cast<uint32>(clusterIndex / layerRangeData.rowSize);

                uint32 resolutionCellX = absoluteCellX / resolutionRowSize;
                uint32 resolutionCellY = absoluteCellY / resolutionRowSize;

                resolutionData.position = clusterPosition[layerRangeData.index + clusterIndex];
                resolutionData.resolutionId = PrepareResolutionId(resolutionId, clusterX, clusterY);
                resolutionData.effectiveResolutionId = resolutionId;
                resolutionData.cellIndex = resolutionCellX + resolutionCellY * currentTilesPerRowCount;
            }
        }
    }
}

uint32 VegetationGeometry::PrepareResolutionId(uint32 currentResolutionId, uint32 cellX, uint32 cellY) const
{
    uint32 nextResolutionId = Min(resolutionCount - 1, currentResolutionId + 1);
    uint32 rowStride = resolutionClusterStride[nextResolutionId];

    bool isNextResolution = (((cellX % rowStride) == 0) && ((cellY % rowStride) == 0));
    return (isNextResolution) ? nextResolutionId : currentResolutionId;
}

void VegetationGeometry::GenerateVertexData(const Vector<CustomGeometryEntityData>& sourceGeomData, const Vector<ClusterResolutionData>& clusterResolution,
                                            Vector<VegetationVertex>& vertexData, Vector<BufferCellData>& cellOffsets)
{
    cellOffsets.clear();

    uint32 currentMatrix = 0;
    uint32 currentCluster = 0;
    uint32 currentVertexIndex = static_cast<uint32>(vertexData.size());

    Vector<Vector3> transformedVertices;
    Vector<Vector3> transformedNormals;

    Vector<Vector3> scaledVertices;
    Vector<Vector3> scaledNormals;

    size_t clusterCount = clusterResolution.size();
    for (size_t clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
    {
        const ClusterResolutionData& clusterData = clusterResolution[clusterIndex];

        if (currentMatrix != clusterData.cellIndex)
        {
            BufferCellData rangeData;
            rangeData.vertexStartIndex = currentVertexIndex;
            rangeData.vertexCount = static_cast<uint32>(vertexData.size() - currentVertexIndex);
            rangeData.clusterStartIndex = currentCluster;
            rangeData.clusterCount = static_cast<uint32>(clusterIndex - currentCluster);

            cellOffsets.push_back(rangeData);

            currentCluster = static_cast<uint32>(clusterIndex);
            currentMatrix = clusterData.cellIndex;
            currentVertexIndex = static_cast<uint32>(vertexData.size());
        }

        const CustomGeometryLayerData& clusterGeometry = sourceGeomData[clusterData.position.layerId].lods[clusterData.effectiveResolutionId];

        Scale(clusterGeometry.pivot, clusterData.position.scale, clusterGeometry.sourcePositions, clusterGeometry.sourceNormals, scaledVertices, scaledNormals);

        Rotate(clusterData.position.rotation, scaledVertices, scaledNormals, transformedVertices, transformedNormals);

        size_t vertexCount = clusterGeometry.sourcePositions.size();
        for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            VegetationVertex vertex;

            vertex.coord.x = clusterData.position.pos.x + transformedVertices[vertexIndex].x;
            vertex.coord.y = clusterData.position.pos.y + transformedVertices[vertexIndex].y;
            vertex.coord.z = clusterData.position.pos.z + transformedVertices[vertexIndex].z;

            //vertex.normal = transformedNormals[vertexIndex]; uncomment, when normals will be used for vertex lit implementation

            vertex.texCoord1.x = float32(clusterData.resolutionId);
            vertex.texCoord1.y = float32(clusterData.position.layerId);
            vertex.texCoord1.z = Max(0.0f, ((vertex.coord.z - clusterGeometry.pivot.z) / (clusterGeometry.bbox.max.z - clusterGeometry.pivot.z)));

            vertex.texCoord2.x = clusterData.position.pos.x + clusterGeometry.pivot.x;
            vertex.texCoord2.y = clusterData.position.pos.y + clusterGeometry.pivot.y;
            vertex.texCoord2.z = clusterData.position.pos.z + clusterGeometry.pivot.z;

            vertex.texCoord0 = clusterGeometry.sourceTextureCoords[vertexIndex];

            vertexData.push_back(vertex);
        }
    }

    BufferCellData rangeData;
    rangeData.vertexStartIndex = currentVertexIndex;
    rangeData.vertexCount = static_cast<uint32>(vertexData.size() - currentVertexIndex);
    rangeData.clusterStartIndex = currentCluster;
    rangeData.clusterCount = static_cast<uint32>(clusterCount - currentCluster);

    cellOffsets.push_back(rangeData);
}

void VegetationGeometry::Rotate(float32 angle, const Vector<Vector3>& sourcePositions, const Vector<Vector3>& sourceNormals, Vector<Vector3>& rotatedPositions, Vector<Vector3>& rotatedNormals)
{
    rotatedPositions.clear();
    rotatedNormals.clear();

    Matrix4 rotMat;
    rotMat.BuildRotation(Vector3(0.0f, 0.0f, 1.0f), -DegToRad(angle));

    Matrix4 normalMatrix4;
    rotMat.GetInverse(normalMatrix4);
    normalMatrix4.Transpose();
    Matrix3 normalMatrix3;
    normalMatrix3 = normalMatrix4;

    size_t sourceVertexCount = sourcePositions.size();
    for (size_t vertexIndex = 0; vertexIndex < sourceVertexCount; ++vertexIndex)
    {
        Vector3 transformedVertex = sourcePositions[vertexIndex] * rotMat;
        rotatedPositions.push_back(transformedVertex);

        Vector3 transformedNormal = sourceNormals[vertexIndex] * normalMatrix3;
        transformedNormal.Normalize();

        rotatedNormals.push_back(transformedNormal);
    }
}

void VegetationGeometry::Scale(const Vector3& clusterPivot, float32 scale, const Vector<Vector3>& sourcePositions, const Vector<Vector3>& sourceNormals,
                               Vector<Vector3>& scaledPositions, Vector<Vector3>& scaledNormals)
{
    scaledPositions.clear();
    scaledNormals.clear();

    size_t sourceVertexCount = sourcePositions.size();
    for (size_t vertexIndex = 0; vertexIndex < sourceVertexCount; ++vertexIndex)
    {
        Vector3 transformedVertex;
        transformedVertex.Lerp(clusterPivot, sourcePositions[vertexIndex], scale);

        scaledPositions.push_back(transformedVertex);
        scaledNormals.push_back(sourceNormals[vertexIndex]);
    }
}

void VegetationGeometry::GenerateIndexData(const Vector<CustomGeometryEntityData>& sourceGeomData, const Vector<ClusterResolutionData>& clusterResolution, const BufferCellData& rangeData,
                                           Vector<VegetationVertex>& vertexData, Vector<VegetationIndex>& indexData, BufferData& bufferOffset)
{
    uint32 lastClusterIndex = rangeData.clusterStartIndex + rangeData.clusterCount;
    uint32 vertexIndexOffset = 0;
    Vector<VegetationIndex> sourceCellIndices;
    for (uint32 clusterIndex = rangeData.clusterStartIndex; clusterIndex < lastClusterIndex; ++clusterIndex)
    {
        const ClusterResolutionData& resolutionData = clusterResolution[clusterIndex];

        const CustomGeometryLayerData& layerGeometry = sourceGeomData[resolutionData.position.layerId].lods[resolutionData.effectiveResolutionId];
        size_t clusterIndexCount = layerGeometry.sourceIndices.size();

        for (size_t i = 0; i < clusterIndexCount; ++i)
        {
            uint32 index = rangeData.vertexStartIndex + vertexIndexOffset + layerGeometry.sourceIndices[i];
            sourceCellIndices.push_back(VegetationIndex(index));
        }

        vertexIndexOffset += static_cast<uint32>(layerGeometry.sourcePositions.size());
    }

    bufferOffset.indexOffset = static_cast<uint32>(indexData.size());
    bufferOffset.size = static_cast<uint32>(sourceCellIndices.size());

    size_t cellItemCount = sourceCellIndices.size() / 3;
    for (size_t cellItemIndex = 0; cellItemIndex < cellItemCount; ++cellItemIndex)
    {
        indexData.push_back(sourceCellIndices[cellItemIndex * 3 + 0]);
        indexData.push_back(sourceCellIndices[cellItemIndex * 3 + 1]);
        indexData.push_back(sourceCellIndices[cellItemIndex * 3 + 2]);
    }
}

void VegetationGeometry::InitCustomGeometry(const VegetationGeometryDataPtr& geometryData)
{
    uint32 layerCount = geometryData->GetLayerCount();
    for (uint32 layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        customGeometryData.push_back(CustomGeometryEntityData());
        CustomGeometryEntityData& customGeometryDataItem = customGeometryData[customGeometryData.size() - 1];

        customGeometryDataItem.SetMaterial(geometryData->GetMaterial(layerIndex));

        uint32 lodCount = geometryData->GetLodCount(layerIndex);
        for (uint32 lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            customGeometryDataItem.lods.push_back(CustomGeometryLayerData());
            CustomGeometryLayerData& geometryLayerData = customGeometryDataItem.lods[customGeometryDataItem.lods.size() - 1];

            Vector<Vector3>& positions = geometryLayerData.sourcePositions;
            Vector<Vector2>& texCoords = geometryLayerData.sourceTextureCoords;
            Vector<Vector3>& normals = geometryLayerData.sourceNormals;
            Vector<VegetationIndex>& indices = geometryLayerData.sourceIndices;

            Vector<Vector3>& srcPositions = geometryData->GetPositions(layerIndex, lodIndex);
            Vector<Vector2>& srcTexCoords = geometryData->GetTextureCoords(layerIndex, lodIndex);
            Vector<Vector3>& srcNormals = geometryData->GetNormals(layerIndex, lodIndex);
            Vector<VegetationIndex>& srcIndices = geometryData->GetIndices(layerIndex, lodIndex);

            size_t posCount = srcPositions.size();
            for (size_t posIndex = 0; posIndex < posCount; ++posIndex)
            {
                positions.push_back(srcPositions[posIndex]);
            }

            size_t texCoordCount = srcTexCoords.size();
            for (size_t texCoordIndex = 0; texCoordIndex < texCoordCount; ++texCoordIndex)
            {
                texCoords.push_back(srcTexCoords[texCoordIndex]);
            }

            size_t normalsCount = srcNormals.size();
            for (size_t normalIndex = 0; normalIndex < normalsCount; ++normalIndex)
            {
                normals.push_back(srcNormals[normalIndex]);
            }

            size_t indexCount = srcIndices.size();
            for (size_t indexIndex = 0; indexIndex < indexCount; ++indexIndex)
            {
                indices.push_back(srcIndices[indexIndex]);
            }
        }

        if (lodCount < resolutionCount)
        {
            uint32 stubGeometryCount = resolutionCount - lodCount;
            for (uint32 stubIndex = 0; stubIndex < stubGeometryCount; ++stubIndex)
            {
                customGeometryDataItem.lods.push_back(customGeometryDataItem.lods[customGeometryDataItem.lods.size() - 1]);
            }
        }
    }
}

void VegetationGeometry::PrepareBoundingBoxes()
{
    size_t customGeometryDataCount = customGeometryData.size();
    for (size_t geometryIndex = 0; geometryIndex < customGeometryDataCount; ++geometryIndex)
    {
        CustomGeometryEntityData& geometryEntityData = customGeometryData[geometryIndex];

        AABBox3 bbox;
        Vector3 pivot(0.0f, 0.0f, FLT_MAX);

        size_t lodCount = geometryEntityData.lods.size();
        for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            CustomGeometryLayerData& lodData = geometryEntityData.lods[lodIndex];

            size_t vertexCount = lodData.sourcePositions.size();
            for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                Vector3& pt = lodData.sourcePositions[vertexIndex];
                bbox.AddPoint(pt);

                if (pt.z < pivot.z)
                {
                    pivot = pt;
                }
            }
        }

        for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            CustomGeometryLayerData& lodData = geometryEntityData.lods[lodIndex];

            lodData.bbox = bbox;
            lodData.pivot = pivot;
        }
    }
}

void VegetationGeometry::SetupCameraPositions(const AABBox3& bbox, Vector<Vector3>& positions)
{
    float32 z = bbox.min.z + (bbox.max.z - bbox.min.z) * 0.5f;

    positions.push_back(Vector3(bbox.min.x, bbox.min.y, z));
    positions.push_back(Vector3(bbox.min.x + (bbox.max.x - bbox.min.x) * 0.5f, bbox.min.y, z));
    positions.push_back(Vector3(bbox.max.x, bbox.min.y, z));
    positions.push_back(Vector3(bbox.max.x, bbox.min.y + (bbox.max.y - bbox.min.y) * 0.5f, z));
    positions.push_back(Vector3(bbox.max.x, bbox.max.y, z));
    positions.push_back(Vector3(bbox.min.x + (bbox.max.x - bbox.min.x) * 0.5f, bbox.max.y, z));
    positions.push_back(Vector3(bbox.min.x, bbox.max.y, z));
    positions.push_back(Vector3(bbox.min.x, bbox.min.y + (bbox.max.y - bbox.min.y) * 0.5f, z));
}
};
