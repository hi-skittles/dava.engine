#include "MeshUtils.h"
#include "EdgeAdjacency.h"

#include "Scene3D/Components/ComponentHelpers.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Render/Material/NMaterial.h"
#include "Render/Highlevel/ShadowVolume.h"
#include "Render/Highlevel/SkinnedMesh.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Utils/StringFormat.h"
#include "Logger/Logger.h"

namespace DAVA
{
namespace MeshUtils
{
namespace MeshUtilsDetails
{
struct FaceWork
{
    int32 indexOrigin[3];
    Vector3 tangent, binormal;
};

struct VertexWork
{
    Vector<int32> refIndices;
    Vector3 tangent, binormal;
    int32 tbRatio;
    int32 refIndex;
    int32 resultGroup;
};

struct SkinnedMeshWorkKey
{
    SkinnedMeshWorkKey(int32 _lod, int32 _switch, NMaterial* _materialParent)
        :
        lodIndex(_lod)
        , switchIndex(_switch)
        , materialParent(_materialParent)
    {
    }

    bool operator==(const SkinnedMeshWorkKey& data) const
    {
        return (lodIndex == data.lodIndex)
        && (switchIndex == data.switchIndex)
        && (materialParent == data.materialParent);
    }

    bool operator<(const SkinnedMeshWorkKey& data) const
    {
        if (materialParent != data.materialParent)
        {
            return materialParent < data.materialParent;
        }
        else if (lodIndex != data.lodIndex)
        {
            return lodIndex < data.lodIndex;
        }
        else
        {
            return switchIndex < data.switchIndex;
        }
    }

    int32 lodIndex;
    int32 switchIndex;
    NMaterial* materialParent;
};

struct SkinnedMeshJointWork
{
    SkinnedMeshJointWork(RenderBatch* _batch, uint32 _jointIndex)
        :
        batch(_batch)
        , jointIndex(_jointIndex)
    {
    }

    RenderBatch* batch;
    uint32 jointIndex;
};

struct EdgeMappingWork
{
    int32 oldEdge[2];
    int32 newEdge[2][2];

