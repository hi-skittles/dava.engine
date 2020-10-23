#include "Render/Highlevel/GeoDecalManager.h"
#include "Render/Highlevel/SkinnedMesh.h"
#include "Render/Highlevel/GeometryOctTree.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Reflection/Reflection.h"
#include "FileSystem/FileSystem.h"

namespace DAVA
{
struct GeoDecalManager::DecalBuildInfo
{
    AABBox3 boundingBox;
    RenderObject* renderObject = nullptr;
    RenderBatch* sourceBatch = nullptr;
    PolygonGroup* polygonGroup = nullptr;
    NMaterial* material = nullptr;
    Vector3 projectionAxis;
    Matrix4 projectionSpaceTransform;
    int32 lodIndex = -1;
    int32 switchIndex = -1;
    bool useCustomNormal = false;
    bool useCustomSpecular = false;
    bool useSkinning = false;
};

struct GeoDecalManager::DecalVertex
{
    Vector3 originalPoint;
    Vector3 actualPoint;
    Vector3 normal;
    Vector3 tangent;
    Vector3 binormal;
    Vector2 texCoord0;
    Vector2 texCoord1;
    Vector2 texCoord2;
    Vector4 decalCoord;
    int32 jointIndex = 0;

    DecalVertex(const DecalVertex& r)
    {
        memcpy(this, &r, sizeof(DecalVertex));
    }

    DecalVertex(DecalVertex&& r)
    {
        memcpy(this, &r, sizeof(DecalVertex));
    }

    DecalVertex& operator=(const DecalVertex& r)
    {
        memcpy(this, &r, sizeof(DecalVertex));
        return *this;
    }

    DecalVertex& operator=(DecalVertex&& r)
    {
        memcpy(this, &r, sizeof(DecalVertex));
        return *this;
    }
};

GeoDecalManager::BuiltDecal::BuiltDecal(BuiltDecal&& r)
    : sourceObject(r.sourceObject)
    , batchProvider(r.batchProvider)
{
    r.sourceObject = nullptr;
    r.batchProvider = nullptr;
}

GeoDecalManager::BuiltDecal& GeoDecalManager::BuiltDecal::operator=(BuiltDecal&& r)
{
    sourceObject = r.sourceObject;
    batchProvider = r.batchProvider;
    r.sourceObject = nullptr;
    r.batchProvider = nullptr;
    return *this;
}

GeoDecalManager::BuiltDecal::~BuiltDecal()
{
    DVASSERT(batchProvider->GetRetainCount() == 1);
    SafeRelease(sourceObject);
    SafeRelease(batchProvider);
}

class GeoDecalRenderBatchProvider : public RenderBatchProvider
{
public:
    ~GeoDecalRenderBatchProvider()
    {
        for (const RenderBatchWithOptions& b : batches)
        {
            RenderBatch* renderBatch = b.renderBatch;
            SafeRelease(renderBatch);
        }
    }

    const Vector<RenderBatchWithOptions> GetRenderBatches() const
    {
        return batches;
    }

