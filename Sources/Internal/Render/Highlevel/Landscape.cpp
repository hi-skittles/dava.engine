#include "Time/SystemTimer.h"
#include "FileSystem/FileSystem.h"
#include "Utils/StringFormat.h"
#include "Scene3D/Scene.h"
#include "Scene3D/SceneFileV2.h"
#include "Scene3D/Systems/FoliageSystem.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/Heightmap.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Material/NMaterial.h"
#include "Render/Material/NMaterialNames.h"
#include "Render/RenderHelper.h"
#include "Render/Texture.h"
#include "Render/Renderer.h"
#include "Render/Shader.h"
#include "Render/ShaderCache.h"
#include "Render/TextureDescriptor.h"
#include "Render/DynamicBufferAllocator.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Concurrency/LockGuard.h"

#include "Engine/Engine.h"
#include "Engine/EngineSettings.h"

#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
#include "Logger/Logger.h"

#if defined(__DAVAENGINE_ANDROID__)
#include "Platform/DeviceInfo.h"
#endif

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(Landscape)
{
    ReflectionRegistrator<Landscape>::Begin()
    .Field("heightmapPath", &Landscape::GetHeightmapPathname, &Landscape::SetHeightmapPathname)[M::DisplayName("Height Map Path")]
    .Field("size", &Landscape::GetLandscapeSize, static_cast<void (Landscape::*)(float32)>(&Landscape::SetLandscapeSize))[M::DisplayName("Size")]
    .Field("height", &Landscape::GetLandscapeHeight, &Landscape::SetLandscapeHeight)[M::DisplayName("Height")]
    .Field("userMorphing", &Landscape::IsUseMorphing, &Landscape::SetUseMorphing)[M::DisplayName("Use morphing")]
    .Field("isDrawWired", &Landscape::IsDrawWired, &Landscape::SetDrawWired)[M::DisplayName("Is draw wired")]
    .Field("debugDrawMorphing", &Landscape::IsDrawMorphing, &Landscape::SetDrawMorphing)[M::DisplayName("Debug draw morphing")]
    .Field("debugDrawMetrics", &Landscape::debugDrawMetrics)[M::DisplayName("Debug draw metrics")]
    .Field("subdivision", &Landscape::subdivision)[M::DisplayName("Subdivision")]
    .End();
}

const FastName Landscape::PARAM_TEXTURE_TILING("textureTiling");
const FastName Landscape::PARAM_TILE_COLOR0("tileColor0");
const FastName Landscape::PARAM_TILE_COLOR1("tileColor1");
const FastName Landscape::PARAM_TILE_COLOR2("tileColor2");
const FastName Landscape::PARAM_TILE_COLOR3("tileColor3");

const FastName Landscape::TEXTURE_COLOR("colorTexture");
const FastName Landscape::TEXTURE_TILE("tileTexture0");
const FastName Landscape::TEXTURE_TILEMASK("tileMask");
const FastName Landscape::TEXTURE_SPECULAR("specularMap");

const FastName Landscape::LANDSCAPE_QUALITY_NAME("Landscape");
const FastName Landscape::LANDSCAPE_QUALITY_VALUE_HIGH("HIGH");

const uint32 LANDSCAPE_BATCHES_POOL_SIZE = 32;
const uint32 LANDSCAPE_MATERIAL_SORTING_KEY = 10;

static const uint32 PATCH_SIZE_VERTICES = 9;
static const uint32 PATCH_SIZE_QUADS = (PATCH_SIZE_VERTICES - 1);

static const uint32 INSTANCE_DATA_BUFFERS_POOL_SIZE = 9;

Landscape::Landscape()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    type = TYPE_LANDSCAPE;

    subdivision = new LandscapeSubdivision();

    renderMode = RENDERMODE_NO_INSTANCING;
    if (rhi::DeviceCaps().isInstancingSupported && rhi::DeviceCaps().isVertexTextureUnitsSupported)
    {
        if (rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R8G8B8A8, rhi::PROG_VERTEX))
        {
            renderMode = RENDERMODE_INSTANCING_MORPHING;
        }
        else if (rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R4G4B4A4, rhi::PROG_VERTEX))
        {
            renderMode = RENDERMODE_INSTANCING;
        }
        else if (rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R32F, rhi::PROG_VERTEX))
        {
            renderMode = RENDERMODE_INSTANCING;
            floatHeightTexture = true;
        }
    }

    EngineSettings* settings = GetEngineContext()->settings;
    EngineSettings::eSettingValue landscapeSetting = settings->GetSetting<EngineSettings::SETTING_LANDSCAPE_RENDERMODE>().Get<EngineSettings::eSettingValue>();
    if (landscapeSetting == EngineSettings::LANDSCAPE_NO_INSTANCING)
        renderMode = RENDERMODE_NO_INSTANCING;
    else if (landscapeSetting == EngineSettings::LANDSCAPE_INSTANCING && renderMode == RENDERMODE_INSTANCING_MORPHING)
        renderMode = RENDERMODE_INSTANCING;

    isRequireTangentBasis = (QualitySettingsSystem::Instance()->GetCurMaterialQuality(LANDSCAPE_QUALITY_NAME) == LANDSCAPE_QUALITY_VALUE_HIGH);

#if defined(__DAVAENGINE_ANDROID__)
    if (renderMode == RENDERMODE_INSTANCING_MORPHING)
    {
        String version = DeviceInfo::GetVersion();
        const char* dotChar = strchr(version.c_str(), '.');
        int32 majorVersion = (dotChar && dotChar != version.c_str()) ? atoi(dotChar - 1) : 0;

        bool maliT600series = strstr(rhi::DeviceCaps().deviceDescription, "Mali-T6") != nullptr;

        //Workaround for some mali drivers (Android 4.x + T6xx gpu): it does not support fetch from texture mips in vertex program
        renderMode = (majorVersion == 4 && maliT600series) ? RENDERMODE_INSTANCING : RENDERMODE_INSTANCING_MORPHING;
    }
    if (renderMode != RENDERMODE_NO_INSTANCING)
    {
        //Workaround for Lenovo P90: on this device vertex texture fetch is very slow
        //(relevant for Android 4.4.4, currently there is no update to Android 5.0)
        if (strstr(DeviceInfo::GetModel().c_str(), "Lenovo P90") != nullptr)
        {
            renderMode = RENDERMODE_NO_INSTANCING;
        }
    }
#endif

    AddFlag(RenderObject::CUSTOM_PREPARE_TO_RENDER);

    Renderer::GetSignals().needRestoreResources.Connect(this, &Landscape::RestoreGeometry);
}

Landscape::~Landscape()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    ReleaseGeometryData();

    SafeRelease(heightmap);
    SafeDelete(subdivision);

    SafeRelease(landscapeMaterial);
    Renderer::GetSignals().needRestoreResources.Disconnect(this);
}

void Landscape::RestoreGeometry()
{
    LockGuard<Mutex> lock(restoreDataMutex);
    for (auto& restoreData : bufferRestoreData)
    {
        switch (restoreData.bufferType)
        {
        case RestoreBufferData::RESTORE_BUFFER_VERTEX:
            if (rhi::NeedRestoreVertexBuffer(static_cast<rhi::HVertexBuffer>(restoreData.buffer)))
                rhi::UpdateVertexBuffer(static_cast<rhi::HVertexBuffer>(restoreData.buffer), restoreData.data, 0, restoreData.dataSize);
            break;

        case RestoreBufferData::RESTORE_BUFFER_INDEX:
            if (rhi::NeedRestoreIndexBuffer(static_cast<rhi::HIndexBuffer>(restoreData.buffer)))
                rhi::UpdateIndexBuffer(static_cast<rhi::HIndexBuffer>(restoreData.buffer), restoreData.data, 0, restoreData.dataSize);
            break;

        case RestoreBufferData::RESTORE_TEXTURE:
            // if (rhi::NeedRestoreTexture(static_cast<rhi::HTexture>(restoreData.buffer)))
            // we are not checking condition above,
            // because texture is marked as restored immediately after updating zero level
            rhi::UpdateTexture(static_cast<rhi::HTexture>(restoreData.buffer), restoreData.data, restoreData.level);
            break;

        default:
            DVASSERT(0, "Invalid RestoreBufferData type");
        }
    }
}

void Landscape::ReleaseGeometryData()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    ////General
    for (RenderBatchWithOptions& batch : renderBatchArray)
        batch.renderBatch->Release();

    renderBatchArray.clear();
    activeRenderBatchArray.clear();

    {
        LockGuard<Mutex> lock(restoreDataMutex);
        for (auto& restoreData : bufferRestoreData)
            SafeDeleteArray(restoreData.data);
        bufferRestoreData.clear();
    }

    ////Non-instanced data
    for (rhi::HVertexBuffer handle : vertexBuffers)
        rhi::DeleteVertexBuffer(handle);
    vertexBuffers.clear();

    indices.clear();

    subdivision->ReleaseInternalData();

    quadsInWidthPow2 = 0;

    ////Instanced data

    SafeRelease(heightTexture);

    if (patchVertexBuffer)
    {
        rhi::DeleteVertexBuffer(patchVertexBuffer);
        patchVertexBuffer = rhi::HVertexBuffer();
    }

    if (patchIndexBuffer)
    {
        rhi::DeleteIndexBuffer(patchIndexBuffer);
        patchIndexBuffer = rhi::HIndexBuffer();
    }

    for (InstanceDataBuffer* buffer : freeInstanceDataBuffers)
    {
        rhi::DeleteVertexBuffer(buffer->buffer);
        SafeDelete(buffer);
    }
    freeInstanceDataBuffers.clear();

    for (InstanceDataBuffer* buffer : usedInstanceDataBuffers)
    {
        rhi::DeleteVertexBuffer(buffer->buffer);
        SafeDelete(buffer);
    }
    usedInstanceDataBuffers.clear();

    if (landscapeMaterial)
    {
        if (landscapeMaterial->HasLocalTexture(NMaterialTextureName::TEXTURE_HEIGHTMAP))
        {
            landscapeMaterial->RemoveTexture(NMaterialTextureName::TEXTURE_HEIGHTMAP);
            heightTexture = nullptr;
        }
        if (landscapeMaterial->HasLocalTexture(NMaterialTextureName::TEXTURE_TANGENTSPACE))
        {
            landscapeMaterial->RemoveTexture(NMaterialTextureName::TEXTURE_TANGENTSPACE);
            tangentTexture = nullptr;
        }
    }
}

