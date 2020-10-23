#pragma once

#include <random>

#include "OverdrawTestConfig.h"

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Base/String.h"
#include "Render/RenderBase.h"
#include "Functional/Function.h"
#include "Entity/SceneSystem.h"
#include "Math/Vector.h"

namespace DAVA
{
class Scene;
class NMaterial;
class Texture;
}

namespace OverdrawPerformanceTester
{
class OverdrawTesterRenderObject;

class OverdrawTesterSystem : public DAVA::SceneSystem
{
public:
    OverdrawTesterSystem(DAVA::Scene* scene, DAVA::PixelFormat textureFormat_, DAVA::uint16 textureResolution_, DAVA::Function<void(DAVA::Array<DAVA::Vector<FrameData>, 6>*)> finishCallback_);
    ~OverdrawTesterSystem();

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    virtual void Process(DAVA::float32 timeElapsed) override;

    inline DAVA::float32 GetCurrentOverdraw() const;
    inline DAVA::uint32 GetCurrentSampleCount() const;

private:
    DAVA::Texture* GenerateTexture(std::mt19937& rng, std::uniform_int_distribution<std::mt19937::result_type>& dist255);
    void GenerateTextureData(std::mt19937& rng, std::uniform_int_distribution<std::mt19937::result_type>& dist255, unsigned char* data);
    void SetupMaterial(const DAVA::FastName* texture);
    void Restore();

    DAVA::Vector<OverdrawTesterRenderObject*> activeRenderObjects;
    DAVA::Array<DAVA::Vector<FrameData>, 6> performanceData;
    DAVA::Vector<DAVA::Texture*> textures;

    DAVA::Function<void(DAVA::Array<DAVA::Vector<FrameData>, 6>*)> finishCallback;

    DAVA::uint32 currentStepsCount = 1;
    DAVA::uint32 maxStepsCount = 100;
    DAVA::uint32 textureSampleCount = 0;
    DAVA::float32 overdrawPercent = 10.0f;

    DAVA::NMaterial* overdrawMaterial = nullptr;
    DAVA::Array<DAVA::float32, 20> frames;

    DAVA::PixelFormat textureFormat;
    DAVA::uint16 textureResolution;

    bool isFinished = false;
};

DAVA::float32 OverdrawTesterSystem::GetCurrentOverdraw() const
{
    return overdrawPercent * currentStepsCount;
}

DAVA::uint32 OverdrawTesterSystem::GetCurrentSampleCount() const
{
    return textureSampleCount;
}
}