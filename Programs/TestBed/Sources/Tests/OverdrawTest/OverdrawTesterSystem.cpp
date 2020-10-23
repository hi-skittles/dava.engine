#include "OverdrawTesterSystem.h"

#include <random>
#include <numeric>

#include "OverdrawTesterComponent.h"
#include "OverdrawTesterRenderObject.h"

#include "Base/String.h"
#include "Functional/Function.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Render/Renderer.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Render/Image/Image.h"
#include "Render/RHI/rhi_Type.h"
#include "Render/Texture.h"
#include "Time/SystemTimer.h"
#include "Utils/StringFormat.h"

namespace OverdrawPerformanceTester
{
using DAVA::Array;
using DAVA::String;

using DAVA::FastName;
using DAVA::Texture;
using DAVA::NMaterial;

using DAVA::uint32;
using DAVA::int32;
using DAVA::float32;
using DAVA::uint8;
using DAVA::Vector4;

using DAVA::Scene;
using DAVA::Function;
using DAVA::Entity;
using DAVA::PixelFormat;

const Array<FastName, 4> textureNames =
{ {
FastName("t1"),
FastName("t2"),
FastName("t3"),
FastName("t4")
} };

const uint8 maxTexturesCount = 4;
const FastName materialPath("~res:/TestBed/CustomMaterials/OverdrawTester.material");
const FastName sampleCountKeyword("SAMPLE_COUNT");
const FastName dependentReadKeyword("DEPENDENT_READ_TEST");
const uint32 accumulatedFramesCount = 20;
const bool generateTexWithMips = true;

OverdrawTesterSystem::OverdrawTesterSystem(DAVA::Scene* scene, DAVA::PixelFormat textureFormat_, DAVA::uint16 textureResolution_, DAVA::Function<void(DAVA::Array<DAVA::Vector<FrameData>, 6>*)> finishCallback_)
    : SceneSystem(scene)
    , textureFormat(textureFormat_)
    , textureResolution(textureResolution_)
    , finishCallback(finishCallback_)
{
    overdrawMaterial = new NMaterial();
    overdrawMaterial->SetFXName(materialPath);
    overdrawMaterial->AddFlag(FastName("SAMPLE_COUNT"), 0);
    overdrawMaterial->PreBuildMaterial(DAVA::PASS_FORWARD);

    if (textureFormat == DAVA::FORMAT_A8)
        overdrawMaterial->AddFlag(FastName("ALPHA8"), 1);

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist255(1, 255);

    for (uint32 i = 0; i < maxTexturesCount; i++)
    {
        textures.push_back(GenerateTexture(rng, dist255));
    }
    DAVA::Renderer::GetSignals().needRestoreResources.Connect(this, &OverdrawTesterSystem::Restore);
}

OverdrawTesterSystem::~OverdrawTesterSystem()
{
    SafeRelease(overdrawMaterial);
    for (auto tex : textures)
    {
        SafeRelease(tex);
    }
    textures.clear();
    DAVA::Renderer::GetSignals().needRestoreResources.Disconnect(this);
}

void OverdrawTesterSystem::AddEntity(DAVA::Entity* entity)
{
    OverdrawTesterComponent* comp = entity->GetComponent<OverdrawTesterComponent>();
    if (comp != nullptr)
    {
        maxStepsCount = comp->GetStepsCount();
        overdrawPercent = comp->GetStepOverdraw();

        OverdrawTesterRenderObject* renderObject = comp->GetRenderObject();
        renderObject->SetDrawMaterial(overdrawMaterial);
        GetScene()->GetRenderSystem()->RenderPermanent(renderObject);
        activeRenderObjects.push_back(renderObject);
    }
}

void OverdrawTesterSystem::RemoveEntity(Entity* entity)
{
    OverdrawTesterComponent* comp = entity->GetComponent<OverdrawTesterComponent>();
    if (comp != nullptr)
    {
        GetScene()->GetRenderSystem()->RemoveFromRender(comp->GetRenderObject());
        auto it = std::find(activeRenderObjects.begin(), activeRenderObjects.end(), comp->GetRenderObject());

        DVASSERT(it != activeRenderObjects.end());
        activeRenderObjects.erase(it);
    }
}

void OverdrawTesterSystem::PrepareForRemove()
{
    DAVA::RenderSystem* renderSystem = GetScene()->GetRenderSystem();
    for (OverdrawTesterRenderObject* ro : activeRenderObjects)
    {
        renderSystem->RemoveFromRender(ro);
    }
    activeRenderObjects.clear();
}

void OverdrawTesterSystem::Process(DAVA::float32 timeElapsed)
{
    if (isFinished)
        return;

    static int32 framesDrawn = 0;
    frames[framesDrawn] = DAVA::SystemTimer::GetRealFrameDelta();
    framesDrawn++;

    if (framesDrawn == accumulatedFramesCount)
    {
        float32 smoothFrametime = std::accumulate(frames.begin(), frames.end(), 0.0f) / static_cast<float32>(frames.size());
        performanceData[textureSampleCount].push_back({ smoothFrametime, GetCurrentOverdraw() });
        currentStepsCount++;
        framesDrawn = 0;
    }

    if (currentStepsCount > maxStepsCount)
    {
        currentStepsCount = 0;
        textureSampleCount++;
        if (textureSampleCount < maxTexturesCount + 1)
            SetupMaterial(&textureNames[textureSampleCount - 1]);
        else if (textureSampleCount == maxTexturesCount + 1)
            overdrawMaterial->AddFlag(dependentReadKeyword, 1);
        else
        {
            isFinished = true;
            if (finishCallback)
                finishCallback(&performanceData);
        }
    }

    for (auto renderObject : activeRenderObjects)
        renderObject->SetCurrentStepsCount(currentStepsCount);
}

DAVA::Texture* OverdrawTesterSystem::GenerateTexture(std::mt19937& rng, std::uniform_int_distribution<std::mt19937::result_type>& dist255)
{
    unsigned char* data = new unsigned char[textureResolution * textureResolution * 4];
    GenerateTextureData(rng, dist255, data);

    Texture* result;
    if (!generateTexWithMips)
    {
        result = DAVA::Texture::CreateFromData(textureFormat, data, textureResolution, textureResolution, false);
    }
    else
    {
        DAVA::Vector<DAVA::Image*> imageSet;

        DVASSERT(!(textureResolution & (textureResolution - 1)) && "Texture width must be power of two");
        for (uint32 dim = textureResolution; dim >= 1; dim /= 2)
        {
            imageSet.push_back(DAVA::Image::CreateFromData(dim, dim, textureFormat, data));
        }

        result = DAVA::Texture::CreateFromData(imageSet);
        for (DAVA::Image* mip : imageSet)
            SafeRelease(mip);
    }
    result->SetMinMagFilter(rhi::TEXFILTER_LINEAR, rhi::TEXFILTER_LINEAR, rhi::TEXMIPFILTER_LINEAR);
    result->SetWrapMode(rhi::TEXADDR_WRAP, rhi::TEXADDR_WRAP);
    delete[] data;
    return result;
}

void OverdrawTesterSystem::GenerateTextureData(std::mt19937& rng, std::uniform_int_distribution<std::mt19937::result_type>& dist255, unsigned char* data)
{
    uint32 dataIndex = 0;
    for (uint32 i = 0; i < textureResolution; i++)
        for (uint32 j = 0; j < textureResolution; j++)
        {
            data[dataIndex++] = static_cast<uint8>(dist255(rng));
            data[dataIndex++] = static_cast<uint8>(dist255(rng));
            data[dataIndex++] = static_cast<uint8>(dist255(rng));
            data[dataIndex++] = static_cast<uint8>(dist255(rng));
        }
}

void OverdrawTesterSystem::SetupMaterial(const DAVA::FastName* texture)
{
    overdrawMaterial->SetFlag(sampleCountKeyword, textureSampleCount);
    overdrawMaterial->AddTexture(*texture, textures[textureSampleCount - 1]);
}

void OverdrawTesterSystem::Restore()
{
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist255(1, 255);

    unsigned char* data = new unsigned char[textureResolution * textureResolution * 4];
    for (DAVA::Texture* texure : textures)
    {
        GenerateTextureData(rng, dist255, data);
        rhi::UpdateTexture(texure->handle, data, 0);
    }
    delete[] data;
}
}