void Landscape::BuildLandscapeFromHeightmapImage(const FilePath& heightmapPathname, const AABBox3& _box)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    heightmapPath = heightmapPathname;
    BuildHeightmap();

    bbox = _box;

    RebuildLandscape();

    if (foliageSystem)
    {
        foliageSystem->SyncFoliageWithLandscape();
    }
}

void Landscape::RecalcBoundingBox()
{
    //do nothing, bbox setup in BuildLandscapeFromHeightmapImage()
}

bool Landscape::BuildHeightmap()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    bool retValue = false;
    SafeRelease(heightmap);

    if (DAVA::TextureDescriptor::IsSourceTextureExtension(heightmapPath.GetExtension()))
    {
        Vector<Image*> imageSet;
        ImageSystem::Load(heightmapPath, imageSet);
        if (0 != imageSet.size())
        {
            if ((imageSet[0]->GetPixelFormat() != FORMAT_A8) && (imageSet[0]->GetPixelFormat() != FORMAT_A16))
            {
                Logger::Error("Image for landscape should be gray scale");
            }
            else
            {
                DVASSERT(imageSet[0]->GetWidth() == imageSet[0]->GetHeight());
                heightmap = new Heightmap();
                heightmap->BuildFromImage(imageSet[0]);
                retValue = true;
            }

            for_each(imageSet.begin(), imageSet.end(), SafeRelease<Image>);
        }
    }
    else if (heightmapPath.IsEqualToExtension(Heightmap::FileExtension()))
    {
        heightmap = new Heightmap();
        retValue = heightmap->Load(heightmapPath);
    }

    return retValue;
}

int32 Landscape::GetHeightmapSize() const
{
    if (heightmap != nullptr)
    {
        return heightmap->Size();
    }
    return 0;
}

void Landscape::AllocateGeometryData()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    uint32 heightmapSize = GetHeightmapSize();
    if (heightmapSize == 0)
    {
        return;
    }

    uint32 minSubdivLevelSize = (renderMode == RENDERMODE_NO_INSTANCING) ? heightmapSize / RENDER_PARCEL_SIZE_QUADS : 0;
    uint32 minSubdivLevel = uint32(HighestBitIndex(minSubdivLevelSize));

    heightmapSizePow2 = uint32(HighestBitIndex(heightmapSize));
    heightmapSizef = float32(heightmapSize);

    subdivision->BuildSubdivision(heightmap, bbox, PATCH_SIZE_QUADS, minSubdivLevel, (renderMode == RENDERMODE_INSTANCING_MORPHING));

    (renderMode == RENDERMODE_NO_INSTANCING) ? AllocateGeometryDataNoInstancing() : AllocateGeometryDataInstancing();
}

void Landscape::RebuildLandscape()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (landscapeMaterial == nullptr)
    {
        landscapeMaterial = new NMaterial();
        landscapeMaterial->SetMaterialName(FastName("Landscape_TileMask_Material"));
        landscapeMaterial->SetFXName(NMaterialName::TILE_MASK);

        PrepareMaterial(landscapeMaterial);
        landscapeMaterial->PreBuildMaterial(PASS_FORWARD);
    }

    ReleaseGeometryData();
    AllocateGeometryData();
}

void Landscape::PrepareMaterial(NMaterial* material)
{
    material->AddFlag(NMaterialFlagName::FLAG_LANDSCAPE_USE_INSTANCING, (renderMode == RENDERMODE_NO_INSTANCING) ? 0 : 1);
    material->AddFlag(NMaterialFlagName::FLAG_LANDSCAPE_LOD_MORPHING, (renderMode == RENDERMODE_INSTANCING_MORPHING) ? 1 : 0);
    material->AddFlag(NMaterialFlagName::FLAG_LANDSCAPE_MORPHING_COLOR, debugDrawMorphing ? 1 : 0);
    material->AddFlag(NMaterialFlagName::FLAG_HEIGHTMAP_FLOAT_TEXTURE, floatHeightTexture ? 1 : 0);
}

Texture* Landscape::CreateHeightTexture(Heightmap* heightmap, RenderMode renderMode)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    DVASSERT(renderMode != RENDERMODE_NO_INSTANCING);

    Vector<Image*> textureData = CreateHeightTextureData(heightmap, renderMode);

    Texture* tx = Texture::CreateFromData(textureData);
    tx->texDescriptor->pathname = "memoryfile_landscape_height";
    tx->SetWrapMode(rhi::TEXADDR_CLAMP, rhi::TEXADDR_CLAMP);
    tx->SetMinMagFilter(rhi::TEXFILTER_NEAREST, rhi::TEXFILTER_NEAREST, (renderMode == RENDERMODE_INSTANCING_MORPHING) ? rhi::TEXMIPFILTER_NEAREST : rhi::TEXMIPFILTER_NONE);

    uint32 level = 0;
    LockGuard<Mutex> lock(restoreDataMutex);
    for (Image* img : textureData)
    {
        bufferRestoreData.emplace_back();

        auto& restore = bufferRestoreData.back();
        restore.bufferType = RestoreBufferData::RESTORE_TEXTURE;
        restore.buffer = tx->handle;
        restore.dataSize = img->dataSize;
        restore.data = new uint8[img->dataSize];
        restore.level = level;
        memcpy(restore.data, img->data, img->dataSize);

        img->Release();
        ++level;
    }

    return tx;
}

Vector<Image*> Landscape::CreateHeightTextureData(Heightmap* heightmap, RenderMode renderMode)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    const uint32 hmSize = GetHeightmapSize();
    DVASSERT(IsPowerOf2(hmSize));
    DVASSERT(renderMode != RENDERMODE_NO_INSTANCING);

    Vector<Image*> dataOut;
    if (renderMode == RENDERMODE_INSTANCING_MORPHING)
    {
        DVASSERT(rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R8G8B8A8, rhi::PROG_VERTEX));

        dataOut.reserve(HighestBitIndex(hmSize));

        uint32 mipSize = hmSize;
        uint32 step = 1;
        uint32 mipLevel = 0;
        uint32* mipData = new uint32[mipSize * mipSize]; //RGBA8888

        while (mipSize)
        {
            uint16* mipDataPtr = reinterpret_cast<uint16*>(mipData);
            for (uint32 y = 0; y < mipSize; ++y)
            {
                uint32 mipLastIndex = mipSize - 1;
                uint16 yy = y * step;
                uint16 y1 = yy;
                uint16 y2 = yy;
                if ((y & 0x1) && y != mipLastIndex)
                {
                    y1 -= step;
                    y2 += step;
                }

                for (uint32 x = 0; x < mipSize; ++x)
                {
                    uint16 xx = x * step;
                    uint16 x1 = xx;
                    uint16 x2 = xx;
                    if ((x & 0x1) && x != mipLastIndex)
                    {
                        x1 += step;
                        x2 -= step;
                    }

                    *mipDataPtr++ = heightmap->GetHeight(xx, yy);

                    uint16 h1 = heightmap->GetHeightClamp(x1, y1);
                    uint16 h2 = heightmap->GetHeightClamp(x2, y2);
                    *mipDataPtr++ = (h1 + h2) / 2;
                }
            }

            Image* mipImg = Image::CreateFromData(mipSize, mipSize, FORMAT_RGBA8888, reinterpret_cast<uint8*>(mipData));
            mipImg->mipmapLevel = mipLevel;
            dataOut.push_back(mipImg);

            mipSize >>= 1;
            step <<= 1;
            mipLevel++;
        }

        SafeDeleteArray(mipData);
    }
    else
    {
        Image* heightImage = nullptr;
        if (floatHeightTexture)
        {
            DVASSERT(rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R32F, rhi::PROG_VERTEX));

            float32* texData = new float32[hmSize * hmSize];
            float32* texDataPtr = texData;

            for (uint32 y = 0; y < hmSize; ++y)
            {
                for (uint32 x = 0; x < hmSize; ++x)
                {
                    *texDataPtr++ = float32(heightmap->GetHeight(x, y)) / Heightmap::MAX_VALUE;
                }
            }

            heightImage = Image::CreateFromData(hmSize, hmSize, FORMAT_R32F, reinterpret_cast<uint8*>(texData));
            SafeDeleteArray(texData);
        }
        else
        {
            DVASSERT(rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R4G4B4A4, rhi::PROG_VERTEX));
            heightImage = Image::CreateFromData(hmSize, hmSize, FORMAT_RGBA4444, reinterpret_cast<uint8*>(heightmap->Data()));
        }

        dataOut.push_back(heightImage);
    }

    return dataOut;
}