    EdgeMappingWork()
    {
        Memset(oldEdge, -1, sizeof(oldEdge));
        Memset(newEdge, -1, sizeof(newEdge));
    }
};

struct SkinnedTriangleWork
{
    Set<int32> usedJoints;
    int32 indices[3];
    bool processed = false;
};

int32 FindEdgeInMappingTable(int32 nV1, int32 nV2, EdgeMappingWork* mapping, int32 count)
{
    for (int i = 0; i < count; ++i)
    {
        // If both vertex indexes of the old edge in mapping entry are -1, then
        // we have searched every valid entry without finding a match.  Return
        // this index as a newly created entry.
        if ((mapping[i].oldEdge[0] == -1 && mapping[i].oldEdge[1] == -1) ||

            // Or if we find a match, return the index.
            (mapping[i].oldEdge[1] == nV1 && mapping[i].oldEdge[0] == nV2))
        {
            return i;
        }
    }

    DVASSERT(0);
    return -1; // We should never reach this line
}

Vector<int32> GetSignificantJoints(PolygonGroup* pg, int32 vertex)
{
    DVASSERT(pg);
    DVASSERT(vertex < pg->GetVertexCount());

    Vector<int32> result;
    int32 jIndex = -1;
    float32 jWeight = 0.f;
    if (pg->GetFormat() & EVF_HARD_JOINTINDEX) //hard-skinning
    {
        pg->GetHardJointIndex(vertex, jIndex);
        result.emplace_back(jIndex);
    }
    else if (pg->GetFormat() & (EVF_JOINTINDEX | EVF_JOINTWEIGHT)) //soft-skinning
    {
        for (uint32 j = 0; j < 4; ++j)
        {
            pg->GetJointWeight(vertex, j, jWeight);
            if (jWeight > EPSILON)
            {
                pg->GetJointIndex(vertex, j, jIndex);
                result.emplace_back(jIndex);
            }
        }
    }

    return result;
}

void ReplaceSignificantJoints(PolygonGroup* pg, int32 vertex, const Map<int32, int32>& jointsMap)
{
    DVASSERT(pg);
    DVASSERT(vertex < pg->GetVertexCount());

    int32 jIndex = -1;
    float32 jWeight = 0.f;
    if (pg->GetFormat() & EVF_HARD_JOINTINDEX) //hard-skinning
    {
        pg->GetHardJointIndex(vertex, jIndex);
        DVASSERT(jointsMap.count(jIndex) != 0);
        pg->SetHardJointIndex(vertex, jointsMap.at(jIndex));
    }
    else if (pg->GetFormat() & (EVF_JOINTINDEX | EVF_JOINTWEIGHT)) //soft-skinning
    {
        for (uint32 j = 0; j < 4; ++j)
        {
            pg->GetJointWeight(vertex, j, jWeight);
            if (jWeight > EPSILON)
            {
                pg->GetJointIndex(vertex, j, jIndex);
                DVASSERT(jointsMap.count(jIndex) != 0);
                pg->SetJointIndex(vertex, j, jointsMap.at(jIndex));
            }
        }
    }
}
}

void CopyVertex(PolygonGroup* srcGroup, uint32 srcPos, PolygonGroup* dstGroup, uint32 dstPos)
{
    int32 srcFormat = srcGroup->GetFormat();
    int32 dstFormat = dstGroup->GetFormat();
    int32 copyFormat = srcFormat & dstFormat; //most common format;

    uint8* srcData = srcGroup->meshData + srcPos * GetVertexSize(srcFormat);
    uint8* dstData = dstGroup->meshData + dstPos * GetVertexSize(dstFormat);

    for (uint32 mask = EVF_LOWER_BIT; mask <= EVF_HIGHER_BIT; mask = mask << 1)
    {
        int32 vertexAttribSize = GetVertexSize(mask);
        if (mask & copyFormat)
            Memcpy(dstData, srcData, vertexAttribSize);

        if (mask & srcFormat)
            srcData += vertexAttribSize;

        if (mask & dstFormat)
            dstData += vertexAttribSize;

        copyFormat &= ~mask;
    }

    /*unsupported stream*/
    DVASSERT((copyFormat == 0) && "Unsupported attribute stream in copy");
}

void CopyGroupData(PolygonGroup* srcGroup, PolygonGroup* dstGroup)
{
    dstGroup->ReleaseData();
    dstGroup->AllocateData(srcGroup->GetFormat(), srcGroup->GetVertexCount(), srcGroup->GetIndexCount());

    Memcpy(dstGroup->meshData, srcGroup->meshData, srcGroup->GetVertexCount() * srcGroup->vertexStride);
    Memcpy(dstGroup->indexArray, srcGroup->indexArray, srcGroup->GetIndexCount() * sizeof(int16));

    dstGroup->BuildBuffers();
}

void RebuildMeshTangentSpace(PolygonGroup* group, bool precomputeBinormal /*=true*/)
{
    using namespace MeshUtilsDetails;

    DVASSERT(group->GetPrimitiveType() == rhi::PRIMITIVE_TRIANGLELIST); //only triangle lists for now
    DVASSERT(group->GetFormat() & EVF_TEXCOORD0);
    DVASSERT(group->GetFormat() & EVF_NORMAL);

    Vector<FaceWork> faces;
    uint32 faceCount = group->GetPrimitiveCount();
    faces.resize(faceCount);
    Vector<VertexWork> verticesOrigin;
    Vector<VertexWork> verticesFull;
    verticesOrigin.resize(group->GetVertexCount());
    verticesFull.resize(group->GetIndexCount());

    for (uint32 i = 0, sz = group->GetVertexCount(); i < sz; ++i)
        verticesOrigin[i].refIndex = i;
    //compute tangent for faces
    for (uint32 f = 0; f < faceCount; ++f)
    {
        Vector3 pos[3];
        Vector2 texCoord[3];
        for (uint32 i = 0; i < 3; ++i)
        {
            int32 workIndex = f * 3 + i;
            int32 originIndex;
            group->GetIndex(workIndex, originIndex);
            faces[f].indexOrigin[i] = originIndex;
            group->GetCoord(originIndex, pos[i]);
            group->GetTexcoord(0, originIndex, texCoord[i]);

            verticesOrigin[originIndex].refIndices.push_back(workIndex);
            verticesFull[f * 3 + i].refIndex = faces[f].indexOrigin[i];
        }

        float32 x10 = pos[1].x - pos[0].x;
        float32 y10 = pos[1].y - pos[0].y;
        float32 z10 = pos[1].z - pos[0].z;
        float32 u10 = texCoord[1].x - texCoord[0].x;
        float32 v10 = texCoord[1].y - texCoord[0].y;

        float32 x20 = pos[2].x - pos[0].x;
        float32 y20 = pos[2].y - pos[0].y;
        float32 z20 = pos[2].z - pos[0].z;
        float32 u20 = texCoord[2].x - texCoord[0].x;
        float32 v20 = texCoord[2].y - texCoord[0].y;

        float32 d = u10 * v20 - u20 * v10;

        if (d == 0.0f)
        {
            d = 1.0f; // this may happen in case of degenerated triangle
        }
        d = 1.0f / d;

        Vector3 tangent = Vector3((v20 * x10 - v10 * x20) * d, (v20 * y10 - v10 * y20) * d, (v20 * z10 - v10 * z20) * d);
        Vector3 binormal = Vector3((x20 * u10 - x10 * u20) * d, (y20 * u10 - y10 * u20) * d, (z20 * u10 - z10 * u20) * d);

        //should we normalize it here or only final result?
        tangent.Normalize();
        binormal.Normalize();

        faces[f].tangent = tangent;
        faces[f].binormal = binormal;
        for (int32 i = 0; i < 3; ++i)
        {
            verticesFull[f * 3 + i].tangent = tangent;
            verticesFull[f * 3 + i].binormal = binormal;
        }
    }

    /*smooth tangent space preventing mirrored uv's smooth*/
    for (uint32 v = 0, sz = static_cast<uint32>(verticesFull.size()); v < sz; ++v)
    {
        int32 faceId = v / 3;
        VertexWork& originVert = verticesOrigin[verticesFull[v].refIndex];
        verticesFull[v].tbRatio = 1;
        for (int32 iRef = 0, refSz = static_cast<int32>(originVert.refIndices.size()); iRef < refSz; ++iRef)
        {
            int32 refFaceId = originVert.refIndices[iRef] / 3;
            if (refFaceId == faceId)
                continue;

            //check if uv's mirrored;

            //here we use handness to find mirrored UV's - still not sure if it is better then using dot product (upd: experiments show it is really better)
            Vector3 n1 = CrossProduct(verticesFull[v].tangent, verticesFull[v].binormal);
            Vector3 n2 = CrossProduct(faces[refFaceId].tangent, faces[refFaceId].binormal);

            if (DotProduct(n1, n2) > 0.0f)
            {
                verticesFull[v].tangent += faces[refFaceId].tangent;
                verticesFull[v].binormal += faces[refFaceId].binormal;
                verticesFull[v].tbRatio++;
            }
        }

        //as we use normalized tangent space - we renormalize vertex TB instead of rescaling it - think later if it is ok
        verticesFull[v].tangent.Normalize();
        verticesFull[v].binormal.Normalize();

        /*float32 invScale = 1.0f/(float32)vertices_full[v].tbRatio;
        vertices_full[v].tangent*=invScale;
        vertices_full[v].binormal*=invScale;*/
    }

    const float32 EPS = 0.00001f; //should be the same value as in exporter
    Vector<int32> groups;
    //unlock vertices that have different tangent/binormal but same ref
    for (uint32 i = 0, sz = static_cast<uint32>(verticesOrigin.size()); i < sz; ++i)
    {
        DVASSERT(verticesOrigin[i].refIndices.size()); //vertex with no reference triangles found?

        verticesOrigin[i].tangent = verticesFull[verticesOrigin[i].refIndices[0]].tangent;
        verticesOrigin[i].binormal = verticesFull[verticesOrigin[i].refIndices[0]].binormal;

        if (verticesOrigin[i].refIndices.size() <= 1) //1 and less references do not need unlock test
            continue;
        groups.clear();
        groups.push_back(0);
        verticesFull[verticesOrigin[i].refIndices[0]].resultGroup = 0;
        //if has different refs, check different groups;
        for (int32 refId = 1, refSz = static_cast<int32>(verticesOrigin[i].refIndices.size()); refId < refSz; ++refId)
        {
            VertexWork& vertexRef = verticesFull[verticesOrigin[i].refIndices[refId]];
            bool groupFound = false;
            for (int32 groupId = 0, groupSz = static_cast<int32>(groups.size()); groupId < groupSz; ++groupId)
            {
                const VertexWork& groupRef = verticesFull[verticesOrigin[i].refIndices[groups[groupId]]];
                bool groupEqual = FLOAT_EQUAL_EPS(vertexRef.tangent.x, groupRef.tangent.x, EPS) && FLOAT_EQUAL_EPS(vertexRef.tangent.y, groupRef.tangent.y, EPS) && FLOAT_EQUAL_EPS(vertexRef.tangent.z, groupRef.tangent.z, EPS);
                if (precomputeBinormal)
                    groupEqual &= FLOAT_EQUAL_EPS(vertexRef.binormal.x, groupRef.binormal.x, EPS) && FLOAT_EQUAL_EPS(vertexRef.binormal.y, groupRef.binormal.y, EPS) && FLOAT_EQUAL_EPS(vertexRef.binormal.z, groupRef.binormal.z, EPS);

                if (groupEqual)
                {
                    vertexRef.resultGroup = groupId;
                    groupFound = true;
                    break;
                }
            }
            if (!groupFound) //start new group
            {
                vertexRef.resultGroup = static_cast<int32>(groups.size());
                groups.push_back(refId);
            }
        }

        if (groups.size() > 1) //different groups found - unlock vertices and update refs
        {
            groups[0] = i;
            for (int32 groupId = 1, groupSz = static_cast<int32>(groups.size()); groupId < groupSz; ++groupId)
            {
                verticesOrigin.push_back(verticesOrigin[i]);

                verticesOrigin.back().tangent = verticesFull[verticesOrigin[i].refIndices[groups[groupId]]].tangent;
                verticesOrigin.back().binormal = verticesFull[verticesOrigin[i].refIndices[groups[groupId]]].binormal;
                groups[groupId] = static_cast<int32>(verticesOrigin.size() - 1);
                verticesOrigin[groups[groupId]].refIndex = i;
            }
            for (int32 refId = 1, refSz = static_cast<int32>(verticesOrigin[i].refIndices.size()); refId < refSz; ++refId)
            {
                VertexWork& vertexRef = verticesFull[verticesOrigin[i].refIndices[refId]];
                vertexRef.refIndex = groups[vertexRef.resultGroup];
            }
        }
    }

    //copy original polygon group data and fill new tangent/binormal values
    ScopedPtr<PolygonGroup> tmpGroup(new PolygonGroup());
    tmpGroup->AllocateData(group->GetFormat(), group->GetVertexCount(), group->GetIndexCount());

    Memcpy(tmpGroup->meshData, group->meshData, group->GetVertexCount() * group->vertexStride);
    Memcpy(tmpGroup->indexArray, group->indexArray, group->GetIndexCount() * sizeof(int16));

    int32 vertexFormat = group->GetFormat() | EVF_TANGENT;
    if (precomputeBinormal)
        vertexFormat |= EVF_BINORMAL;
    group->ReleaseData();
    group->AllocateData(vertexFormat, static_cast<int32>(verticesOrigin.size()), static_cast<int32>(verticesFull.size()));

    //copy vertices
    for (uint32 i = 0, sz = static_cast<uint32>(verticesOrigin.size()); i < sz; ++i)
    {
        CopyVertex(tmpGroup, verticesOrigin[i].refIndex, group, i);
        Vector3 normal;
        group->GetNormal(i, normal);

        Vector3 tangent = verticesOrigin[i].tangent;
        tangent -= normal * DotProduct(tangent, normal);
        tangent.Normalize();
        group->SetTangent(i, tangent);
        if (precomputeBinormal)
        {
            Vector3 binormal = -verticesOrigin[i].binormal;
            binormal -= normal * DotProduct(binormal, normal);
            binormal.Normalize();
            group->SetBinormal(i, binormal);
        }
    }

    //copy indices
    for (size_t i = 0, sz = verticesFull.size(); i < sz; ++i)
        group->SetIndex(static_cast<int32>(i), verticesFull[i].refIndex);

    group->BuildBuffers();
}

SkinnedMesh* CreateHardSkinnedMesh(Entity* fromEntity, Vector<SkeletonComponent::Joint>& outJoints)
{
    using namespace MeshUtilsDetails;

    SkinnedMesh* newRenderObject = new SkinnedMesh();

    Map<SkinnedMeshWorkKey, Vector<SkinnedMeshJointWork>> collapseDataMap;

    Vector<Entity*> childrenNodes;
    fromEntity->GetChildNodes(childrenNodes);

    outJoints.resize(childrenNodes.size());

    SkinnedMesh::JointTargets jointTargets;
    for (int32 nodeIndex = 0; nodeIndex < int32(childrenNodes.size()); ++nodeIndex)
    {
        Entity* child = childrenNodes[nodeIndex];

        Matrix4 bindTransform = child->AccamulateLocalTransform(fromEntity);

        outJoints[nodeIndex].name = childrenNodes[nodeIndex]->GetName();
        outJoints[nodeIndex].uid = childrenNodes[nodeIndex]->GetName();
        outJoints[nodeIndex].bindTransform = bindTransform;
        bindTransform.GetInverse(outJoints[nodeIndex].bindTransformInv);

        RenderObject* ro = GetRenderObject(child);
        if (ro != nullptr)
        {
            uint32 jointTarget = uint32(jointTargets.size());
            jointTargets.emplace_back(nodeIndex);

            int32 batchesCount = ro->GetRenderBatchCount();
            for (int32 batchIndex = 0; batchIndex < batchesCount; ++batchIndex)
            {
                int32 lodIndex, switchIndex;
                RenderBatch* rb = ro->GetRenderBatch(batchIndex, lodIndex, switchIndex);
                SkinnedMeshWorkKey dataKey(lodIndex, switchIndex, rb->GetMaterial()->GetParent());

                collapseDataMap[dataKey].push_back(SkinnedMeshJointWork(rb, jointTarget));
            }

            ro->GetBoundingBox().GetTransformedBox(outJoints[nodeIndex].bindTransformInv, outJoints[nodeIndex].bbox);
        }
        else
        {
            outJoints[nodeIndex].bbox.Empty();
        }

        Entity* parentEntity = child->GetParent();
        if (!parentEntity || parentEntity == fromEntity)
        {
            outJoints[nodeIndex].parentIndex = SkeletonComponent::INVALID_JOINT_INDEX;
        }
        else
        {
            for (int32 i = 0; i < int32(childrenNodes.size()); ++i)
            {
                if (parentEntity == childrenNodes[i])
                {
                    outJoints[nodeIndex].parentIndex = i;
                    continue;
                }
            }
        }
    }

    Map<SkinnedMeshWorkKey, Vector<SkinnedMeshJointWork>>::iterator it = collapseDataMap.begin();
    Map<SkinnedMeshWorkKey, Vector<SkinnedMeshJointWork>>::iterator itEnd = collapseDataMap.end();
    for (; it != itEnd; ++it)
    {
        const SkinnedMeshWorkKey& key = it->first;
        Vector<SkinnedMeshJointWork>& data = it->second;

        int32 vxCount = 0;
        int32 indCount = 0;
        int32 meshFormat = data.front().batch->GetPolygonGroup()->GetFormat();
        for (int32 dataIndex = 0; dataIndex < int32(data.size()); ++dataIndex)
        {
            vxCount += data[dataIndex].batch->GetPolygonGroup()->GetVertexCount();
            indCount += data[dataIndex].batch->GetPolygonGroup()->GetIndexCount();

            DVASSERT(meshFormat == data[dataIndex].batch->GetPolygonGroup()->GetFormat(), Format("Invalid Entity: %s", fromEntity->GetName().c_str()).c_str());
        }

        PolygonGroup* polygonGroup = new PolygonGroup();
        polygonGroup->AllocateData(meshFormat | EVF_HARD_JOINTINDEX, vxCount, indCount);

        int32 vertexOffset = 0;
        int32 indexOffset = 0;
        for (int32 dataIndex = 0; dataIndex < int32(data.size()); ++dataIndex)
        {
            PolygonGroup* currentGroup = data[dataIndex].batch->GetPolygonGroup();
            int32 currentBatchVxCount = currentGroup->GetVertexCount();
            for (int32 currentBatchVxIndex = 0; currentBatchVxIndex < currentBatchVxCount; ++currentBatchVxIndex)
            {
                int32 newBatchVxIndex = vertexOffset + currentBatchVxIndex;
                CopyVertex(currentGroup, currentBatchVxIndex, polygonGroup, newBatchVxIndex);
                polygonGroup->SetHardJointIndex(newBatchVxIndex, data[dataIndex].jointIndex);
            }

            int32 currentBatchIndexCount = currentGroup->GetIndexCount();
            for (int32 currentBatchIdxIndex = 0; currentBatchIdxIndex < currentBatchIndexCount; ++currentBatchIdxIndex)
            {
                int32 index;
                currentGroup->GetIndex(currentBatchIdxIndex, index);
                polygonGroup->SetIndex(indexOffset + currentBatchIdxIndex, int16(vertexOffset + index));
            }

            vertexOffset += currentBatchVxCount;
            indexOffset += currentBatchIndexCount;
        }

        NMaterial* material = new NMaterial();
        material->SetParent(key.materialParent);
        material->AddFlag(NMaterialFlagName::FLAG_HARD_SKINNING, 1);

        RenderBatch* newBatch = new RenderBatch();
        polygonGroup->RecalcAABBox();
        polygonGroup->BuildBuffers();
        newBatch->SetPolygonGroup(polygonGroup);
        newBatch->SetMaterial(material);

        newRenderObject->AddRenderBatch(newBatch, key.lodIndex, key.switchIndex);
        newRenderObject->SetJointTargets(newBatch, jointTargets);

        material->Release();
        polygonGroup->Release();
        newBatch->Release();
    }

    return newRenderObject;
}

Vector<std::pair<PolygonGroup*, SkinnedMesh::JointTargets>> SplitSkinnedMeshGeometry(PolygonGroup* dataSource, uint32 maxJointCount)
{
    using namespace MeshUtilsDetails;

    DVASSERT(dataSource);
    DVASSERT(dataSource->GetPrimitiveType() == rhi::PRIMITIVE_TRIANGLELIST);

    int32 vertexFormat = dataSource->GetFormat();
    int32 trianglesCount = dataSource->GetPrimitiveCount();

    DVASSERT((vertexFormat & EVF_HARD_JOINTINDEX) || (vertexFormat & (EVF_JOINTINDEX | EVF_JOINTWEIGHT)));

    Vector<std::pair<PolygonGroup*, SkinnedMesh::JointTargets>> result;
    Vector<SkinnedTriangleWork> sourceTriangles(trianglesCount);

    //////////////////////////////////////////////////////////////////////////
    //Disassemble mesh to triangles and sort it

    int32 jIndex = -1, vIndex = -1;
    float32 jWeight = 0.f;
    int32 sourceMaxJointIndex = 0;
    for (int32 t = 0; t < trianglesCount; ++t)
    {
        SkinnedTriangleWork& triangle = sourceTriangles[t];
        for (int32 i = 0; i < 3; ++i)
        {
            dataSource->GetIndex(t * 3 + i, vIndex);
            triangle.indices[i] = vIndex;

            for (int32 jIndex : GetSignificantJoints(dataSource, vIndex))
            {
                triangle.usedJoints.insert(jIndex);
                sourceMaxJointIndex = Max(sourceMaxJointIndex, jIndex);
            }
        }
    }

    std::sort(sourceTriangles.begin(), sourceTriangles.end(), [](const SkinnedTriangleWork& l, const SkinnedTriangleWork& r) {
        return *l.usedJoints.rbegin() > *r.usedJoints.rbegin(); //descending order by max joint index
    });

    Set<int32> usedJointsWork;
    Set<int32> usedIndicesWork;
    Vector<int32> jointsWork;
    Vector<int32> indicesWork;
    Map<int32, int32> indicesMapWork;
    Map<int32, int32> jointsMapWork;
    Vector<SkinnedTriangleWork> triangles;
    while (sourceTriangles.size())
    {
        triangles.clear();
        usedJointsWork.clear();
        usedIndicesWork.clear();

        //////////////////////////////////////////////////////////////////////////
        //Search triangles that suit to max joints count

        for (SkinnedTriangleWork& triangle : sourceTriangles)
        {
            jointsWork.clear();
            std::set_union(usedJointsWork.begin(), usedJointsWork.end(), triangle.usedJoints.begin(), triangle.usedJoints.end(), std::back_inserter(jointsWork));
            if (uint32(jointsWork.size()) > maxJointCount)
                continue;

            usedJointsWork.insert(triangle.usedJoints.begin(), triangle.usedJoints.end());

            for (int32 ind : triangle.indices)
                usedIndicesWork.insert(ind);

            triangles.push_back(triangle);
            triangle.processed = true;
        }

        auto removed = std::remove_if(sourceTriangles.begin(), sourceTriangles.end(), [](const SkinnedTriangleWork& element) {
            return element.processed;
        });
        sourceTriangles.erase(removed, sourceTriangles.end());

        //////////////////////////////////////////////////////////////////////////
        //Fill temporary mapping data for joints and indices

        SkinnedMesh::JointTargets jointTargets;
        jointTargets.insert(jointTargets.end(), usedJointsWork.begin(), usedJointsWork.end());

        jointsMapWork.clear();
        for (int32 j = 0; j < int32(jointTargets.size()); ++j)
            jointsMapWork[jointTargets[j]] = j;

        indicesWork.clear();
        indicesWork.reserve(usedIndicesWork.size());
        indicesWork.insert(indicesWork.end(), usedIndicesWork.begin(), usedIndicesWork.end());

        indicesMapWork.clear();
        for (int32 ind = 0; ind < int32(usedIndicesWork.size()); ++ind)
            indicesMapWork[indicesWork[ind]] = ind;

        //////////////////////////////////////////////////////////////////////////
        //Fill vertex data

        int32 vertexCount = int32(usedIndicesWork.size());
        int32 indexCount = int32(triangles.size()) * 3;

        PolygonGroup* pg = new PolygonGroup();
        pg->AllocateData(vertexFormat, vertexCount, indexCount);

        for (int32 v = 0; v < vertexCount; ++v)
        {
            MeshUtils::CopyVertex(dataSource, indicesWork[v], pg, v);
            ReplaceSignificantJoints(pg, v, jointsMapWork);
        }

        //////////////////////////////////////////////////////////////////////////
        //Fill index data

        int32 iIndex = 0;
        for (const SkinnedTriangleWork& t : triangles)
        {
            for (int32 i : t.indices)
            {
                DVASSERT(indicesMapWork.count(i) != 0);
                pg->SetIndex(iIndex, uint16(indicesMapWork[i]));

                ++iIndex;
            }
        }
        DVASSERT(iIndex == indexCount);

        //////////////////////////////////////////////////////////////////////////

        result.emplace_back(pg, std::move(jointTargets));
    }

    return result;
}

PolygonGroup* CreateShadowPolygonGroup(PolygonGroup* oldPolygonGroup)
{
    using namespace MeshUtilsDetails;

    int32 numEdges = oldPolygonGroup->GetIndexCount();
    int32 oldIndexCount = oldPolygonGroup->GetIndexCount();
    EdgeMappingWork* mapping = new EdgeMappingWork[numEdges];
    int32 numMaps = 0;

    //generate adjacency
    int32 oldVertexCount = oldPolygonGroup->GetVertexCount();
    int32* adjacency = new int32[oldVertexCount];
    Memset(adjacency, -1, oldVertexCount * sizeof(int32));
    for (int32 i = 0; i < oldVertexCount; ++i)
    {
        Vector3 newFoundCoord;
        oldPolygonGroup->GetCoord(i, newFoundCoord);
        adjacency[i] = i;
        for (int32 j = 0; j < i; ++j)
        {
            Vector3 oldCoord;
            oldPolygonGroup->GetCoord(j, oldCoord);
            if (EdgeAdjacency::IsPointsEqual(newFoundCoord, oldCoord))
            {
                adjacency[i] = j;
                break;
            }
        }
    }

    PolygonGroup* newPolygonGroup = new PolygonGroup();
    newPolygonGroup->AllocateData(EVF_VERTEX | EVF_NORMAL, oldIndexCount, oldIndexCount + numEdges * 3);
    int32 nextIndex = 0;

    bool indefiniteNormals = false;

    int32 facesCount = oldIndexCount / 3;
    for (int32 f = 0; f < facesCount; ++f)
    {
        //copy old vertex data
        int32 oldIndex0, oldIndex1, oldIndex2;
        Vector3 oldPos0, oldPos1, oldPos2;
        oldPolygonGroup->GetIndex(f * 3 + 0, oldIndex0);
        oldPolygonGroup->GetCoord(oldIndex0, oldPos0);
        newPolygonGroup->SetCoord(f * 3 + 0, oldPos0);
        newPolygonGroup->SetIndex(nextIndex++, f * 3 + 0);

        oldPolygonGroup->GetIndex(f * 3 + 1, oldIndex1);
        oldPolygonGroup->GetCoord(oldIndex1, oldPos1);
        newPolygonGroup->SetCoord(f * 3 + 1, oldPos1);
        newPolygonGroup->SetIndex(nextIndex++, f * 3 + 1);

        oldPolygonGroup->GetIndex(f * 3 + 2, oldIndex2);
        oldPolygonGroup->GetCoord(oldIndex2, oldPos2);
        newPolygonGroup->SetCoord(f * 3 + 2, oldPos2);
        newPolygonGroup->SetIndex(nextIndex++, f * 3 + 2);

        //generate new normals
        Vector3 v0 = oldPos1 - oldPos0;
        Vector3 v1 = oldPos2 - oldPos0;
        Vector3 normal = v0.CrossProduct(v1);
        normal.Normalize();

        // check normals
        if (std::isnan(normal.x) || std::isnan(normal.y) || std::isnan(normal.z))
        {
            indefiniteNormals = true;

            // temporary fix indefinite normal. normal length must be equals 1
            normal.Set(1.0f, 0.0f, 0.0f);
        }

        newPolygonGroup->SetNormal(f * 3 + 0, normal);
        newPolygonGroup->SetNormal(f * 3 + 1, normal);
        newPolygonGroup->SetNormal(f * 3 + 2, normal);

        //edge 1
        int32 nIndex;
        int32 vertIndex[3] =
        {
          adjacency[oldIndex0],
          adjacency[oldIndex1],
          adjacency[oldIndex2]
        };
        nIndex = FindEdgeInMappingTable(vertIndex[0], vertIndex[1], mapping, numEdges);

        if (mapping[nIndex].oldEdge[0] == -1 && mapping[nIndex].oldEdge[1] == -1)
        {
            // No entry for this edge yet.  Initialize one.
            mapping[nIndex].oldEdge[0] = vertIndex[0];
            mapping[nIndex].oldEdge[1] = vertIndex[1];
            mapping[nIndex].newEdge[0][0] = f * 3 + 0;
            mapping[nIndex].newEdge[0][1] = f * 3 + 1;

            ++numMaps;
        }
        else
        {
            // An entry is found for this edge.  Create
            // a quad and output it.
            mapping[nIndex].newEdge[1][0] = f * 3 + 0;
            mapping[nIndex].newEdge[1][1] = f * 3 + 1;

            // First triangle
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][1]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][0]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][0]);

