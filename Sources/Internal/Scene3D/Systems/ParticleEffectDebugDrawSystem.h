#pragma once

#include "Base/BaseTypes.h"

#include "Entity/SceneSystem.h"
#include "Math/Vector.h"

namespace DAVA
{
class ParticleDebugRenderPass;
class ParticleDebugDrawQuadRenderPass;
class Texture;
class NMaterial;
class RenderObject;
class RenderSystem;

enum eParticleDebugDrawMode
{
    WIREFRAME,
    LOW_ALPHA,
    OVERDRAW
};

class ParticleEffectDebugDrawSystem : public SceneSystem
{
public:
    ParticleEffectDebugDrawSystem(Scene* scene);
    ~ParticleEffectDebugDrawSystem();

    void PrepareForRemove() override
    {
    }
    void Draw();

    void GenerateDebugMaterials();
    void GenerateQuadMaterials();

    Texture* GenerateHeatTexture() const;
    Vector4 LerpColors(float normalizedWidth) const;

    inline eParticleDebugDrawMode GetDrawMode() const;
    inline void SetDrawMode(eParticleDebugDrawMode mode);

    inline bool GetIsDrawOnlySected() const;
    inline void SetIsDrawOnlySelected(bool showOnlySelected);

    inline const Vector<NMaterial*>* const GetMaterials() const;

    void SetSelectedParticles(UnorderedSet<RenderObject*> selectedParticles);
    void SetAlphaThreshold(float32 threshold);

private:
    struct TextureKey
    {
        Vector4 color = {};
        float32 time = 0.0f;
        TextureKey(Vector4 color, float32 time)
            : color(color)
            , time(time)
        {
        }
    };

    void Restore();
    void GenerateHeatTextureData(unsigned char* data, size_t dataSize, int32 width, int32 height = 1) const;

    UnorderedSet<RenderObject*> selectedParticles;
    ParticleDebugRenderPass* renderPass = nullptr;
    ParticleDebugDrawQuadRenderPass* drawQuadPass = nullptr;
    RenderSystem* renderSystem = nullptr;

    bool isDrawOnlySelected = false;
    eParticleDebugDrawMode drawMode = OVERDRAW;

    NMaterial* wireframeMaterial = nullptr;
    NMaterial* overdrawMaterial = nullptr;
    NMaterial* showAlphaMaterial = nullptr;

    NMaterial* quadMaterial = nullptr;
    NMaterial* quadHeatMaterial = nullptr;

    Texture* heatTexture = nullptr;

    Vector<NMaterial*> materials;
};

eParticleDebugDrawMode ParticleEffectDebugDrawSystem::GetDrawMode() const
{
    return drawMode;
}

void ParticleEffectDebugDrawSystem::SetDrawMode(eParticleDebugDrawMode mode)
{
    drawMode = mode;
}

bool ParticleEffectDebugDrawSystem::GetIsDrawOnlySected() const
{
    return isDrawOnlySelected;
}

void ParticleEffectDebugDrawSystem::SetIsDrawOnlySelected(bool enable)
{
    isDrawOnlySelected = enable;
}

const Vector<NMaterial*>* const ParticleEffectDebugDrawSystem::GetMaterials() const
{
    return &materials;
}
}