Texture* Landscape::CreateTangentTexture()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Vector<Image*> textureData = CreateTangentBasisTextureData();

    Texture* tx = Texture::CreateFromData(textureData);
    tx->texDescriptor->pathname = "memoryfile_landscape_tangents";
    tx->SetWrapMode(rhi::TEXADDR_CLAMP, rhi::TEXADDR_CLAMP);
    tx->SetMinMagFilter(rhi::TEXFILTER_NEAREST, rhi::TEXFILTER_NEAREST, rhi::TEXMIPFILTER_NONE);

    uint32 level = 0;
    LockGuard<Mutex> lock(restoreDataMutex);
    for (Image* img : textureData)
    {
        auto& restore = bufferRestoreData.back();
        restore.bufferType = RestoreBufferData::RESTORE_TEXTURE;
        restore.buffer = tx->handle;
        restore.dataSize = img->dataSize;
        restore.data = new uint8[img->dataSize];
        restore.level = level;
        memcpy(restore.data, img->data, img->dataSize);

        img->Release();
        ++level;
    }

    return tx;
}

Vector<Image*> Landscape::CreateTangentBasisTextureData()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    const uint32 hmSize = GetHeightmapSize();
    DVASSERT(IsPowerOf2(hmSize));

    Vector<Image*> dataOut;
    {
        uint32* normalTangntData = new uint32[hmSize * hmSize]; //RGBA8888
        uint8* normalTangntDataPtr = reinterpret_cast<uint8*>(normalTangntData);

        Vector3 normal, tangent;
        for (uint32 y = 0; y < hmSize; ++y)
        {
            for (uint32 x = 0; x < hmSize; ++x)
            {
                GetTangentBasis(x, y, normal, tangent);

                normal = normal * 0.5f + 0.5f;
                tangent = tangent * 0.5 + 0.5f;

                *normalTangntDataPtr++ = uint8(normal.x * 255.f);
                *normalTangntDataPtr++ = uint8(normal.y * 255.f);
                *normalTangntDataPtr++ = uint8(tangent.y * 255.f);
                *normalTangntDataPtr++ = uint8(tangent.z * 255.f);
            }
        }

        Image* basisImage = Image::CreateFromData(hmSize, hmSize, FORMAT_RGBA8888, reinterpret_cast<uint8*>(normalTangntData));
        dataOut.push_back(basisImage);
    }

    return dataOut;
}

void Landscape::GetTangentBasis(uint32 x, uint32 y, Vector3& normalOut, Vector3& tangentOut) const
{
    DVASSERT(heightmap);
    const uint32 hmSize = heightmap->Size();

    Vector3 position = heightmap->GetPoint(x, y, bbox);

    uint32 xx = Min(x + 1, hmSize - 1);
    uint32 yy = Min(y + 1, hmSize - 1);
    Vector3 right = heightmap->GetPoint(xx, y, bbox);
    Vector3 bottom = heightmap->GetPoint(x, yy, bbox);

    xx = (x == 0) ? 0 : x - 1;
    yy = (y == 0) ? 0 : y - 1;
    Vector3 left = heightmap->GetPoint(xx, y, bbox);
    Vector3 top = heightmap->GetPoint(x, yy, bbox);

    Vector3 normal0 = (top != position && right != position) ? CrossProduct(top - position, right - position) : Vector3(0, 0, 0);
    Vector3 normal1 = (right != position && bottom != position) ? CrossProduct(right - position, bottom - position) : Vector3(0, 0, 0);
    Vector3 normal2 = (bottom != position && left != position) ? CrossProduct(bottom - position, left - position) : Vector3(0, 0, 0);
    Vector3 normal3 = (left != position && top != position) ? CrossProduct(left - position, top - position) : Vector3(0, 0, 0);

    Vector3 normalAverage = normal0 + normal1 + normal2 + normal3;

    normalOut = Normalize(normalAverage);
    tangentOut = Normalize(right - position);

    /*
     VS: Algorithm
     // # P.xy store the position for which we want to calculate the normals
     // # height() here is a function that return the height at a point in the terrain
     
     // read neighbor heights using an arbitrary small offset
     vec3 off = vec3(1.0, 1.0, 0.0);
     float hL = height(P.xy - off.xz);
     float hR = height(P.xy + off.xz);
     float hD = height(P.xy - off.zy);
     float hU = height(P.xy + off.zy);
     
     // deduce terrain normal
     N.x = hL - hR;
     N.y = hD - hU;
     N.z = 2.0;
     N = normalize(N);
     */
}

bool Landscape::GetHeightAtPoint(const Vector3& point, float32& value) const
{
    if ((point.x > bbox.max.x) || (point.x < bbox.min.x) || (point.y > bbox.max.y) || (point.y < bbox.min.y))
    {
        return false;
    }

    int32 hmSize = GetHeightmapSize();
    if (hmSize == 0)
    {
        Logger::Error("[Landscape::GetHeightAtPoint] Trying to get height at point using empty heightmap data!");
        return false;
    }

    float32 fx = static_cast<float32>(hmSize) * (point.x - bbox.min.x) / (bbox.max.x - bbox.min.x);
    float32 fy = static_cast<float32>(hmSize) * (point.y - bbox.min.y) / (bbox.max.y - bbox.min.y);
    uint16 x = static_cast<uint16>(fx);
    uint16 y = static_cast<uint16>(fy);

    Vector3 h00 = heightmap->GetPoint(x, y, bbox);
    Vector3 h01 = heightmap->GetPoint(x + 1, y, bbox);
    Vector3 h10 = heightmap->GetPoint(x, y + 1, bbox);
    Vector3 h11 = heightmap->GetPoint(x + 1, y + 1, bbox);

    float32 dx = fx - static_cast<float32>(x);
    float32 dy = fy - static_cast<float32>(y);
    float32 h0 = h00.z * (1.0f - dx) + h01.z * dx;
    float32 h1 = h10.z * (1.0f - dx) + h11.z * dx;
    value = (h0 * (1.0f - dy) + h1 * dy);

    return true;
}

bool Landscape::PlacePoint(const Vector3& worldPoint, Vector3& result, Vector3* normal) const
{
    result = worldPoint;

    if (GetHeightAtPoint(worldPoint, result.z) == false)
    {
        return false;
    }

    if (normal != nullptr)
    {
        const float32 normalDelta = 0.01f;
        Vector3 dx = result + Vector3(normalDelta, 0.0f, 0.0f);
        Vector3 dy = result + Vector3(0.0f, normalDelta, 0.0f);
        GetHeightAtPoint(dx, dx.z);
        GetHeightAtPoint(dy, dy.z);
        *normal = (dx - result).CrossProduct(dy - result);
        normal->Normalize();
    }

    return true;
};

