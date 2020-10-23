#pragma once

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Reflection/Reflection.h"
#include "Render/RenderBase.h"
#include "Render/Highlevel/RenderObject.h"
#include "FileSystem/FilePath.h"
#include "MemoryManager/MemoryProfiler.h"
#include "Render/Highlevel/LandscapeSubdivision.h"
#include "Concurrency/Mutex.h"

namespace DAVA
{
/**    
    \brief Implementation of cdlod algorithm to render landscapes
    This class is base of the landscape code on all platforms
 */

class LandscapeSystem;
class FoliageSystem;
class NMaterial;
class SerializationContext;
class Heightmap;
class LandscapeSubdivision;

class Landscape : public RenderObject
{
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_LANDSCAPE)

public:
    Landscape();
    virtual ~Landscape();

    static const int32 RENDER_PARCEL_SIZE_VERTICES = 129;
    static const int32 RENDER_PARCEL_SIZE_QUADS = (RENDER_PARCEL_SIZE_VERTICES - 1);
    static const int32 RENDER_PARCEL_AND = RENDER_PARCEL_SIZE_VERTICES - 2;
    static const int32 INITIAL_INDEX_BUFFER_CAPACITY = 20000;

    static const int32 TEXTURE_SIZE_FULL_TILED = 2048;
    static const int32 CUSTOM_COLOR_TEXTURE_SIZE = 2048;

    const static FastName PARAM_TEXTURE_TILING;
    const static FastName PARAM_TILE_COLOR0;
    const static FastName PARAM_TILE_COLOR1;
    const static FastName PARAM_TILE_COLOR2;
    const static FastName PARAM_TILE_COLOR3;

    const static FastName TEXTURE_COLOR;
    const static FastName TEXTURE_TILE;
    const static FastName TEXTURE_TILEMASK;
    const static FastName TEXTURE_SPECULAR;

    const static FastName LANDSCAPE_QUALITY_NAME;
    const static FastName LANDSCAPE_QUALITY_VALUE_HIGH;

    enum RenderMode
    {
        RENDERMODE_NO_INSTANCING = 0,
        RENDERMODE_INSTANCING,
        RENDERMODE_INSTANCING_MORPHING,
    };

    //LandscapeVertex used in GetLevel0Geometry() only
    struct LandscapeVertex
    {
        Vector3 position;
        Vector2 texCoord;
    };

    //TODO: move to Beast
    DAVA_DEPRECATED(bool GetLevel0Geometry(Vector<LandscapeVertex>& vertices, Vector<int32>& indices) const);

    /**
        \brief Builds landscape from heightmap image and bounding box of this landscape block
        \param[in] landscapeBox axial-aligned bounding box of the landscape block
     */
    virtual void BuildLandscapeFromHeightmapImage(const FilePath& heightmapPathname, const AABBox3& landscapeBox);

    /**
        \brief Function to receive pathname of heightmap object
        \returns pathname of heightmap
     */
    const FilePath& GetHeightmapPathname();
    void SetHeightmapPathname(const FilePath& newPath);

    float32 GetLandscapeSize() const;
    void SetLandscapeSize(float32 newSize);

    float32 GetLandscapeHeight() const;
    void SetLandscapeHeight(float32 newHeight);

    void GetDataNodes(Set<DataNode*>& dataNodes) override;