            // Second triangle
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][1]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][0]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][0]);

            // pMapping[nIndex] is no longer needed. Copy the last map entry
            // over and decrement the map count.
            mapping[nIndex] = mapping[numMaps - 1];
            Memset(&mapping[numMaps - 1], 0xFF, sizeof(mapping[numMaps - 1]));
            --numMaps;
        }

        //edge 2
        nIndex = FindEdgeInMappingTable(vertIndex[1], vertIndex[2], mapping, numEdges);

        if (mapping[nIndex].oldEdge[0] == -1 && mapping[nIndex].oldEdge[1] == -1)
        {
            mapping[nIndex].oldEdge[0] = vertIndex[1];
            mapping[nIndex].oldEdge[1] = vertIndex[2];
            mapping[nIndex].newEdge[0][0] = f * 3 + 1;
            mapping[nIndex].newEdge[0][1] = f * 3 + 2;

            ++numMaps;
        }
        else
        {
            mapping[nIndex].newEdge[1][0] = f * 3 + 1;
            mapping[nIndex].newEdge[1][1] = f * 3 + 2;

            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][1]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][0]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][0]);

            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][1]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][0]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][0]);

            mapping[nIndex] = mapping[numMaps - 1];
            Memset(&mapping[numMaps - 1], 0xFF, sizeof(mapping[numMaps - 1]));
            --numMaps;
        }

        //edge 3
        nIndex = FindEdgeInMappingTable(vertIndex[2], vertIndex[0], mapping, numEdges);

        if (mapping[nIndex].oldEdge[0] == -1 && mapping[nIndex].oldEdge[1] == -1)
        {
            mapping[nIndex].oldEdge[0] = vertIndex[2];
            mapping[nIndex].oldEdge[1] = vertIndex[0];
            mapping[nIndex].newEdge[0][0] = f * 3 + 2;
            mapping[nIndex].newEdge[0][1] = f * 3 + 0;

            ++numMaps;
        }
        else
        {
            mapping[nIndex].newEdge[1][0] = f * 3 + 2;
            mapping[nIndex].newEdge[1][1] = f * 3 + 0;

            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][1]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][0]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][0]);

            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][1]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[1][0]);
            newPolygonGroup->SetIndex(nextIndex++, mapping[nIndex].newEdge[0][0]);

            mapping[nIndex] = mapping[numMaps - 1];
            Memset(&mapping[numMaps - 1], 0xFF, sizeof(mapping[numMaps - 1]));
            --numMaps;
        }
    }

    int32 nextVertex = oldIndexCount;

    //patch holes
    if (numMaps > 0)
    {
        PolygonGroup* patchPolygonGroup = new PolygonGroup();
        // Make enough room in IB for the face and up to 3 quads for each patching face
        patchPolygonGroup->AllocateData(EVF_VERTEX | EVF_NORMAL, oldIndexCount + numMaps * 3, nextIndex + numMaps * 7 * 3);

        Memcpy(patchPolygonGroup->meshData, newPolygonGroup->meshData, newPolygonGroup->GetVertexCount() * newPolygonGroup->vertexStride);
        Memcpy(patchPolygonGroup->indexArray, newPolygonGroup->indexArray, newPolygonGroup->GetIndexCount() * sizeof(int16));

        SafeRelease(newPolygonGroup);
        newPolygonGroup = patchPolygonGroup;

        // Now, we iterate through the edge mapping table and
        // for each shared edge, we generate a quad.
        // For each non-shared edge, we patch the opening
        // with new faces.

        for (int32 i = 0; i < numMaps; ++i)
        {
            if (mapping[i].oldEdge[0] != -1 && mapping[i].oldEdge[1] != -1)
            {
                // If the 2nd new edge indexes is -1,
                // this edge is a non-shared one.
                // We patch the opening by creating new
                // faces.
                if (mapping[i].newEdge[1][0] == -1 || mapping[i].newEdge[1][1] == -1) // must have only one new edge
                {
                    // Find another non-shared edge that
                    // shares a vertex with the current edge.
                    for (int32 i2 = i + 1; i2 < numMaps; ++i2)
                    {
                        if (mapping[i2].oldEdge[0] != -1 && mapping[i2].oldEdge[1] != -1 // must have a valid old edge
                            && (mapping[i2].newEdge[1][0] == -1 || mapping[i2].newEdge[1][1] == -1)) // must have only one new edge
                        {
                            int32 nVertShared = 0;
                            if (mapping[i2].oldEdge[0] == mapping[i].oldEdge[1])
                                ++nVertShared;
                            if (mapping[i2].oldEdge[1] == mapping[i].oldEdge[0])
                                ++nVertShared;

                            if (2 == nVertShared)
                            {
                                // These are the last two edges of this particular
                                // opening. Mark this edge as shared so that a degenerate
                                // quad can be created for it.

                                mapping[i2].newEdge[1][0] = mapping[i].newEdge[0][0];
                                mapping[i2].newEdge[1][1] = mapping[i].newEdge[0][1];
                                break;
                            }
                            else if (1 == nVertShared)
                            {
                                // nBefore and nAfter tell us which edge comes before the other.
                                int32 nBefore, nAfter;
                                if (mapping[i2].oldEdge[0] == mapping[i].oldEdge[1])
                                {
                                    nBefore = i;
                                    nAfter = i2;
                                }
                                else
                                {
                                    nBefore = i2;
                                    nAfter = i;
                                }

                                // Found such an edge. Now create a face along with two
                                // degenerate quads from these two edges.
                                Vector3 coord0, coord1, coord2;
                                newPolygonGroup->GetCoord(mapping[nAfter].newEdge[0][1], coord0);
                                newPolygonGroup->GetCoord(mapping[nBefore].newEdge[0][1], coord1);
                                newPolygonGroup->GetCoord(mapping[nBefore].newEdge[0][0], coord2);

                                newPolygonGroup->SetCoord(nextVertex + 0, coord0);
                                newPolygonGroup->SetCoord(nextVertex + 1, coord1);
                                newPolygonGroup->SetCoord(nextVertex + 2, coord2);

                                // Recompute the normal
                                Vector3 v0 = coord1 - coord0;
                                Vector3 v1 = coord2 - coord0;
                                Vector3 normal = v0.CrossProduct(v1);
                                normal.Normalize();

                                // check normals
                                if (std::isnan(normal.x) || std::isnan(normal.y) || std::isnan(normal.z))
                                {
                                    indefiniteNormals = true;

                                    // temporary fix indefinite normal. normal length must be equals 1
                                    normal.Set(1.0f, 0.0f, 0.0f);
                                }

                                newPolygonGroup->SetNormal(nextVertex + 0, normal);
                                newPolygonGroup->SetNormal(nextVertex + 1, normal);
                                newPolygonGroup->SetNormal(nextVertex + 2, normal);

                                newPolygonGroup->SetIndex(nextIndex + 0, nextVertex + 0);
                                newPolygonGroup->SetIndex(nextIndex + 1, nextVertex + 1);
                                newPolygonGroup->SetIndex(nextIndex + 2, nextVertex + 2);

                                // 1st quad
                                newPolygonGroup->SetIndex(nextIndex + 3, mapping[nBefore].newEdge[0][1]);
                                newPolygonGroup->SetIndex(nextIndex + 4, mapping[nBefore].newEdge[0][0]);
                                newPolygonGroup->SetIndex(nextIndex + 5, nextVertex + 1);

                                newPolygonGroup->SetIndex(nextIndex + 6, nextVertex + 2);
                                newPolygonGroup->SetIndex(nextIndex + 7, nextVertex + 1);
                                newPolygonGroup->SetIndex(nextIndex + 8, mapping[nBefore].newEdge[0][0]);

                                // 2nd quad
                                newPolygonGroup->SetIndex(nextIndex + 9, mapping[nAfter].newEdge[0][1]);
                                newPolygonGroup->SetIndex(nextIndex + 10, mapping[nAfter].newEdge[0][0]);
                                newPolygonGroup->SetIndex(nextIndex + 11, nextVertex);

                                newPolygonGroup->SetIndex(nextIndex + 12, nextVertex + 1);
                                newPolygonGroup->SetIndex(nextIndex + 13, nextVertex);
                                newPolygonGroup->SetIndex(nextIndex + 14, mapping[nAfter].newEdge[0][0]);

                                // Modify mapping entry i2 to reflect the third edge
                                // of the newly added face.
                                if (mapping[i2].oldEdge[0] == mapping[i].oldEdge[1])
                                {
                                    mapping[i2].oldEdge[0] = mapping[i].oldEdge[0];
                                }
                                else
                                {
                                    mapping[i2].oldEdge[1] = mapping[i].oldEdge[1];
                                }
                                mapping[i2].newEdge[0][0] = nextVertex + 2;
                                mapping[i2].newEdge[0][1] = nextVertex;

                                // Update next vertex/index positions
                                nextVertex += 3;
                                nextIndex += 15;

                                break;
                            }
                        }
                    }
                }
                else
                {
                    // This is a shared edge.  Create the degenerate quad.
                    // First triangle
                    newPolygonGroup->SetIndex(nextIndex++, mapping[i].newEdge[0][1]);
                    newPolygonGroup->SetIndex(nextIndex++, mapping[i].newEdge[0][0]);
                    newPolygonGroup->SetIndex(nextIndex++, mapping[i].newEdge[1][0]);

                    // Second triangle
                    newPolygonGroup->SetIndex(nextIndex++, mapping[i].newEdge[1][1]);
                    newPolygonGroup->SetIndex(nextIndex++, mapping[i].newEdge[1][0]);
                    newPolygonGroup->SetIndex(nextIndex++, mapping[i].newEdge[0][0]);
                }
            }
        }
    }

    PolygonGroup* shadowDataSource = new PolygonGroup();
    shadowDataSource->AllocateData(EVF_VERTEX | EVF_NORMAL, nextVertex, nextIndex);
    Memcpy(shadowDataSource->meshData, newPolygonGroup->meshData, nextVertex * newPolygonGroup->vertexStride);
    Memcpy(shadowDataSource->indexArray, newPolygonGroup->indexArray, nextIndex * sizeof(int16));

    shadowDataSource->RecalcAABBox();

    SafeRelease(newPolygonGroup);
    SafeDeleteArray(adjacency);
    SafeDeleteArray(mapping);

    if (indefiniteNormals)
    {
        Logger::Error("Shadow data source has indefinite normals. Fix triangles with identical vertices.");
    }

    return shadowDataSource;
}