void Landscape::AddPatchToRender(uint32 level, uint32 x, uint32 y)
{
    DVASSERT(level < subdivision->GetLevelCount());

    const LandscapeSubdivision::SubdivisionPatchInfo& subdivPatchInfo = subdivision->GetPatchInfo(level, x, y);

    uint32 state = subdivPatchInfo.subdivisionState;
    if (state == LandscapeSubdivision::SubdivisionPatchInfo::CLIPPED)
        return;

    if (state == LandscapeSubdivision::SubdivisionPatchInfo::SUBDIVIDED)
    {
        uint32 x2 = x << 1;
        uint32 y2 = y << 1;

        AddPatchToRender(level + 1, x2 + 0, y2 + 0);
        AddPatchToRender(level + 1, x2 + 1, y2 + 0);
        AddPatchToRender(level + 1, x2 + 0, y2 + 1);
        AddPatchToRender(level + 1, x2 + 1, y2 + 1);
    }
    else
    {
        uint32 xNegLevel = level, yNegLevel = level, xPosLevel = level, yPosLevel = level;
        const LandscapeSubdivision::SubdivisionPatchInfo* xNeg = subdivision->GetTerminatedPatchInfo(level, x - 1, y, xNegLevel);
        const LandscapeSubdivision::SubdivisionPatchInfo* yNeg = subdivision->GetTerminatedPatchInfo(level, x, y - 1, yNegLevel);
        const LandscapeSubdivision::SubdivisionPatchInfo* xPos = subdivision->GetTerminatedPatchInfo(level, x + 1, y, xPosLevel);
        const LandscapeSubdivision::SubdivisionPatchInfo* yPos = subdivision->GetTerminatedPatchInfo(level, x, y + 1, yPosLevel);

        switch (renderMode)
        {
        case RENDERMODE_INSTANCING_MORPHING:
        {
            const float32 morph = subdivPatchInfo.subdivMorph;
            float32 xNegMorph = xNeg ? ((xNegLevel < level) ? xNeg->subdivMorph : Min(xNeg->subdivMorph, morph)) : morph;
            float32 yNegMorph = yNeg ? ((yNegLevel < level) ? yNeg->subdivMorph : Min(yNeg->subdivMorph, morph)) : morph;
            float32 xPosMorph = xPos ? ((xPosLevel < level) ? xPos->subdivMorph : Min(xPos->subdivMorph, morph)) : morph;
            float32 yPosMorph = yPos ? ((yPosLevel < level) ? yPos->subdivMorph : Min(yPos->subdivMorph, morph)) : morph;

            Vector4 neighbourLevelf = Vector4(float32(xNegLevel), float32(yNegLevel), float32(xPosLevel), float32(yPosLevel));
            Vector4 neighbourMorph = Vector4(xNegMorph, yNegMorph, xPosMorph, yPosMorph);
            DrawPatchInstancing(level, x, y, neighbourLevelf, morph, neighbourMorph);
        }
        break;
        case RENDERMODE_INSTANCING:
        {
            Vector4 neighbourLevelf = Vector4(float32(xNegLevel), float32(yNegLevel), float32(xPosLevel), float32(yPosLevel));
            DrawPatchInstancing(level, x, y, neighbourLevelf);
        }
        break;
        case RENDERMODE_NO_INSTANCING:
        {
            DrawPatchNoInstancing(level, x, y, xNegLevel, yNegLevel, xPosLevel, yPosLevel);
        }
        break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////Non-instancing render

void Landscape::AllocateGeometryDataNoInstancing()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    rhi::VertexLayout vLayout;
    vLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    vLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    if (isRequireTangentBasis)
    {
        vLayout.AddElement(rhi::VS_NORMAL, 0, rhi::VDT_FLOAT, 3);
        vLayout.AddElement(rhi::VS_TANGENT, 0, rhi::VDT_FLOAT, 3);
    }
    vLayoutUIDNoInstancing = rhi::VertexLayout::UniqueId(vLayout);

    for (uint32 i = 0; i < LANDSCAPE_BATCHES_POOL_SIZE; i++)
    {
        AllocateRenderBatch();
    }

    indices.resize(INITIAL_INDEX_BUFFER_CAPACITY);

    uint32 quadsInWidth = heightmap->Size() / RENDER_PARCEL_SIZE_QUADS;
    // For cases where landscape is very small allocate 1 VBO.
    if (quadsInWidth == 0)
        quadsInWidth = 1;

    quadsInWidthPow2 = uint32(HighestBitIndex(quadsInWidth));

    for (uint32 y = 0; y < quadsInWidth; ++y)
    {
        for (uint32 x = 0; x < quadsInWidth; ++x)
        {
            uint16 check = AllocateParcelVertexBuffer(x * RENDER_PARCEL_SIZE_QUADS, y * RENDER_PARCEL_SIZE_QUADS, RENDER_PARCEL_SIZE_QUADS);
            DVASSERT(check == uint16(x + y * quadsInWidth));
        }
    }
}

void Landscape::AllocateRenderBatch()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    ScopedPtr<RenderBatch> batch(new RenderBatch());
    AddRenderBatch(batch);

    batch->SetMaterial(landscapeMaterial);
    batch->SetSortingKey(LANDSCAPE_MATERIAL_SORTING_KEY);

    batch->vertexLayoutId = vLayoutUIDNoInstancing;
    batch->vertexCount = RENDER_PARCEL_SIZE_VERTICES * RENDER_PARCEL_SIZE_VERTICES;
}

int16 Landscape::AllocateParcelVertexBuffer(uint32 quadX, uint32 quadY, uint32 quadSize)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    uint32 verticesCount = (quadSize + 1) * (quadSize + 1);
    uint32 vertexSize = sizeof(VertexNoInstancing);
    if (!isRequireTangentBasis)
    {
        vertexSize -= sizeof(Vector3); // (Vertex::normal);
        vertexSize -= sizeof(Vector3); // (Vertex::tangent);
    }

    uint8* landscapeVertices = new uint8[verticesCount * vertexSize];
    uint32 index = 0;
    for (uint32 y = quadY; y < quadY + quadSize + 1; ++y)
    {
        for (uint32 x = quadX; x < quadX + quadSize + 1; ++x)
        {
            VertexNoInstancing* vertex = reinterpret_cast<VertexNoInstancing*>(&landscapeVertices[index * vertexSize]);
            vertex->position = heightmap->GetPoint(x, y, bbox);

            Vector2 texCoord = Vector2(x / heightmapSizef, 1.0f - y / heightmapSizef);
            vertex->texCoord = texCoord;

            if (isRequireTangentBasis)
            {
                GetTangentBasis(x, y, vertex->normal, vertex->tangent);
            }

            index++;
        }
    }

    uint32 vBufferSize = static_cast<uint32>(verticesCount * vertexSize);

    rhi::VertexBuffer::Descriptor desc;
    desc.size = vBufferSize;
    desc.initialData = landscapeVertices;
    if (updatable)
        desc.usage = rhi::USAGE_DYNAMICDRAW;
    else
        desc.usage = rhi::USAGE_STATICDRAW;

    rhi::HVertexBuffer vertexBuffer = rhi::CreateVertexBuffer(desc);
    vertexBuffers.push_back(vertexBuffer);

#if defined(__DAVAENGINE_IPHONE__)
    SafeDeleteArray(landscapeVertices);
#else
    LockGuard<Mutex> lock(restoreDataMutex);
    bufferRestoreData.push_back({ vertexBuffer, landscapeVertices, vBufferSize, 0, RestoreBufferData::RESTORE_BUFFER_VERTEX });
#endif

    return int16(vertexBuffers.size() - 1);
}

void Landscape::DrawLandscapeNoInstancing()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    drawIndices = 0;
    flushQueueCounter = 0;
    activeRenderBatchArray.clear();
    queuedQuadBuffer = -1;

    DVASSERT(queueIndexCount == 0);

    AddPatchToRender(0, 0, 0);
    FlushQueue();
}

void Landscape::DrawPatchNoInstancing(uint32 level, uint32 xx, uint32 yy, uint32 xNegSizePow2, uint32 yNegSizePow2, uint32 xPosSizePow2, uint32 yPosSizePow2)
{
    const LandscapeSubdivision::SubdivisionLevelInfo& levelInfo = subdivision->GetLevelInfo(level);

    int32 dividerPow2 = level - quadsInWidthPow2;
    DVASSERT(dividerPow2 >= 0);
    uint16 quadBuffer = ((yy >> dividerPow2) << quadsInWidthPow2) + (xx >> dividerPow2);

    if ((quadBuffer != queuedQuadBuffer) && (queuedQuadBuffer != -1))
    {
        FlushQueue();
    }

    queuedQuadBuffer = quadBuffer;

    // Draw Middle
    uint32 realVertexCountInPatch = heightmap->Size() >> level;
    uint32 step = realVertexCountInPatch / PATCH_SIZE_QUADS;
    uint32 heightMapStartX = xx * realVertexCountInPatch;
    uint32 heightMapStartY = yy * realVertexCountInPatch;

    ResizeIndicesBufferIfNeeded(queueIndexCount + PATCH_SIZE_QUADS * PATCH_SIZE_QUADS * 6);

    uint16* indicesPtr = indices.data() + queueIndexCount;
    // Draw middle block
    {
        for (uint16 y = (heightMapStartY & RENDER_PARCEL_AND); y < (heightMapStartY & RENDER_PARCEL_AND) + realVertexCountInPatch; y += step)
        {
            for (uint16 x = (heightMapStartX & RENDER_PARCEL_AND); x < (heightMapStartX & RENDER_PARCEL_AND) + realVertexCountInPatch; x += step)
            {
                uint16 x0 = x;
                uint16 y0 = y;
                uint16 x1 = x + step;
                uint16 y1 = y + step;

                uint16 x0aligned = x0;
                uint16 y0aligned = y0;
                uint16 x1aligned = x1;
                uint16 y1aligned = y1;

                uint16 x0aligned2 = x0;
                uint16 y0aligned2 = y0;
                uint16 x1aligned2 = x1;
                uint16 y1aligned2 = y1;

                if (x == (heightMapStartX & RENDER_PARCEL_AND))
                {
                    uint16 alignMod = levelInfo.size >> xNegSizePow2;
                    if (alignMod > 1)
                    {
                        y0aligned = y0 / (alignMod * step) * (alignMod * step);
                        y1aligned = y1 / (alignMod * step) * (alignMod * step);
                    }
                }

                if (y == (heightMapStartY & RENDER_PARCEL_AND))
                {
                    uint16 alignMod = levelInfo.size >> yNegSizePow2;
                    if (alignMod > 1)
                    {
                        x0aligned = x0 / (alignMod * step) * (alignMod * step);
                        x1aligned = x1 / (alignMod * step) * (alignMod * step);
                    }
                }

                if (x == ((heightMapStartX & RENDER_PARCEL_AND) + realVertexCountInPatch - step))
                {
                    uint16 alignMod = levelInfo.size >> xPosSizePow2;
                    if (alignMod > 1)
                    {
                        y0aligned2 = y0 / (alignMod * step) * (alignMod * step);
                        y1aligned2 = y1 / (alignMod * step) * (alignMod * step);
                    }
                }

                if (y == ((heightMapStartY & RENDER_PARCEL_AND) + realVertexCountInPatch - step))
                {
                    uint16 alignMod = levelInfo.size >> yPosSizePow2;
                    if (alignMod > 1)
                    {
                        x0aligned2 = x0 / (alignMod * step) * (alignMod * step);
                        x1aligned2 = x1 / (alignMod * step) * (alignMod * step);
                    }
                }

                *indicesPtr++ = GetVertexIndex(x0aligned, y0aligned);
                *indicesPtr++ = GetVertexIndex(x1aligned, y0aligned2);
                *indicesPtr++ = GetVertexIndex(x0aligned2, y1aligned);

                *indicesPtr++ = GetVertexIndex(x1aligned, y0aligned2);
                *indicesPtr++ = GetVertexIndex(x1aligned2, y1aligned2);
                *indicesPtr++ = GetVertexIndex(x0aligned2, y1aligned);

                queueIndexCount += 6;
            }
        }
    }
}

