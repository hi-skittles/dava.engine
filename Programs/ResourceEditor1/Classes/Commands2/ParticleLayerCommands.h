#pragma once

#include "Base/RefPtr.h"
#include "Commands2/Base/RECommand.h"
#include "FileSystem/FilePath.h"
#include "Particles/ParticlePropertyLine.h"
#include "Render/RenderBase.h"

namespace DAVA
{
struct ParticleLayer;
class FilePath;
}

class CommandChangeLayerMaterialProperties : public RECommand
{
public:
    CommandChangeLayerMaterialProperties(DAVA::ParticleLayer* layer, const DAVA::FilePath& spritePath, DAVA::eBlending blending, bool enableFog, bool enableBlending);

    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const;

private:
    struct LayerParams
    {
        DAVA::FilePath spritePath;
        DAVA::eBlending blending = DAVA::BLENDING_NONE;
        bool enableFog = false;
        bool enableBlending = false;
    };

    void ApplyParams(const LayerParams& params);

private:
    LayerParams newParams;
    LayerParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
};

class CommandChangeFlowProperties : public RECommand
{
public:
    struct FlowParams
    {
        DAVA::FilePath spritePath;
        bool enableFlow = false;
        bool enabelFlowAnimation = false;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> flowSpeed;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> flowSpeedVariation;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> flowOffset;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> flowOffsetVariation;
    };

    CommandChangeFlowProperties(DAVA::ParticleLayer* layer_, FlowParams&& params);

    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const;

private:
    void ApplyParams(FlowParams& params);

    FlowParams newParams;
    FlowParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
};

class CommandChangeNoiseProperties : public RECommand
{
public:
    struct NoiseParams
    {
        DAVA::FilePath noisePath;
        bool enableNoise = false;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseScale;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseScaleVariation;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseScaleOverLife;
        bool enableNoiseScroll = false;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseUScrollSpeed;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseUScrollSpeedVariation;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseUScrollSpeedOverLife;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseVScrollSpeed;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseVScrollSpeedVariation;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> noiseVScrollSpeedOverLife;
    };

    CommandChangeNoiseProperties(DAVA::ParticleLayer* layer, NoiseParams&& params);

    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const;

private:
    void ApplyParams(NoiseParams& params);

    NoiseParams newParams;
    NoiseParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
};

class CommandChangeFresnelToAlphaProperties : public RECommand
{
public:
    struct FresnelToAlphaParams
    {
        bool useFresnelToAlpha = false;
        DAVA::float32 fresnelToAlphaBias = 0.0f;
        DAVA::float32 fresnelToAlphaPower = 0.0f;
    };

    CommandChangeFresnelToAlphaProperties(DAVA::ParticleLayer* layer, FresnelToAlphaParams&& params);

    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const;

private:
    void ApplyParams(FresnelToAlphaParams& params);

    FresnelToAlphaParams newParams;
    FresnelToAlphaParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
};

class CommandChangeParticlesStripeProperties : public RECommand
{
public:
    struct StripeParams
    {
        DAVA::float32 stripeVertexSpawnStep;
        DAVA::float32 stripeLifetime;
        DAVA::float32 stripeStartSize;
        DAVA::float32 stripeUScrollSpeed;
        DAVA::float32 stripeVScrollSpeed;
        DAVA::float32 stripeFadeDistanceFromTop;
        bool stripeInheritPositionForBase;
        bool usePerspectiveMapping;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> stripeTextureTileOverLife;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> stripeSizeOverLife;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> stripeNoiseUScrollSpeedOverLife;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> stripeNoiseVScrollSpeedOverLife;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> stripeColorOverLife;
    };

    CommandChangeParticlesStripeProperties(DAVA::ParticleLayer* layer, StripeParams&& params);

    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const;

private:
    void ApplyParams(StripeParams& params);

    StripeParams newParams;
    StripeParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
};

class CommandChangeAlphaRemapProperties : public RECommand
{
public:
    struct AlphaRemapParams
    {
        DAVA::FilePath alphaRemapPath;
        bool enableAlphaRemap = false;
        DAVA::float32 alphaRemapLoopCount = 1.0f;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> alphaRemapOverLife;
    };

    CommandChangeAlphaRemapProperties(DAVA::ParticleLayer* layer, AlphaRemapParams&& params);

    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const;

private:
    void ApplyParams(AlphaRemapParams& params);

    AlphaRemapParams newParams;
    AlphaRemapParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
};

class CommandChangeThreePointGradientProperties : public RECommand
{
public:
    struct ThreePointGradientParams
    {
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> gradientColorForWhite;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> gradientColorForBlack;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> gradientColorForMiddle;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> gradientMiddlePointLine;
        DAVA::float32 gradientMiddlePoint = 0.5f;
        bool useThreePointGradient = false;
    };

    CommandChangeThreePointGradientProperties(DAVA::ParticleLayer* layer, ThreePointGradientParams&& params);
    void Undo() override;
    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const;

private:
    void ApplyParams(ThreePointGradientParams& params);

    ThreePointGradientParams newParams;
    ThreePointGradientParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
};