    template <class... A>
    void EmplaceRenderBatch(A&&... args)
    {
        batches.emplace_back(std::forward<A>(args)...);
        SafeRetain(batches.back().renderBatch);
    }

private:
    Vector<RenderBatchWithOptions> batches;
};

const GeoDecalManager::Decal GeoDecalManager::InvalidDecal = nullptr;

GeoDecalManager::~GeoDecalManager()
{
    for (auto& d : builtDecals)
    {
        UnregisterDecal(d.first);
    }
    builtDecals.clear();
}

GeoDecalManager::Decal GeoDecalManager::BuildDecal(const DecalConfig& config, const Matrix4& decalWorldTransform, RenderObject* ro)
{
    ++decalCounter;

    uintptr_t thisId = reinterpret_cast<uintptr_t>(this);
    Decal decal = reinterpret_cast<Decal>(decalCounter ^ thisId);
    // todo : use something better for decal id

    AABBox3 decalBox = config.GetBoundingBox();

    AABBox3 worldSpaceBox;
    decalBox.GetTransformedBox(decalWorldTransform, worldSpaceBox);

    uint8 boxCornersData[8 * sizeof(Vector3)];
    Vector3* boxCorners = reinterpret_cast<Vector3*>(boxCornersData);
    decalBox.GetCorners(boxCorners);

    Matrix4 worldToObject = decalWorldTransform * ro->GetInverseWorldTransform();

    Vector3 side = MultiplyVectorMat3x3(Vector3(-1.0f, 0.0f, 0.0f), worldToObject);
    Vector3 up = MultiplyVectorMat3x3(Vector3(0.0f, 1.0f, 0.0f), worldToObject);
    Vector3 dir = MultiplyVectorMat3x3(Vector3(0.0f, 0.0f, -1.0f), worldToObject);

    Vector3 boxMin = Vector3(+std::numeric_limits<float>::max(), +std::numeric_limits<float>::max(), +std::numeric_limits<float>::max());
    Vector3 boxMax = Vector3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    Matrix4 view = Matrix4::IDENTITY;
    view._data[0][0] = side.x;
    view._data[0][1] = up.x;
    view._data[0][2] = dir.x;
    view._data[1][0] = side.y;
    view._data[1][1] = up.y;
    view._data[1][2] = dir.y;
    view._data[2][0] = side.z;
    view._data[2][1] = up.z;
    view._data[2][2] = dir.z;

    for (uint32 i = 0; i < 8; ++i)
    {
        Vector3 objectPos = boxCorners[i] * worldToObject;
        Vector3 t = MultiplyVectorMat3x3(objectPos, view);
        boxMin.x = std::min(boxMin.x, t.x);
        boxMin.y = std::min(boxMin.y, t.y);
        boxMin.z = std::min(boxMin.z, t.z);
        boxMax.x = std::max(boxMax.x, t.x);
        boxMax.y = std::max(boxMax.y, t.y);
        boxMax.z = std::max(boxMax.z, t.z);
    }

    // notice flip "-boxMax.z, -boxMin.z" in znear/zfar parameters
    // this is done because BuildOrtho building matrix for NDC, with z direction reversed
    // and here we expecting boxMin mapping to -1 -1 -1 and boxMax to +1 +1 +1
    Matrix4 proj;
    proj.BuildOrtho(boxMin.x, boxMax.x, boxMin.y, boxMax.y, -boxMax.z, -boxMin.z, false);

    DecalBuildInfo info;
    info.renderObject = ro;
    info.projectionAxis = dir;
    info.projectionSpaceTransform = view * proj;
    info.useCustomNormal = FileSystem::Instance()->Exists(config.normal);
    info.useCustomSpecular = FileSystem::Instance()->Exists(config.specular);
    info.useSkinning = ro->GetType() == RenderObject::TYPE_SKINNED_MESH;

    worldSpaceBox.GetTransformedBox(ro->GetInverseWorldTransform(), info.boundingBox);

    BuiltDecal& builtDecal = builtDecals[decal];
    {
        GeoDecalRenderBatchProvider* decalBatchProvider = new GeoDecalRenderBatchProvider();
        builtDecal.sourceObject = SafeRetain(ro);
        builtDecal.batchProvider = decalBatchProvider;

        for (uint32 i = 0, e = ro->GetRenderBatchCount(); i < e; ++i)
        {
            int32 lodIndex = -1;
            int32 switchIndex = -1;
            info.sourceBatch = ro->GetRenderBatch(i, lodIndex, switchIndex);
            info.polygonGroup = info.sourceBatch->GetPolygonGroup();
            info.material = info.sourceBatch->GetMaterial();
            info.lodIndex = lodIndex;
            info.switchIndex = switchIndex;
            BuildDecal(info, config, decalBatchProvider);
        }
    }
    RegisterDecal(decal);

    return decal;
}

void GeoDecalManager::DeleteDecal(Decal decal)
{
    UnregisterDecal(decal);
    builtDecals.erase(decal);
}

void GeoDecalManager::RegisterDecal(Decal decal)
{
    DVASSERT(builtDecals.count(decal) > 0);

    const BuiltDecal& builtDecal = builtDecals[decal];
    builtDecal.sourceObject->AddRenderBatchProvider(builtDecal.batchProvider);
}

void GeoDecalManager::UnregisterDecal(Decal decal)
{
    DVASSERT(builtDecals.count(decal) > 0);

    const BuiltDecal& builtDecal = builtDecals[decal];
    builtDecal.sourceObject->RemoveRenderBatchProvider(builtDecal.batchProvider);
}

void GeoDecalManager::RemoveRenderObject(RenderObject* ro)
{
    for (const auto& b : builtDecals)
    {
        if (b.second.sourceObject == ro)
        {
            UnregisterDecal(b.first);
        }
    }
}

#define MAX_CLIPPED_POLYGON_CAPACITY 9
#define PLANE_THICKNESS_EPSILON 0.00001f

void GeoDecalManager::AddVerticesToGeometry(const DecalBuildInfo& info, const DecalConfig& config, DecalVertex* points, DecalVertex* points_tmp, Vector<uint8>& buffer)
{
    const AABBox3 clipSpaceBox = AABBox3(Vector3(0.0f, 0.0f, 0.0f), 2.0f);

    uint32_t numPoints = 3;
    points[0].actualPoint = points[0].actualPoint * info.projectionSpaceTransform;
    points[1].actualPoint = points[1].actualPoint * info.projectionSpaceTransform;
    points[2].actualPoint = points[2].actualPoint * info.projectionSpaceTransform;

    float minU = 1.0f;
    float maxU = 0.0f;
    for (uint32 i = 0; i < numPoints; ++i)
    {
        if (config.mapping == Mapping::SPHERICAL)
        {
            Vector3 p = points[i].actualPoint;
            p.Normalize();
            points[i].decalCoord.x = -std::atan2(p.y, p.x);
            points[i].decalCoord.y = (std::asin(p.z) + 0.5f * PI) / PI;
            minU = std::min(minU, points[i].decalCoord.x);
            maxU = std::max(maxU, points[i].decalCoord.x);
        }
        else if (config.mapping == Mapping::CYLINDRICAL)
        {
            points[i].decalCoord.x = -std::atan2(points[i].actualPoint.y, points[i].actualPoint.x);
            points[i].decalCoord.y = points[i].actualPoint.z * 0.5f + 0.5f;
            minU = std::min(minU, points[i].decalCoord.x);
            maxU = std::max(maxU, points[i].decalCoord.x);
        }
        else
        {
            points[i].decalCoord.x = points[i].actualPoint.x * 0.5f + 0.5f;
            points[i].decalCoord.y = points[i].actualPoint.y * 0.5f + 0.5f;
        }
    }

    if (config.mapping != Mapping::PLANAR)
    {
        bool edgeTriangle = std::abs(minU - maxU) >= PI;
        for (uint32 i = 0; i < numPoints; ++i)
        {
            float wrap = (edgeTriangle && (points[i].decalCoord.x < 0.0f)) ? 2.0f * PI : 0.0f;
            points[i].decalCoord.x = (points[i].decalCoord.x + PI + wrap) / (2.0f * PI);
        }
    }

    ClipToBoundingBox(points, points_tmp, &numPoints, clipSpaceBox);

    if (numPoints >= 3)
    {
        for (uint32 i = 0; i < numPoints; ++i)
        {
            points[i].decalCoord.x = points[i].decalCoord.x * config.uvScale.x + config.uvOffset.x;
            points[i].decalCoord.y = points[i].decalCoord.y * config.uvScale.y + config.uvOffset.y;
        }
        size_t offset = buffer.size();
        buffer.resize(offset + 3 * (numPoints - 2) * sizeof(DecalVertex));
        DecalVertex* decalVertexPtr = reinterpret_cast<DecalVertex*>(buffer.data() + offset);
        for (uint32 i = 0; i + 2 < numPoints; ++i)
        {
            *decalVertexPtr++ = points[0];
            *decalVertexPtr++ = points[i + 1];
            *decalVertexPtr++ = points[i + 2];
        }
    }
}

void GeoDecalManager::GetStaticMeshGeometry(const DecalBuildInfo& info, const DecalConfig& config, Vector<uint8>& buffer)
{
    uint8 decalVertexData[MAX_CLIPPED_POLYGON_CAPACITY * sizeof(DecalVertex)] = {};
    DecalVertex* points = reinterpret_cast<DecalVertex*>(decalVertexData);

    uint8 decalVertexData_tmp[MAX_CLIPPED_POLYGON_CAPACITY * sizeof(DecalVertex)] = {};
    DecalVertex* points_tmp = reinterpret_cast<DecalVertex*>(decalVertexData_tmp);

    Vector<uint16> triangles;
    triangles.reserve(512);
    info.polygonGroup->GetGeometryOctTree()->GetTrianglesInBox(info.boundingBox, triangles);

    int32 geometryFormat = info.polygonGroup->GetFormat();

    for (uint16 triangleIndex : triangles)
    {
        uint16 idx[3];
        info.polygonGroup->GetTriangleIndices(3 * triangleIndex, idx);
        info.polygonGroup->GetCoord(idx[0], points[0].originalPoint);
        info.polygonGroup->GetCoord(idx[1], points[1].originalPoint);
        info.polygonGroup->GetCoord(idx[2], points[2].originalPoint);
        points[0].actualPoint = points[0].originalPoint;
        points[1].actualPoint = points[1].originalPoint;
        points[2].actualPoint = points[2].originalPoint;

        if (geometryFormat & EVF_TEXCOORD0)
        {
            info.polygonGroup->GetTexcoord(0, idx[0], points[0].texCoord0);
            info.polygonGroup->GetTexcoord(0, idx[1], points[1].texCoord0);
            info.polygonGroup->GetTexcoord(0, idx[2], points[2].texCoord0);
        }
        if (geometryFormat & EVF_TEXCOORD1)
        {
            info.polygonGroup->GetTexcoord(1, idx[0], points[0].texCoord1);
            info.polygonGroup->GetTexcoord(1, idx[1], points[1].texCoord1);
            info.polygonGroup->GetTexcoord(1, idx[2], points[2].texCoord1);
        }
        if (geometryFormat & EVF_TEXCOORD2)
        {
            info.polygonGroup->GetTexcoord(2, idx[0], points[0].texCoord2);
            info.polygonGroup->GetTexcoord(2, idx[1], points[1].texCoord2);
            info.polygonGroup->GetTexcoord(2, idx[2], points[2].texCoord2);
        }
        if (geometryFormat & EVF_NORMAL)
        {
            info.polygonGroup->GetNormal(idx[0], points[0].normal);
            info.polygonGroup->GetNormal(idx[1], points[1].normal);
            info.polygonGroup->GetNormal(idx[2], points[2].normal);
        }
        if (geometryFormat & EVF_TANGENT)
        {
            info.polygonGroup->GetTangent(idx[0], points[0].tangent);
            info.polygonGroup->GetTangent(idx[1], points[1].tangent);
            info.polygonGroup->GetTangent(idx[2], points[2].tangent);
        }
        if (geometryFormat & EVF_BINORMAL)
        {
            info.polygonGroup->GetBinormal(idx[0], points[0].binormal);
            info.polygonGroup->GetBinormal(idx[1], points[1].binormal);
            info.polygonGroup->GetBinormal(idx[2], points[2].binormal);
        }
        Vector3 nrm = (points[1].actualPoint - points[0].actualPoint).CrossProduct(points[2].actualPoint - points[0].actualPoint);
        if ((config.mapping != Mapping::PLANAR) || (nrm.DotProduct(info.projectionAxis) < -std::numeric_limits<float>::epsilon()))
        {
            AddVerticesToGeometry(info, config, points, points_tmp, buffer);
        }
    }
}

void GeoDecalManager::GetSkinnedMeshGeometry(const DecalBuildInfo& info, const DecalConfig& config, Vector<uint8>& buffer)
{
    SkinnedMesh* mesh = static_cast<SkinnedMesh*>(info.renderObject);
    const SkinnedMesh::JointTargetsData& jointTargetsData = mesh->GetJointTargetsData(info.sourceBatch);

    const AABBox3 clipSpaceBox = AABBox3(Vector3(0.0f, 0.0f, 0.0f), 2.0f);

    uint8 decalVertexData[MAX_CLIPPED_POLYGON_CAPACITY * sizeof(DecalVertex)];
    DecalVertex* points = reinterpret_cast<DecalVertex*>(decalVertexData);
    uint8 decalVertexData_tmp[MAX_CLIPPED_POLYGON_CAPACITY * sizeof(DecalVertex)];
    DecalVertex* points_tmp = reinterpret_cast<DecalVertex*>(decalVertexData_tmp);

    int32 geometryFormat = info.polygonGroup->GetFormat();

    uint32 triangleCount = static_cast<uint32>(info.polygonGroup->GetIndexCount() / 3);
    for (uint32 triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
    {
        uint16 idx[3];
        info.polygonGroup->GetTriangleIndices(3 * triangleIndex, idx);
        for (int32 j = 0; j < 3; ++j)
        {
            info.polygonGroup->GetCoord(idx[j], points[j].originalPoint);

            if (geometryFormat & EVF_TEXCOORD0)
                info.polygonGroup->GetTexcoord(0, idx[j], points[j].texCoord0);
            if (geometryFormat & EVF_TEXCOORD1)
                info.polygonGroup->GetTexcoord(1, idx[j], points[j].texCoord1);
            if (geometryFormat & EVF_TEXCOORD2)
                info.polygonGroup->GetTexcoord(2, idx[j], points[j].texCoord2);
            if (geometryFormat & EVF_NORMAL)
                info.polygonGroup->GetNormal(idx[j], points[j].normal);
            if (geometryFormat & EVF_TANGENT)
                info.polygonGroup->GetTangent(idx[j], points[j].tangent);
            if (geometryFormat & EVF_BINORMAL)
                info.polygonGroup->GetBinormal(idx[j], points[j].binormal);

            info.polygonGroup->GetHardJointIndex(idx[j], points[j].jointIndex);

            Vector4 weightedVertexPosition = jointTargetsData.positions[points[j].jointIndex];
            Vector4 weightedVertexQuaternion = jointTargetsData.quaternions[points[j].jointIndex];
            Vector3 tmpVec = 2.0f * weightedVertexQuaternion.GetVector3().CrossProduct(points[j].originalPoint);
            points[j].actualPoint = weightedVertexPosition.GetVector3() + weightedVertexPosition.w *
            (points[j].originalPoint + weightedVertexQuaternion.w * tmpVec + weightedVertexQuaternion.GetVector3().CrossProduct(tmpVec));
        }

        if (Intersection::BoxTriangle(info.boundingBox, points[2].actualPoint, points[1].actualPoint, points[0].actualPoint))
        {
            Vector3 nrm = (points[1].actualPoint - points[0].actualPoint).CrossProduct(points[2].actualPoint - points[0].actualPoint);
            if ((config.mapping != Mapping::PLANAR) || (nrm.DotProduct(info.projectionAxis) < -std::numeric_limits<float>::epsilon()))
            {
                AddVerticesToGeometry(info, config, points, points_tmp, buffer);
            }
        }
    }
}

bool GeoDecalManager::BuildDecal(const DecalBuildInfo& info, const DecalConfig& config, RenderBatchProvider* batchProvider)
{
    if (info.polygonGroup == nullptr)
        return false;

    const FastName& effectiveFxName = info.material->GetEffectiveFXName();

    if ((effectiveFxName == NMaterialName::SILHOUETTE) || (effectiveFxName == NMaterialName::SHADOW_VOLUME))
        return false;

    int32 geometryFormat = info.polygonGroup->GetFormat();

    Vector<uint8_t> buffer;
    buffer.reserve(3 * sizeof(DecalVertex) * info.polygonGroup->GetIndexCount());
    if (info.useSkinning)
    {
        if ((geometryFormat & EVF_JOINTINDEX) || (geometryFormat & EVF_HARD_JOINTINDEX))
        {
            GetSkinnedMeshGeometry(info, config, buffer);
        }
        else
        {
            // we are no supporting soft skinning yet
            return false;
        }
    }
    else
    {
        GetStaticMeshGeometry(info, config, buffer);
    }

    if (buffer.empty())
        return false;

    uint32 decalVertexCount = static_cast<uint32>(buffer.size() / sizeof(DecalVertex));
    const DecalVertex* decalVertexPtr = reinterpret_cast<DecalVertex*>(buffer.data());

    ScopedPtr<PolygonGroup> newPolygonGroup(new PolygonGroup());
    newPolygonGroup->AllocateData(geometryFormat | EVF_TEXCOORD3, decalVertexCount, decalVertexCount);
    for (uint32 index = 0; index < decalVertexCount; ++index, ++decalVertexPtr)
    {
        const DecalVertex& c = (*decalVertexPtr);

        newPolygonGroup->SetCoord(index, c.originalPoint);
        newPolygonGroup->SetIndex(index, static_cast<int16>(index));
        newPolygonGroup->SetTexcoord(3, index, Vector2(c.decalCoord.x, c.decalCoord.y));

        if (geometryFormat & EVF_TEXCOORD0)
            newPolygonGroup->SetTexcoord(0, index, info.useCustomNormal ? Vector2(c.decalCoord.x, c.decalCoord.y) : c.texCoord0);

        if (geometryFormat & EVF_TEXCOORD1)
            newPolygonGroup->SetTexcoord(1, index, c.texCoord1);

        if (geometryFormat & EVF_TEXCOORD2)
            newPolygonGroup->SetTexcoord(2, index, c.texCoord2);

        if (geometryFormat & EVF_NORMAL)
            newPolygonGroup->SetNormal(index, c.normal);

        if (geometryFormat & EVF_TANGENT)
            newPolygonGroup->SetTangent(index, c.tangent);

        if (geometryFormat & EVF_BINORMAL)
            newPolygonGroup->SetBinormal(index, c.binormal);

        if (geometryFormat & EVF_HARD_JOINTINDEX)
            newPolygonGroup->SetHardJointIndex(index, c.jointIndex);
    }
    newPolygonGroup->BuildBuffers();

    /*
     * Process material
     */
    String baseFXName(info.material->GetEffectiveFXName().c_str());
    std::transform(baseFXName.begin(), baseFXName.end(), baseFXName.begin(), ::tolower);

    String fxFileName = (baseFXName.find("normalizedblinnphong") != String::npos) ? "NormalizedBlinnPhongAllQualities.GeoDecal.material" : "GeoDecal.material";
    FilePath fxName = (config.overridenMaterialsPath.IsEmpty() ? String("~res:/Materials/") : config.overridenMaterialsPath) + fxFileName;

    ScopedPtr<NMaterial> material(new NMaterial());
    material->SetParent(info.material);
    material->SetQualityGroup(info.material->GetQualityGroup());
    material->SetFXName(FastName(fxName.GetStringValue().c_str()));
    material->SetMaterialName(FastName("GeoDecal"));
    material->SetRuntime(true);

    if (baseFXName.find("lightmap") != String::npos)
    {
        material->AddFlag(FastName("MATERIAL_LIGHTMAP"), 1);
    }

    if (material->GetEffectiveFlagValue(NMaterialFlagName::FLAG_TILED_DECAL_MASK) != 0)
    {
        // disable decals over decals
        material->AddFlag(NMaterialFlagName::FLAG_TILED_DECAL_MASK, 0);
    }

    ScopedPtr<Texture> geoDecalTexture(Texture::CreateFromFile(config.albedo));
    material->AddTexture(NMaterialTextureName::TEXTURE_ALBEDO, geoDecalTexture);

    if (info.useCustomNormal)
    {
        ScopedPtr<Texture> customNormal(Texture::CreateFromFile(config.normal));
        material->AddTexture(NMaterialTextureName::TEXTURE_NORMAL, customNormal);
    }

    if (info.useCustomSpecular)
    {
        static const float exactlyOne = 1.0f;
        ScopedPtr<Texture> customSpecular(Texture::CreateFromFile(config.specular));
        material->AddTexture(NMaterialTextureName::TEXTURE_SPECULAR, customSpecular);
        material->AddFlag(NMaterialFlagName::FLAG_GEO_DECAL_SPECULAR, 1);
        material->AddProperty(NMaterialParamName::PARAM_SPECULAR_SCALE, &exactlyOne, rhi::ShaderProp::TYPE_FLOAT1);
    }
    else
    {
        material->AddProperty(NMaterialParamName::PARAM_SPECULAR_SCALE, &config.specularScale, rhi::ShaderProp::TYPE_FLOAT1);
    }

    ScopedPtr<RenderBatch> batch(new RenderBatch());
    batch->SetPolygonGroup(newPolygonGroup);
    batch->SetMaterial(material);
    batch->UpdateAABBoxFromSource();

    if (info.sourceBatch->GetSortingKey() > 0)
        batch->SetSortingKey(info.sourceBatch->GetSortingKey() - 1);

    static_cast<GeoDecalRenderBatchProvider*>(batchProvider)->EmplaceRenderBatch(batch, info.lodIndex, info.switchIndex);

    if (info.useSkinning)
    {
        SkinnedMesh* mesh = static_cast<SkinnedMesh*>(info.renderObject);
        mesh->SetJointTargets(batch, mesh->GetJointTargets(info.sourceBatch));
    }

    if (config.debugOverlayEnabled)
    {
        /*
         * Process debug material
         */
        ScopedPtr<NMaterial> debugMaterial(material->Clone());
        debugMaterial->SetFXName(FastName("~res:/Materials/GeoDecal.Debug.material"));

        ScopedPtr<RenderBatch> debugBatch(batch->Clone());
        debugBatch->SetMaterial(debugMaterial);
        debugBatch->UpdateAABBoxFromSource();
        static_cast<GeoDecalRenderBatchProvider*>(batchProvider)->EmplaceRenderBatch(debugBatch, info.lodIndex, info.switchIndex);

        if (info.useSkinning)
        {
            SkinnedMesh* mesh = static_cast<SkinnedMesh*>(info.renderObject);
            mesh->SetJointTargets(debugBatch, mesh->GetJointTargets(info.sourceBatch));
        }
    }

    return true;
}

int32_t GeoDecalManager::Classify(int32_t sign, Vector3::eAxis axis, const Vector3& c_v, const DecalVertex& p_v)
{
    float32 d = static_cast<float>(sign) * (p_v.actualPoint[axis] - c_v[axis]);
    return static_cast<int>(d > PLANE_THICKNESS_EPSILON) - static_cast<int>(d < -PLANE_THICKNESS_EPSILON);
}

void GeoDecalManager::Lerp(float t, const DecalVertex& v1, const DecalVertex& v2, DecalVertex& result)
{
    result.jointIndex = (t >= 0.5f) ? v2.jointIndex : v1.jointIndex;

#define LERP_IMPL(var) result.var = v1.var + (v2.var - v1.var) * t;
    LERP_IMPL(actualPoint);
    LERP_IMPL(originalPoint);
    LERP_IMPL(normal);
    LERP_IMPL(tangent);
    LERP_IMPL(binormal);
    LERP_IMPL(texCoord0);
    LERP_IMPL(texCoord1);
    LERP_IMPL(texCoord2);
    LERP_IMPL(decalCoord);
#undef LERP_IMPL
}

void GeoDecalManager::ClipToPlane(DecalVertex* p_vs, DecalVertex* new_p_vs, uint32_t* nb_p_vs, int32_t sign, Vector3::eAxis axis, const Vector3& c_v)
{
    uint32_t nb = *nb_p_vs;
    if (nb <= 1)
    {
        *nb_p_vs = 0;
        return;
    }

    uint32_t k = 0;
    float fsign = static_cast<float>(sign);
    bool polygonCompletelyOnPlane = true; // polygon is fully located on clipping plane

    const DecalVertex* p_v1 = &(p_vs[nb - 1]);
    float32 d1f = fsign * (p_v1->actualPoint[axis] - c_v[axis]);
    int32_t d1 = static_cast<int>(d1f > PLANE_THICKNESS_EPSILON) - static_cast<int>(d1f < -PLANE_THICKNESS_EPSILON);
    for (uint32_t j = 0; j < nb; ++j)
    {
        const DecalVertex& p_v2 = p_vs[j];
        float32 d2f = fsign * (p_v2.actualPoint[axis] - c_v[axis]);
        int32_t d2 = static_cast<int>(d2f > PLANE_THICKNESS_EPSILON) - static_cast<int>(d2f < -PLANE_THICKNESS_EPSILON);
        if (d2 < 0)
        {
            polygonCompletelyOnPlane = false;
            if (d1 > 0)
            {
                const float alpha = (p_v2.actualPoint[axis] - c_v[axis]) / (p_v2.actualPoint[axis] - p_v1->actualPoint[axis]);
                Lerp(alpha, p_v2, *p_v1, new_p_vs[k]);
                ++k;
            }
            else if (d1 == 0 && (k == 0 || new_p_vs[k - 1].actualPoint != p_v1->actualPoint))
            {
                new_p_vs[k++] = *p_v1;
            }
        }
        else if (d2 > 0)
        {
            polygonCompletelyOnPlane = false;
            if (d1 < 0)
            {
                const float alpha = (p_v2.actualPoint[axis] - c_v[axis]) / (p_v2.actualPoint[axis] - p_v1->actualPoint[axis]);
                Lerp(alpha, p_v2, *p_v1, new_p_vs[k]);
                ++k;
            }
            else if (d1 == 0 && (k == 0 || new_p_vs[k - 1].actualPoint != p_v1->actualPoint))
            {
                new_p_vs[k++] = *p_v1;
            }
            new_p_vs[k++] = p_v2;
        }
        else if (d1 != 0)
        {
            new_p_vs[k++] = p_v2;
        }

        p_v1 = &p_v2;
        d1 = d2;
    }

    if (!polygonCompletelyOnPlane)
    {
        *nb_p_vs = k;
    }
}

void GeoDecalManager::ClipToBoundingBox(DecalVertex* p_vs, DecalVertex* p_out, uint32_t* nb_p_vs, const AABBox3& clipper)
{
    ClipToPlane(p_vs, p_out, nb_p_vs, 1, Vector3::eAxis::AXIS_X, clipper.min);
    ClipToPlane(p_out, p_vs, nb_p_vs, -1, Vector3::eAxis::AXIS_X, clipper.max);
    ClipToPlane(p_vs, p_out, nb_p_vs, 1, Vector3::eAxis::AXIS_Y, clipper.min);
    ClipToPlane(p_out, p_vs, nb_p_vs, -1, Vector3::eAxis::AXIS_Y, clipper.max);
    ClipToPlane(p_vs, p_out, nb_p_vs, 1, Vector3::eAxis::AXIS_Z, clipper.min);
    ClipToPlane(p_out, p_vs, nb_p_vs, -1, Vector3::eAxis::AXIS_Z, clipper.max);
}

#undef MAX_CLIPPED_POLYGON_CAPACITY
#undef PLANE_THICKNESS_EPSILON
}