void Landscape::FlushQueue()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (queueIndexCount == 0)
        return;

    DVASSERT(queuedQuadBuffer != -1);

    uint16* indicesPtr = indices.data();
    while (queueIndexCount != 0)
    {
        DVASSERT(flushQueueCounter <= static_cast<int32>(renderBatchArray.size()));
        if (static_cast<int32>(renderBatchArray.size()) == flushQueueCounter)
        {
            AllocateRenderBatch();
        }

        DynamicBufferAllocator::AllocResultIB indexBuffer = DynamicBufferAllocator::AllocateIndexBuffer(queueIndexCount);
        DVASSERT(queueIndexCount >= indexBuffer.allocatedindices);
        uint32 allocatedIndices = indexBuffer.allocatedindices - indexBuffer.allocatedindices % 3; //in buffer must be completed triangles

        Memcpy(indexBuffer.data, indicesPtr, allocatedIndices * sizeof(uint16));
        RenderBatch* batch = renderBatchArray[flushQueueCounter].renderBatch;
        batch->indexBuffer = indexBuffer.buffer;
        batch->indexCount = allocatedIndices;
        batch->startIndex = indexBuffer.baseIndex;
        batch->vertexBuffer = vertexBuffers[queuedQuadBuffer];

        DAVA_PROFILER_GPU_RENDER_BATCH(batch, ProfilerGPUMarkerName::LANDSCAPE);

        activeRenderBatchArray.emplace_back(batch);

        queueIndexCount -= allocatedIndices;
        indicesPtr += allocatedIndices;

        drawIndices += allocatedIndices;
        ++flushQueueCounter;
    }

    DVASSERT(queueIndexCount == 0);
    queuedQuadBuffer = -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////Instancing render

void Landscape::AllocateGeometryDataInstancing()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    heightTexture = CreateHeightTexture(heightmap, renderMode);
    landscapeMaterial->AddTexture(NMaterialTextureName::TEXTURE_HEIGHTMAP, heightTexture);

    if (isRequireTangentBasis)
    {
        tangentTexture = CreateTangentTexture();
        landscapeMaterial->AddTexture(NMaterialTextureName::TEXTURE_TANGENTSPACE, tangentTexture);
    }

    /////////////////////////////////////////////////////////////////

    const uint32 VERTICES_COUNT = PATCH_SIZE_VERTICES * PATCH_SIZE_VERTICES;
    const uint32 INDICES_COUNT = PATCH_SIZE_QUADS * PATCH_SIZE_QUADS * 6;

    VertexInstancing* patchVertices = new VertexInstancing[VERTICES_COUNT];
    uint16* patchIndices = new uint16[INDICES_COUNT];
    uint16* indicesPtr = patchIndices;

    float32 quadSize = 1.f / PATCH_SIZE_QUADS;

    for (uint32 y = 0; y < PATCH_SIZE_VERTICES; ++y)
    {
        for (uint32 x = 0; x < PATCH_SIZE_VERTICES; ++x)
        {
            VertexInstancing& vertex = patchVertices[y * PATCH_SIZE_VERTICES + x];
            vertex.position = Vector2(x * quadSize, y * quadSize);
            vertex.edgeMask = Vector4(0.f, 0.f, 0.f, 0.f);
            vertex.edgeShiftDirection = Vector2(0.f, 0.f);
            vertex.edgeVertexIndex = 0.f;
            vertex.edgeMaskNull = 1.f;

            if (x < (PATCH_SIZE_VERTICES - 1) && y < (PATCH_SIZE_VERTICES - 1))
            {
                *indicesPtr++ = (y + 0) * PATCH_SIZE_VERTICES + (x + 0);
                *indicesPtr++ = (y + 0) * PATCH_SIZE_VERTICES + (x + 1);
                *indicesPtr++ = (y + 1) * PATCH_SIZE_VERTICES + (x + 0);

                *indicesPtr++ = (y + 1) * PATCH_SIZE_VERTICES + (x + 0);
                *indicesPtr++ = (y + 0) * PATCH_SIZE_VERTICES + (x + 1);
                *indicesPtr++ = (y + 1) * PATCH_SIZE_VERTICES + (x + 1);
            }
        }
    }

    for (uint32 i = 1; i < PATCH_SIZE_QUADS; ++i)
    {
        //x = 0; y = i; left side of patch without corners
        patchVertices[i * PATCH_SIZE_VERTICES].edgeMask = Vector4(1.f, 0.f, 0.f, 0.f);
        patchVertices[i * PATCH_SIZE_VERTICES].edgeShiftDirection = Vector2(0.f, -1.f) / float32(PATCH_SIZE_QUADS);
        patchVertices[i * PATCH_SIZE_VERTICES].edgeVertexIndex = float32(i);
        patchVertices[i * PATCH_SIZE_VERTICES].edgeMaskNull = 0.f;

        //x = i; y = 0; bottom side of patch without corners
        patchVertices[i].edgeMask = Vector4(0.f, 1.f, 0.f, 0.f);
        patchVertices[i].edgeShiftDirection = Vector2(-1.f, 0.f) / float32(PATCH_SIZE_QUADS);
        patchVertices[i].edgeVertexIndex = float32(i);
        patchVertices[i].edgeMaskNull = 0.f;

        //x = PATCH_QUAD_COUNT; y = i; right side of patch without corners
        patchVertices[i * PATCH_SIZE_VERTICES + PATCH_SIZE_QUADS].edgeMask = Vector4(0.f, 0.f, 1.f, 0.f);
        patchVertices[i * PATCH_SIZE_VERTICES + PATCH_SIZE_QUADS].edgeShiftDirection = Vector2(0.f, 1.f) / float32(PATCH_SIZE_QUADS);
        patchVertices[i * PATCH_SIZE_VERTICES + PATCH_SIZE_QUADS].edgeVertexIndex = float32(PATCH_SIZE_QUADS - i);
        patchVertices[i * PATCH_SIZE_VERTICES + PATCH_SIZE_QUADS].edgeMaskNull = 0.f;

        //x = i; y = PATCH_QUAD_COUNT; top side of patch without corners
        patchVertices[PATCH_SIZE_QUADS * PATCH_SIZE_VERTICES + i].edgeMask = Vector4(0.f, 0.f, 0.f, 1.f);
        patchVertices[PATCH_SIZE_QUADS * PATCH_SIZE_VERTICES + i].edgeShiftDirection = Vector2(1.f, 0.f) / float32(PATCH_SIZE_QUADS);
        patchVertices[PATCH_SIZE_QUADS * PATCH_SIZE_VERTICES + i].edgeVertexIndex = float32(PATCH_SIZE_QUADS - i);
        patchVertices[PATCH_SIZE_QUADS * PATCH_SIZE_VERTICES + i].edgeMaskNull = 0.f;
    }

    /////////////////////////////////////////////////////////////////

    instanceDataSize = (renderMode == RENDERMODE_INSTANCING) ? INSTANCE_DATA_SIZE : INSTANCE_DATA_SIZE_MORPHING;

    for (uint32 i = 0; i < INSTANCE_DATA_BUFFERS_POOL_SIZE; ++i)
    {
        rhi::VertexBuffer::Descriptor instanceBufferDesc;
        instanceBufferDesc.size = instanceDataMaxCount * instanceDataSize;
        instanceBufferDesc.usage = rhi::USAGE_DYNAMICDRAW;
        instanceBufferDesc.needRestore = false;

        InstanceDataBuffer* instanceDataBuffer = new InstanceDataBuffer();
        instanceDataBuffer->bufferSize = instanceBufferDesc.size;
        instanceDataBuffer->buffer = rhi::CreateVertexBuffer(instanceBufferDesc);

        freeInstanceDataBuffers.push_back(instanceDataBuffer);
    }

    rhi::VertexBuffer::Descriptor vdesc;
    vdesc.size = VERTICES_COUNT * sizeof(VertexInstancing);
    vdesc.initialData = patchVertices;
    vdesc.usage = rhi::USAGE_STATICDRAW;
    patchVertexBuffer = rhi::CreateVertexBuffer(vdesc);

    rhi::IndexBuffer::Descriptor idesc;
    idesc.size = INDICES_COUNT * sizeof(uint16);
    idesc.initialData = patchIndices;
    idesc.usage = rhi::USAGE_STATICDRAW;
    patchIndexBuffer = rhi::CreateIndexBuffer(idesc);

#if defined(__DAVAENGINE_IPHONE__)
    SafeDeleteArray(patchVertices);
    SafeDeleteArray(patchIndices);
#else
    LockGuard<Mutex> lock(restoreDataMutex);
    bufferRestoreData.push_back({ patchVertexBuffer, reinterpret_cast<uint8*>(patchVertices), vdesc.size, 0, RestoreBufferData::RESTORE_BUFFER_VERTEX });
    bufferRestoreData.push_back({ patchIndexBuffer, reinterpret_cast<uint8*>(patchIndices), idesc.size, 0, RestoreBufferData::RESTORE_BUFFER_INDEX });