Vector<uint16> BuildSortedIndexBufferData(PolygonGroup* pg, Vector3 direction)
{
    DVASSERT(pg);
    DVASSERT(pg->GetPrimitiveType() == rhi::PRIMITIVE_TRIANGLELIST);

    struct Triangle
    {
        Vector3 sortPosition;
        Array<uint16, 3> indices;
    };

    int32 trianglesCount = pg->GetPrimitiveCount();

    Vector<uint16> indexBufferData;
    indexBufferData.reserve(pg->GetIndexCount());

    Vector<Triangle> triangles;
    triangles.reserve(trianglesCount);

    Vector3 tempVec3;
    int32 tempInd[3];
    for (int32 ti = 0; ti < trianglesCount; ++ti)
    {
        triangles.emplace_back();
        Triangle& triangle = triangles.back();

        pg->GetIndex(ti * 3 + 0, tempInd[0]);
        pg->GetIndex(ti * 3 + 1, tempInd[1]);
        pg->GetIndex(ti * 3 + 2, tempInd[2]);

        Vector4 pivot;
        if (pg->GetFormat() & EVF_PIVOT4)
            pg->GetPivot(tempInd[0], pivot);

        if (pivot.w > 0.f) //billboard
        {
            triangle.sortPosition = Vector3(pivot);
        }
        else
        {
            pg->GetCoord(tempInd[0], tempVec3);
            triangle.sortPosition += tempVec3;
            pg->GetCoord(tempInd[1], tempVec3);
            triangle.sortPosition += tempVec3;
            pg->GetCoord(tempInd[2], tempVec3);
            triangle.sortPosition += tempVec3;

            triangle.sortPosition /= 3.f;
        }

        triangle.indices[0] = uint16(tempInd[0]);
        triangle.indices[1] = uint16(tempInd[1]);
        triangle.indices[2] = uint16(tempInd[2]);
    }

    std::stable_sort(triangles.begin(), triangles.end(), [&direction](const Triangle& l, const Triangle& r) {
        return direction.DotProduct(l.sortPosition - r.sortPosition) > 0.f;
    });

    for (const Triangle& t : triangles)
        indexBufferData.insert(indexBufferData.end(), t.indices.begin(), t.indices.end());

    return indexBufferData;
}

uint32 ReleaseGeometryDataRecursive(Entity* forEntity)
{
    if (!forEntity)
        return 0;

    uint32 ret = 0;

    int32 childrenCount = forEntity->GetChildrenCount();
    for (int32 i = 0; i < childrenCount; ++i)
    {
        ret += ReleaseGeometryDataRecursive(forEntity->GetChild(i));
    }

    RenderObject* ro = GetRenderObject(forEntity);
    if (ro)
    {
        uint32 rbCount = ro->GetRenderBatchCount();
        for (uint32 i = 0; i < rbCount; ++i)
        {
            PolygonGroup* pg = ro->GetRenderBatch(i)->GetPolygonGroup();
            if (pg && pg->vertexBuffer != rhi::InvalidHandle && pg->indexBuffer != rhi::InvalidHandle)
            {
                ret += pg->ReleaseGeometryData();
            }
        }
    }

    return ret;
}
};
};