    void Save(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Load(KeyedArchive* archive, SerializationContext* serializationContext) override;

    bool PlacePoint(const Vector3& point, Vector3& result, Vector3* normal = 0) const;
    bool GetHeightAtPoint(const Vector3& point, float&) const;

    Heightmap* GetHeightmap();
    virtual void SetHeightmap(Heightmap* height);

    NMaterial* GetMaterial();
    void SetMaterial(NMaterial* material);
    void PrepareMaterial(NMaterial* material);

    RenderObject* Clone(RenderObject* newObject) override;
    void RecalcBoundingBox() override;

    int32 GetDrawIndices() const;

    void SetFoliageSystem(FoliageSystem* _foliageSystem);

    void BindDynamicParameters(Camera* camera, RenderBatch* batch) override;
    void PrepareToRender(Camera* camera) override;

    void UpdatePart(const Rect2i& rect);
    void SetUpdatable(bool isUpdatable);
    bool IsUpdatable() const;

    void SetForceMaxSubdiv(bool force);

    void SetUseInstancing(bool useInstancing);
    bool IsUseInstancing() const;

    LandscapeSubdivision* GetSubdivision();

    RenderMode GetRenderMode() const;
    void SetRenderMode(RenderMode mode);
    void UpdateMaterialFlags();

    void RecursiveRayTrace(uint32 level, uint32 x, uint32 y, const Ray3& rayInObjectSpace, float32& resultT);
    bool RayTrace(const Ray3& rayInObjectSpace, float32& resultT);

protected:
    void AddPatchToRender(uint32 level, uint32 x, uint32 y);

    void AllocateGeometryData();
    void ReleaseGeometryData();

    void RestoreGeometry();

    void SetLandscapeSize(const Vector3& newSize);
    bool BuildHeightmap();
    void RebuildLandscape();

    /**
        Return size of heightmap object. Return if heighmap is nullptr. Used to prevent crashes in ResourceEditor
     */
    int32 GetHeightmapSize() const;

    void SetDrawWired(bool isWire);
    bool IsDrawWired() const;

    void SetDrawMorphing(bool drawMorph);
    bool IsDrawMorphing() const;

    void SetUseMorphing(bool useMorph);
    bool IsUseMorphing() const;

    void GetTangentBasis(uint32 x, uint32 y, Vector3& normalOut, Vector3& tangentOut) const;

    struct RestoreBufferData
    {
        enum eBufferType
        {
            RESTORE_BUFFER_VERTEX,
            RESTORE_BUFFER_INDEX,
            RESTORE_TEXTURE
        };

        rhi::Handle buffer;
        uint8* data;
        uint32 dataSize;
        uint32 level;
        eBufferType bufferType;
    };

    Mutex restoreDataMutex;
    Vector<RestoreBufferData> bufferRestoreData;

    FilePath heightmapPath;
    Heightmap* heightmap = nullptr;
    LandscapeSubdivision* subdivision = nullptr;

    NMaterial* landscapeMaterial = nullptr;
    FoliageSystem* foliageSystem = nullptr;

    uint32 heightmapSizePow2 = 0;
    float32 heightmapSizef = 0.f;

    uint32 drawIndices = 0;

    RenderMode renderMode = RENDERMODE_NO_INSTANCING;
    bool updatable = false;
    bool debugDrawMetrics = false;
    bool debugDrawMorphing = false;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Non-instancing render

    struct VertexNoInstancing
    {
        Vector3 position;
        Vector2 texCoord;
        Vector3 normal;
        Vector3 tangent;
    };

    void AllocateGeometryDataNoInstancing();

    void AllocateRenderBatch();
    int16 AllocateParcelVertexBuffer(uint32 x, uint32 y, uint32 size);

    void DrawLandscapeNoInstancing();
    void DrawPatchNoInstancing(uint32 level, uint32 x, uint32 y, uint32 xNegSizePow2, uint32 yNegSizePow2, uint32 xPosSizePow2, uint32 yPosSizePow2);

    void FlushQueue();

    inline uint16 GetVertexIndex(uint16 x, uint16 y);

    void ResizeIndicesBufferIfNeeded(DAVA::uint32 newSize);

    Vector<rhi::HVertexBuffer> vertexBuffers;
    std::vector<uint16> indices;

    uint32 vLayoutUIDNoInstancing = rhi::VertexLayout::InvalidUID;

    uint32 queueIndexCount = 0;
    int16 queuedQuadBuffer = 0;
    int32 flushQueueCounter = 0;

    uint32 quadsInWidthPow2 = 0;

    bool isRequireTangentBasis = false;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Instancing render

    struct VertexInstancing
    {
        Vector2 position;
        Vector2 edgeShiftDirection;
        Vector4 edgeMask;

        float32 edgeVertexIndex;
        float32 edgeMaskNull;
    };

    struct InstanceData
    {
        Vector2 patchOffset;
        float32 patchScale;
        Vector4 neighbourPatchLodOffset; // per edge: left, right, bottom, top

        //Members for morphing case
        Vector4 neighbourPatchMorph;
        float32 patchLod;
        float32 patchMorph;
        float32 centerPixelOffset;
    };

    static const int32 INSTANCE_DATA_SIZE_MORPHING = sizeof(InstanceData);
    static const int32 INSTANCE_DATA_SIZE = INSTANCE_DATA_SIZE_MORPHING - sizeof(Vector4) - 3 * sizeof(float32);

    struct InstanceDataBuffer
    {
        rhi::HVertexBuffer buffer;
        rhi::HSyncObject syncObject;
        uint32 bufferSize;
    };

    void AllocateGeometryDataInstancing();

    Texture* CreateHeightTexture(Heightmap* heightmap, RenderMode renderMode);
    Vector<Image*> CreateHeightTextureData(Heightmap* heightmap, RenderMode renderMode);

    Texture* CreateTangentTexture();
    Vector<Image*> CreateTangentBasisTextureData();

    void DrawLandscapeInstancing();
    void DrawPatchInstancing(uint32 level, uint32 xx, uint32 yy, const Vector4& neighborLevel, float32 patchMorph = 0.f, const Vector4& neighborMorph = Vector4());

    Texture* heightTexture = nullptr;
    Texture* tangentTexture = nullptr;
    bool floatHeightTexture = false;

    rhi::HVertexBuffer patchVertexBuffer;
    rhi::HIndexBuffer patchIndexBuffer;
    uint8* instanceDataPtr = nullptr;
    uint32 instanceDataMaxCount = 128; //128 instances - initial value. It's will automatic enhanced if needed.
    uint32 instanceDataSize = 0;

    Vector<InstanceDataBuffer*> freeInstanceDataBuffers;
    Vector<InstanceDataBuffer*> usedInstanceDataBuffers;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    friend class LandscapeSystem;

    DAVA_VIRTUAL_REFLECTION(Landscape, RenderObject);
};

// Inline functions
inline uint16 Landscape::GetVertexIndex(uint16 x, uint16 y)
{
    return x + y * RENDER_PARCEL_SIZE_VERTICES;
}

inline LandscapeSubdivision* Landscape::GetSubdivision()
{
    return subdivision;
}

inline Landscape::RenderMode Landscape::GetRenderMode() const
{
    return renderMode;
}
}