#endif

    RenderBatch* batch = new RenderBatch();
    batch->SetMaterial(landscapeMaterial);
    batch->SetSortingKey(LANDSCAPE_MATERIAL_SORTING_KEY);
    batch->vertexBuffer = patchVertexBuffer;
    batch->indexBuffer = patchIndexBuffer;
    batch->primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    batch->indexCount = INDICES_COUNT;
    batch->vertexCount = VERTICES_COUNT;

    rhi::VertexLayout vLayout;
    vLayout.AddStream(rhi::VDF_PER_VERTEX);
    vLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 4); //position + gluDirection
    vLayout.AddElement(rhi::VS_TEXCOORD, 1, rhi::VDT_FLOAT, 4); //edge mask
    vLayout.AddElement(rhi::VS_TEXCOORD, 2, rhi::VDT_FLOAT, 2); //vertex index + edgeMaskNull
    vLayout.AddStream(rhi::VDF_PER_INSTANCE);
    vLayout.AddElement(rhi::VS_TEXCOORD, 3, rhi::VDT_FLOAT, 3); //patch position + scale
    vLayout.AddElement(rhi::VS_TEXCOORD, 4, rhi::VDT_FLOAT, 4); //neighbour patch lodOffset
    if (renderMode == RENDERMODE_INSTANCING_MORPHING)
    {
        vLayout.AddElement(rhi::VS_TEXCOORD, 5, rhi::VDT_FLOAT, 4); //neighbour patch morph
        vLayout.AddElement(rhi::VS_TEXCOORD, 6, rhi::VDT_FLOAT, 3); //patch lod + morph + pixelMappingOffset
    }

    batch->vertexLayoutId = rhi::VertexLayout::UniqueId(vLayout);

    AddRenderBatch(batch);
    SafeRelease(batch);
}

void Landscape::DrawLandscapeInstancing()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    drawIndices = 0;
    activeRenderBatchArray.clear();

    for (int32 i = static_cast<int32>(usedInstanceDataBuffers.size()) - 1; i >= 0; --i)
    {
        if (rhi::SyncObjectSignaled(usedInstanceDataBuffers[i]->syncObject))
        {
            freeInstanceDataBuffers.push_back(usedInstanceDataBuffers[i]);
            RemoveExchangingWithLast(usedInstanceDataBuffers, i);
        }
    }

    uint32 patchesToRender = subdivision->GetTerminatedPatchesCount();
    if (patchesToRender)
    {
        InstanceDataBuffer* instanceDataBuffer = nullptr;
        if (freeInstanceDataBuffers.size())
        {
            instanceDataBuffer = freeInstanceDataBuffers.back();
            if (instanceDataBuffer->bufferSize < patchesToRender * instanceDataSize)
            {
                rhi::DeleteVertexBuffer(instanceDataBuffer->buffer);
                SafeDelete(instanceDataBuffer);
            }
            freeInstanceDataBuffers.pop_back();
        }

        if (!instanceDataBuffer)
        {
            instanceDataMaxCount = Max(instanceDataMaxCount, patchesToRender);

            rhi::VertexBuffer::Descriptor instanceBufferDesc;
            instanceBufferDesc.size = instanceDataMaxCount * instanceDataSize;
            instanceBufferDesc.usage = rhi::USAGE_DYNAMICDRAW;
            instanceBufferDesc.needRestore = false;

            instanceDataBuffer = new InstanceDataBuffer();
            instanceDataBuffer->bufferSize = instanceBufferDesc.size;
            instanceDataBuffer->buffer = rhi::CreateVertexBuffer(instanceBufferDesc);
        }
        usedInstanceDataBuffers.push_back(instanceDataBuffer);
        instanceDataBuffer->syncObject = rhi::GetCurrentFrameSyncObject();

        renderBatchArray[0].renderBatch->instanceBuffer = instanceDataBuffer->buffer;
        renderBatchArray[0].renderBatch->instanceCount = patchesToRender;
        activeRenderBatchArray.emplace_back(renderBatchArray[0].renderBatch);

        instanceDataPtr = static_cast<uint8*>(rhi::MapVertexBuffer(instanceDataBuffer->buffer, 0, patchesToRender * instanceDataSize));

        AddPatchToRender(0, 0, 0);

        rhi::UnmapVertexBuffer(instanceDataBuffer->buffer);
        instanceDataPtr = nullptr;

        drawIndices = activeRenderBatchArray[0]->indexCount * activeRenderBatchArray[0]->instanceCount;

        DAVA_PROFILER_GPU_RENDER_BATCH(activeRenderBatchArray[0], ProfilerGPUMarkerName::LANDSCAPE);
    }
}

inline float32 morphFunc(float32 x)
{
    float32 _x = 1.f - x;
    float32 _x4 = _x * _x; //(1 - x) ^ 2
    _x4 = _x4 * _x4; //(1 - x) ^ 4

    return _x4 * (4.f * _x - 5.f) + 1; //4*(1-x)^5 - 5*(1-x)^4 + 1
}

void Landscape::DrawPatchInstancing(uint32 level, uint32 xx, uint32 yy, const Vector4& neighbourLevel, float32 patchMorph /*= 0.f*/, const Vector4& neighbourMorph /*= Vector4()*/)
{
    const LandscapeSubdivision::SubdivisionLevelInfo& levelInfo = subdivision->GetLevelInfo(level);

    float32 levelf = float32(level);
    InstanceData* instanceData = reinterpret_cast<InstanceData*>(instanceDataPtr);

    instanceData->patchOffset = Vector2(float32(xx) / levelInfo.size, float32(yy) / levelInfo.size);
    instanceData->patchScale = 1.f / levelInfo.size;
    instanceData->neighbourPatchLodOffset = Vector4(levelf - neighbourLevel.x,
                                                    levelf - neighbourLevel.y,
                                                    levelf - neighbourLevel.z,
                                                    levelf - neighbourLevel.w);

    if (renderMode == RENDERMODE_INSTANCING_MORPHING)
    {
        int32 baseLod = subdivision->GetLevelCount() - level - 1;

        instanceData->neighbourPatchMorph = Vector4(morphFunc(neighbourMorph.x),
                                                    morphFunc(neighbourMorph.y),
                                                    morphFunc(neighbourMorph.z),
                                                    morphFunc(neighbourMorph.w));
        instanceData->patchLod = float32(baseLod);
        instanceData->patchMorph = morphFunc(patchMorph);
        instanceData->centerPixelOffset = .5f / (1 << (heightmapSizePow2 - baseLod));
    }

    instanceDataPtr += instanceDataSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void Landscape::BindDynamicParameters(Camera* camera, RenderBatch* batch)
{
    RenderObject::BindDynamicParameters(camera, batch);

    if (heightmap)
        Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_LANDSCAPE_HEIGHTMAP_TEXTURE_SIZE, &heightmapSizef, pointer_size(&heightmapSizef));
}

void Landscape::PrepareToRender(Camera* camera)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::RENDER_PREPARE_LANDSCAPE)

    RenderObject::PrepareToRender(camera);

    if (GetHeightmapSize() == 0)
    {
        return;
    }

    if (!subdivision->GetLevelCount() || !Renderer::GetOptions()->IsOptionEnabled(RenderOptions::LANDSCAPE_DRAW))
    {
        return;
    }

    if (Renderer::GetOptions()->IsOptionEnabled(RenderOptions::UPDATE_LANDSCAPE_LODS))
    {
        subdivision->PrepareSubdivision(camera, worldTransform);
    }

    switch (renderMode)
    {
    case RENDERMODE_INSTANCING:
    case RENDERMODE_INSTANCING_MORPHING:
        DrawLandscapeInstancing();
        break;
    case RENDERMODE_NO_INSTANCING:
        DrawLandscapeNoInstancing();
    default:
        break;
    }
}

bool Landscape::GetLevel0Geometry(Vector<LandscapeVertex>& vertices, Vector<int32>& indices) const
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();
    uint32 hmSize = GetHeightmapSize();
    if (hmSize == 0)
    {
        return false;
    }

    uint32 gridWidth = hmSize + 1;
    uint32 gridHeight = hmSize + 1;
    vertices.resize(gridWidth * gridHeight);
    for (uint32 y = 0, index = 0; y < gridHeight; ++y)
    {
        float32 ny = static_cast<float32>(y) / static_cast<float32>(gridHeight - 1);
        for (uint32 x = 0; x < gridWidth; ++x)
        {
            float32 nx = static_cast<float32>(x) / static_cast<float32>(gridWidth - 1);
            vertices[index].position = heightmap->GetPoint(x, y, bbox);
            vertices[index].texCoord = Vector2(nx, ny);
            index++;
        }
    }

    indices.resize((gridWidth - 1) * (gridHeight - 1) * 6);
    for (uint32 y = 0, index = 0; y < gridHeight - 1; ++y)
    {
        for (uint32 x = 0; x < gridWidth - 1; ++x)
        {
            indices[index++] = x + y * gridWidth;
            indices[index++] = (x + 1) + y * gridWidth;
            indices[index++] = x + (y + 1) * gridWidth;
            indices[index++] = (x + 1) + y * gridWidth;
            indices[index++] = (x + 1) + (y + 1) * gridWidth;
            indices[index++] = x + (y + 1) * gridWidth;
        }
    }
    return true;
}

const FilePath& Landscape::GetHeightmapPathname()
{
    return heightmapPath;
}

void Landscape::SetHeightmapPathname(const FilePath& newHeightMapPath)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (newHeightMapPath == heightmapPath)
    {
        return;
    }
    BuildLandscapeFromHeightmapImage(newHeightMapPath, bbox);
}

float32 Landscape::GetLandscapeSize() const
{
    return bbox.GetSize().x;
}

void Landscape::SetLandscapeSize(float32 newSize)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Vector3 newLandscapeSize(newSize, newSize, bbox.GetSize().z);
    SetLandscapeSize(newLandscapeSize);
}

