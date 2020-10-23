#pragma once

#include "Functional/Function.h"
#include "FileSystem/FilePath.h"
#include "Math/AABBox3.h"
#include <atomic>

namespace DAVA
{
class Texture;
class RenderSystem;
class RenderObject;
class RenderBatch;
class RenderBatchProvider;
class GeoDecalManager
{
public:
    enum Mapping : uint32
    {
        PLANAR,
        SPHERICAL,
        CYLINDRICAL,

        COUNT
    };

    struct DecalConfig
    {
        Vector3 dimensions = Vector3(1.0f, 1.0f, 1.0f);
        Mapping mapping = Mapping::PLANAR;
        FilePath albedo;
        FilePath normal;
        FilePath specular;
        FilePath overridenMaterialsPath;
        Vector2 uvOffset;
        Vector2 uvScale = Vector2(1.0f, 1.0f);
        float specularScale = 1.0f;
        bool debugOverlayEnabled = false;

        bool operator==(const DecalConfig&) const;
        bool operator!=(const DecalConfig&) const;
        void invalidate();

        AABBox3 GetBoundingBox() const
        {
            Vector3 half = 0.5f * dimensions;
            return AABBox3(-half, half);
        }
    };

    using Decal = struct
    {
        uint32 key = 0xDECAAAAA;
    }*;

    static const Decal InvalidDecal;

public:
    GeoDecalManager() = default;
    ~GeoDecalManager();

    Decal BuildDecal(const DecalConfig& config, const Matrix4& decalWorldTransform, RenderObject* object);
    void DeleteDecal(Decal decal);

    /*
     * Removes all decals associated with provided RenderObject
     */
    void RemoveRenderObject(RenderObject*);

private:
    struct DecalVertex;
    struct DecalBuildInfo;

    struct BuiltDecal
    {
        RenderObject* sourceObject = nullptr;
        RenderBatchProvider* batchProvider = nullptr;

        BuiltDecal() = default;
        BuiltDecal(BuiltDecal&&);
        BuiltDecal& operator=(BuiltDecal&&);
        ~BuiltDecal();

        BuiltDecal(const BuiltDecal& r) = delete;
        BuiltDecal& operator=(const BuiltDecal&) = delete;
    };

    void RegisterDecal(Decal decal);
    void UnregisterDecal(Decal decal);

    bool BuildDecal(const DecalBuildInfo& info, const DecalConfig& config, RenderBatchProvider* provider);
    void ClipToPlane(DecalVertex* p_vs, DecalVertex* p_vs_out, uint32* nb_p_vs, int32 sign, Vector3::eAxis axis, const Vector3& c_v);
    void ClipToBoundingBox(DecalVertex* p_vs, DecalVertex* p_out, uint32* nb_p_vs, const AABBox3& clipper);
    int32 Classify(int32 sign, Vector3::eAxis axis, const Vector3& c_v, const DecalVertex& p_v);
    void Lerp(float t, const DecalVertex& v1, const DecalVertex& v2, DecalVertex& result);

    void GetStaticMeshGeometry(const DecalBuildInfo& info, const DecalConfig& config, Vector<uint8>& buffer);
    void GetSkinnedMeshGeometry(const DecalBuildInfo& info, const DecalConfig& config, Vector<uint8>& buffer);
    void AddVerticesToGeometry(const DecalBuildInfo& info, const DecalConfig& config, DecalVertex* points, DecalVertex* points_tmp, Vector<uint8>& buffer);

private:
    Map<Decal, BuiltDecal> builtDecals;
    std::atomic<uintptr_t> decalCounter{ 0 };
};

inline bool GeoDecalManager::DecalConfig::operator==(const GeoDecalManager::DecalConfig& r) const
{
    return (dimensions == r.dimensions) && (albedo == r.albedo) && (normal == r.normal) && (specular == r.specular) &&
    (mapping == r.mapping) && (uvOffset == r.uvOffset) && (uvScale == r.uvScale) && (debugOverlayEnabled == r.debugOverlayEnabled) &&
    (specularScale == r.specularScale);
}

inline bool GeoDecalManager::DecalConfig::operator!=(const GeoDecalManager::DecalConfig& r) const
{
    return (dimensions != r.dimensions) || (albedo != r.albedo) || (normal != r.normal) || (specular != r.specular) ||
    (mapping != r.mapping) || (uvOffset != r.uvOffset) || (uvScale != r.uvScale) || (debugOverlayEnabled != r.debugOverlayEnabled) ||
    (specularScale != r.specularScale);
}

inline void GeoDecalManager::DecalConfig::invalidate()
{
    dimensions = Vector3(0.0f, 0.0f, 0.0f);
    albedo = FilePath();
    normal = FilePath();
    specular = FilePath();
    mapping = Mapping::PLANAR;
    uvOffset = Vector2(0.0f, 0.0f);
    uvScale = Vector2(1.0f, 1.0f);
}
}