float32 Landscape::GetLandscapeHeight() const
{
    return bbox.GetSize().z;
}

void Landscape::SetLandscapeHeight(float32 newHeight)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Vector3 newLandscapeSize(bbox.GetSize().x, bbox.GetSize().y, newHeight);
    SetLandscapeSize(newLandscapeSize);
}

void Landscape::SetLandscapeSize(const Vector3& newLandscapeSize)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (newLandscapeSize.z < 0.0f || newLandscapeSize.x < 0 || newLandscapeSize.y < 0)
    {
        return;
    }
    if (newLandscapeSize == bbox.GetSize())
    {
        return;
    }
    bbox.Empty();
    bbox.AddPoint(Vector3(-newLandscapeSize.x / 2.f, -newLandscapeSize.y / 2.f, 0.f));
    bbox.AddPoint(Vector3(newLandscapeSize.x / 2.f, newLandscapeSize.y / 2.f, newLandscapeSize.z));
    RebuildLandscape();

    if (foliageSystem)
    {
        foliageSystem->SyncFoliageWithLandscape();
    }
}

void Landscape::GetDataNodes(Set<DataNode*>& dataNodes)
{
    NMaterial* curMaterialNode = landscapeMaterial;
    while (curMaterialNode != NULL)
    {
        dataNodes.insert(curMaterialNode);
        curMaterialNode = curMaterialNode->GetParent();
    }
}

void Landscape::Save(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    BaseObject::SaveObject(archive);

    archive->SetUInt32("ro.debugflags", debugFlags);
    archive->SetUInt32("ro.sOclIndex", staticOcclusionIndex);

    //VI: save only VISIBLE flag for now. May be extended in the future
    archive->SetUInt32("ro.flags", flags & RenderObject::SERIALIZATION_CRITERIA);

    uint64 matKey = landscapeMaterial->GetNodeID();
    archive->SetUInt64("matname", matKey);

    //TODO: remove code in future. Need for transition from *.png to *.heightmap
    if (!heightmapPath.IsEqualToExtension(Heightmap::FileExtension()))
    {
        heightmapPath.ReplaceExtension(Heightmap::FileExtension());
    }

    if (heightmap != nullptr)
    {
        heightmap->Save(heightmapPath);
    }
    archive->SetString("hmap", heightmapPath.GetRelativePathname(serializationContext->GetScenePath()));
    archive->SetByteArrayAsType("bbox", bbox);
}

void Landscape::Load(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    debugFlags = archive->GetUInt32("ro.debugflags", 0);
    staticOcclusionIndex = uint16(archive->GetUInt32("ro.sOclIndex", INVALID_STATIC_OCCLUSION_INDEX));

    //VI: load only VISIBLE flag for now. May be extended in the future.
    uint32 savedFlags = RenderObject::SERIALIZATION_CRITERIA & archive->GetUInt32("ro.flags", RenderObject::SERIALIZATION_CRITERIA);
    flags = (savedFlags | (flags & ~RenderObject::SERIALIZATION_CRITERIA));

    uint64 matKey = archive->GetUInt64("matname");

    if (!matKey) //Load from old landscape format: get material from batch0

    {
        uint32 roBatchCount = archive->GetUInt32("ro.batchCount");
        DVASSERT(roBatchCount);
        KeyedArchive* batchesArch = archive->GetArchive("ro.batches");
        DVASSERT(batchesArch);
        KeyedArchive* batchArch = batchesArch->GetArchive(KeyedArchive::GenKeyFromIndex(0));
        DVASSERT(batchArch);

        matKey = batchArch->GetUInt64("rb.nmatname");
    }

    DVASSERT(matKey);
    NMaterial* material = static_cast<NMaterial*>(serializationContext->GetDataBlock(matKey));
    if (material)
    {
        //Import old params
        if (!material->HasLocalProperty(NMaterialParamName::PARAM_LANDSCAPE_TEXTURE_TILING))
        {
            if (archive->IsKeyExists("tiling_0"))
            {
                Vector2 tilingValue;
                tilingValue = archive->GetByteArrayAsType("tiling_0", tilingValue);
                material->AddProperty(NMaterialParamName::PARAM_LANDSCAPE_TEXTURE_TILING, tilingValue.data, rhi::ShaderProp::TYPE_FLOAT2);
            }
            else if (material->HasLocalProperty(NMaterialParamName::DEPRECATED_LANDSCAPE_TEXTURE_0_TILING))
            {
                material->AddProperty(NMaterialParamName::PARAM_LANDSCAPE_TEXTURE_TILING, material->GetLocalPropValue(NMaterialParamName::DEPRECATED_LANDSCAPE_TEXTURE_0_TILING), rhi::ShaderProp::TYPE_FLOAT2);
                for (int32 i = 0; i < 4; i++)
                {
                    FastName propName(Format("texture%dTiling", i));
                    if (material->HasLocalProperty(propName))
                        material->RemoveProperty(propName);
                }
            }
        }

        PrepareMaterial(material);
        material->PreBuildMaterial(PASS_FORWARD);

        SetMaterial(material);
    }

    FilePath heightmapPath = serializationContext->GetScenePath() + archive->GetString("hmap");
    AABBox3 loadedBbox = archive->GetByteArrayAsType("bbox", AABBox3());

    BuildLandscapeFromHeightmapImage(heightmapPath, loadedBbox);
}

Heightmap* Landscape::GetHeightmap()
{
    return heightmap;
}

void Landscape::SetHeightmap(DAVA::Heightmap* height)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    SafeRelease(heightmap);
    heightmap = SafeRetain(height);

    RebuildLandscape();
}

NMaterial* Landscape::GetMaterial()
{
    return landscapeMaterial;
}

void Landscape::SetMaterial(NMaterial* material)
{
    SafeRetain(material);
    SafeRelease(landscapeMaterial);
    landscapeMaterial = material;

    for (uint32 i = 0; i < GetRenderBatchCount(); ++i)
        GetRenderBatch(i)->SetMaterial(landscapeMaterial);

    UpdateMaterialFlags();
}

RenderObject* Landscape::Clone(RenderObject* newObject)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (!newObject)
    {
        DVASSERT(IsPointerToExactClass<Landscape>(this), "Can clone only Landscape");
        newObject = new Landscape();
    }

    Landscape* newLandscape = static_cast<Landscape*>(newObject);

    RefPtr<NMaterial> material(landscapeMaterial->Clone());
    newLandscape->SetMaterial(material.Get());

    newLandscape->flags = flags;
    newLandscape->BuildLandscapeFromHeightmapImage(heightmapPath, bbox);

    return newObject;
}

int32 Landscape::GetDrawIndices() const
{
    return drawIndices;
}

void Landscape::SetFoliageSystem(FoliageSystem* _foliageSystem)
{
    foliageSystem = _foliageSystem;
}

void Landscape::ResizeIndicesBufferIfNeeded(DAVA::uint32 newSize)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (indices.size() < newSize)
    {
        indices.resize(2 * newSize);
    }
};

void Landscape::SetForceMaxSubdiv(bool force)
{
    subdivision->SetForceMaxSubdivision(force);
}

void Landscape::SetUpdatable(bool isUpdatable)
{
    if (updatable != isUpdatable)
    {
        updatable = isUpdatable;
        RebuildLandscape();
    }
}

bool Landscape::IsUpdatable() const
{
    return updatable;
}

void Landscape::SetDrawWired(bool isWired)
{
    landscapeMaterial->SetFXName(isWired ? NMaterialName::TILE_MASK_DEBUG : NMaterialName::TILE_MASK);
}

bool Landscape::IsDrawWired() const
{
    return landscapeMaterial->GetEffectiveFXName() == NMaterialName::TILE_MASK_DEBUG;
}

void Landscape::SetUseInstancing(bool isUse)
{
    RenderMode newRenderMode = (isUse && rhi::DeviceCaps().isInstancingSupported) ? RENDERMODE_INSTANCING : RENDERMODE_NO_INSTANCING;
    SetRenderMode(newRenderMode);
}

bool Landscape::IsUseInstancing() const
{
    return (renderMode == RENDERMODE_INSTANCING || renderMode == RENDERMODE_INSTANCING_MORPHING);
}

void Landscape::SetUseMorphing(bool useMorph)
{
    RenderMode newRenderMode = useMorph ? RENDERMODE_INSTANCING_MORPHING : RENDERMODE_INSTANCING;
    newRenderMode = rhi::DeviceCaps().isInstancingSupported ? newRenderMode : RENDERMODE_NO_INSTANCING;
    SetRenderMode(newRenderMode);
}

bool Landscape::IsUseMorphing() const
{
    return (renderMode == RENDERMODE_INSTANCING_MORPHING);
}

void Landscape::SetRenderMode(RenderMode newRenderMode)
{
    if (renderMode == newRenderMode)
        return;

    renderMode = newRenderMode;
    RebuildLandscape();
    UpdateMaterialFlags();
}

void Landscape::UpdateMaterialFlags()
{
    landscapeMaterial->SetFlag(NMaterialFlagName::FLAG_LANDSCAPE_USE_INSTANCING, (renderMode == RENDERMODE_NO_INSTANCING) ? 0 : 1);
    landscapeMaterial->SetFlag(NMaterialFlagName::FLAG_LANDSCAPE_LOD_MORPHING, (renderMode == RENDERMODE_INSTANCING_MORPHING) ? 1 : 0);
    landscapeMaterial->PreBuildMaterial(PASS_FORWARD);
}

void Landscape::SetDrawMorphing(bool drawMorph)
{
    if (debugDrawMorphing != drawMorph)
    {
        debugDrawMorphing = drawMorph;
        landscapeMaterial->SetFlag(NMaterialFlagName::FLAG_LANDSCAPE_MORPHING_COLOR, debugDrawMorphing ? 1 : 0);
    }
}

bool Landscape::IsDrawMorphing() const
{
    return debugDrawMorphing;
}

void Landscape::UpdatePart(const Rect2i& rect)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    subdivision->UpdatePatchInfo(rect);

    switch (renderMode)
    {
    case RENDERMODE_INSTANCING:
    case RENDERMODE_INSTANCING_MORPHING:
    {
        Vector<Image*> heightTextureData = CreateHeightTextureData(heightmap, renderMode);

        for (Image* img : heightTextureData)
        {
            heightTexture->TexImage(img->mipmapLevel, img->width, img->height, img->data, img->dataSize, img->cubeFaceID);
            img->Release();
        }
        heightTextureData.clear();

        if (isRequireTangentBasis)
        {
            Vector<Image*> tangentTextureData = CreateTangentBasisTextureData();
            for (Image* img : tangentTextureData)
            {
                tangentTexture->TexImage(img->mipmapLevel, img->width, img->height, img->data, img->dataSize, img->cubeFaceID);
                img->Release();
            }
            tangentTextureData.clear();
        }
    }
    break;
    case RENDERMODE_NO_INSTANCING:
    {
        DVASSERT(false && "TODO: Landscape::UpdatePart() for non-instancing render");
    }
    break;
    default:
        break;
    }
}

void Landscape::RecursiveRayTrace(uint32 level, uint32 x, uint32 y, const Ray3& rayInObjectSpace, float32& resultT)
{
#if 0
	const LandscapeSubdivision::SubdivisionPatchInfo& subdivPatchInfo = subdivision->GetPatchInfo(level, x, y);
	const LandscapeSubdivision::PatchQuadInfo& patchQuadInfo = subdivision->GetPatchQuadInfo(level, x, y);

	float32 tMin, tMax;
	bool currentPatchIntersects = Intersection::RayBox(rayInObjectSpace, patchQuadInfo.bbox, tMin, tMax);

	if (currentPatchIntersects)
	{
		if (level == subdivision->GetLevelCount() - 1)
		{
			int32 hmSize = heightmap->Size();
			uint32 patchSize = hmSize >> level;

			uint32 xx = x * patchSize;
			uint32 yy = y * patchSize;

			float32 localResult = FLT_MAX;
			/*int32 collisionCellX2 = -1;
			int32 collisionCellY2 = -1;

			float32 localResult2 = FLT_MAX;
			// Bruteforce algorithm
			{
			for (uint32 y0 = yy; y0 < yy + patchSize; y0 ++)
			{
			for (uint32 x0 = xx; x0 < xx + patchSize; x0 ++)
			{
			uint32 x1 = x0 + 1;
			uint32 y1 = y0 + 1;

			Vector3 p00 = heightmap->GetPoint(x0, y0, bbox);
			Vector3 p01 = heightmap->GetPoint(x0, y1, bbox);
			Vector3 p10 = heightmap->GetPoint(x1, y0, bbox);
			Vector3 p11 = heightmap->GetPoint(x1, y1, bbox);

			float32 t1, t2;
			if (Intersection::RayTriangle(rayInObjectSpace, p00, p01, p11, t1))
			{
			if (t1 < localResult2)
			{
			localResult2 = t1;
			collisionCellX2 = x0;
			collisionCellY2 = y0;
			}
			}

			if (Intersection::RayTriangle(rayInObjectSpace, p00, p11, p10, t2))
			{
			if (t2 < localResult2)
			{
			localResult2 = t2;
			collisionCellX2 = x0;
			collisionCellY2 = y0;

			}
			}
			}
			}
			}

			//Logger::Debug("bf res: %d %d" , collisionCellX2, collisionCellY2);

			//DVASSERT(FLOAT_EQUAL(localResult2, localResult));
			resultT = Min(resultT, localResult2);*/
			float32 localResult3 = FLT_MAX;
			{
				Vector3 grid00 = heightmap->GetPoint(xx, yy, bbox);
				Vector3 gridCellSize = heightmap->GetPoint(xx + 1, yy + 1, bbox) - grid00;

				auto GetPosToVoxel = [patchSize, grid00, gridCellSize](const Vector3 & point, int32 axis)
				{
					int32 pos = static_cast<int32>((point.data[axis] - grid00.data[axis]) / (float32)gridCellSize.data[axis]);
					return Clamp(pos, 0, (int32)patchSize - 1);
				};

				auto GetVoxelToPos = [patchSize, grid00, gridCellSize](int32 voxel, int32 axis)
				{
					float32 pos = (float32)(voxel)* gridCellSize.data[axis] + grid00.data[axis];
					return pos;
				};



				Vector3 patchIntersect = rayInObjectSpace.origin + rayInObjectSpace.direction * tMin;


				float32 width[3] = { gridCellSize.x, gridCellSize.y, gridCellSize.z };


				// Set up 3D DDA for ray
				float32 nextCrossingT[3], deltaT[3];
				int32 step[3], out[3], pos[3];
				for (int axis = 0; axis < 2; ++axis)
				{
					// Compute current voxel for axis
					pos[axis] = GetPosToVoxel(patchIntersect, axis);
					if (rayInObjectSpace.direction.data[axis] >= 0)
					{
						// Handle ray with positive direction for voxel stepping
						nextCrossingT[axis] = tMin + (GetVoxelToPos(pos[axis] + 1, axis) - patchIntersect.data[axis]) / rayInObjectSpace.direction.data[axis];
						deltaT[axis] = width[axis] / rayInObjectSpace.direction.data[axis];
						step[axis] = 1;
						out[axis] = patchSize;
					}
					else
					{
						// Handle ray with negative direction for voxel stepping
						nextCrossingT[axis] = tMin + (GetVoxelToPos(pos[axis], axis) - patchIntersect.data[axis]) / rayInObjectSpace.direction.data[axis];
						deltaT[axis] = -width[axis] / rayInObjectSpace.direction.data[axis];
						step[axis] = -1;
						out[axis] = -1;
					}
				}

				bool hitSomething = false;
				for (;;)
				{
					//Logger::Debug("na: %d %d", xx + pos[0], yy + pos[1]);
					// Check for intersection in current voxel and advance to next
					Vector3 p00 = heightmap->GetPoint(xx + pos[0], yy + pos[1], bbox);
					Vector3 p01 = heightmap->GetPoint(xx + pos[0], yy + pos[1] + 1, bbox);
					Vector3 p10 = heightmap->GetPoint(xx + pos[0] + 1, yy + pos[1], bbox);
					Vector3 p11 = heightmap->GetPoint(xx + pos[0] + 1, yy + pos[1] + 1, bbox);

					float32 t1, t2;
					if (Intersection::RayTriangle(rayInObjectSpace, p00, p01, p11, t1))
					{
						if (t1 < localResult3)
						{
							localResult3 = t1;
						}
					}

					if (Intersection::RayTriangle(rayInObjectSpace, p00, p11, p10, t2))
					{
						if (t2 < localResult3)
						{
							localResult3 = t2;
						}
					}


					// Advance to next voxel

					// Find _stepAxis_ for stepping to next voxel
					/*int bits = (   (nextCrossingT[0] < nextCrossingT[1]) << 2)  // X < Y
					+ ((nextCrossingT[0] < nextCrossingT[2]) << 1)  // X < Z
					+ ((nextCrossingT[1] < nextCrossingT[2]));      // Y < Z
					//    0 - ZYX
					//    7 - XYZ


					const int cmpToAxis[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
					int stepAxis = cmpToAxis[bits];
					*/


					int32 stepAxis = 0;
					if (nextCrossingT[0] < nextCrossingT[1])stepAxis = 0;
					else stepAxis = 1;

					if (tMax < nextCrossingT[stepAxis])
						break;
					pos[stepAxis] += step[stepAxis];
					if (pos[stepAxis] == out[stepAxis])
						break;
					nextCrossingT[stepAxis] += deltaT[stepAxis];
				}
			}
			//if (!FLOAT_EQUAL(localResult2, localResult3))
			//            {
			//                int i = 0;
			//                Logger::Error("DDA Algorithm mistake");
			//            }

			resultT = Min(resultT, localResult3);


		} // End of intersection code


		  // If level is not final go deeper
		if ((level < subdivision->GetLevelCount() - 1))
		{
			uint32 x2 = x << 1;
			uint32 y2 = y << 1;

			RecursiveRayTrace(level + 1, x2 + 0, y2 + 0, rayInObjectSpace, resultT);
			RecursiveRayTrace(level + 1, x2 + 1, y2 + 0, rayInObjectSpace, resultT);
			RecursiveRayTrace(level + 1, x2 + 0, y2 + 1, rayInObjectSpace, resultT);
			RecursiveRayTrace(level + 1, x2 + 1, y2 + 1, rayInObjectSpace, resultT);
		}
	}
#endif
}

bool Landscape::RayTrace(const Ray3& rayInObjectSpace, float32& resultT)
{
    float32 localResultT = FLOAT_MAX;
    RecursiveRayTrace(0, 0, 0, rayInObjectSpace, localResultT);

    if (localResultT < FLOAT_MAX)
    {
        resultT = localResultT;
        return true;
    }
    return false;
}
}
