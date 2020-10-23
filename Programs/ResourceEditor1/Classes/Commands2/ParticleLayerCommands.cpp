#include "Commands2/ParticleLayerCommands.h"
#include "Commands2/RECommandIDs.h"

#include "FileSystem/FilePath.h"
#include "Particles/ParticleLayer.h"

using namespace DAVA;

CommandChangeLayerMaterialProperties::CommandChangeLayerMaterialProperties(ParticleLayer* layer_, const FilePath& spritePath, eBlending blending, bool enableFog, bool enableBlending)
    : RECommand(CMDID_PARTICLE_LAYER_CHANGED_MATERIAL_VALUES, "Change Layer properties")
    , layer(layer_)
{
    newParams.spritePath = spritePath;
    newParams.blending = blending;
    newParams.enableFog = enableFog;
    newParams.enableBlending = enableBlending;

    DVASSERT(layer != nullptr);
    if (layer != nullptr)
    {
        oldParams.spritePath = layer->spritePath;
        oldParams.blending = layer->blending;
        oldParams.enableFog = layer->enableFog;
        oldParams.enableBlending = layer->enableFrameBlend;
    }
}

void CommandChangeLayerMaterialProperties::Redo()
{
    ApplyParams(newParams);
}

void CommandChangeLayerMaterialProperties::Undo()
{
    ApplyParams(oldParams);
}

void CommandChangeLayerMaterialProperties::ApplyParams(const CommandChangeLayerMaterialProperties::LayerParams& params)
{
    if (layer != nullptr)
    {
        layer->SetSprite(params.spritePath);
        layer->blending = params.blending;
        layer->enableFog = params.enableFog;
        layer->enableFrameBlend = params.enableBlending;
    }
}

DAVA::ParticleLayer* CommandChangeLayerMaterialProperties::GetLayer() const
{
    return layer;
}

CommandChangeFlowProperties::CommandChangeFlowProperties(ParticleLayer* layer_, CommandChangeFlowProperties::FlowParams&& params)
    : RECommand(CMDID_PARTICLE_LAYER_CHANGED_FLOW_VALUES, "Change Flow Properties")
    , newParams(params)
    , layer(layer_)
{
    DVASSERT(layer != nullptr);
    if (layer != nullptr)
    {
        oldParams.spritePath = layer->spritePath;
        oldParams.enableFlow = layer->enableFlow;
        oldParams.enabelFlowAnimation = layer->enableFlowAnimation;
        oldParams.flowSpeed = layer->flowSpeed;
        oldParams.flowSpeedVariation = layer->flowSpeedVariation;
        oldParams.flowOffset = layer->flowOffset;
        oldParams.flowOffsetVariation = layer->flowOffsetVariation;
    }
}

void CommandChangeFlowProperties::Undo()
{
    ApplyParams(oldParams);
}

void CommandChangeFlowProperties::Redo()
{
    ApplyParams(newParams);
}

DAVA::ParticleLayer* CommandChangeFlowProperties::GetLayer() const
{
    return layer;
}

void CommandChangeFlowProperties::ApplyParams(FlowParams& params)
{
    if (layer != nullptr)
    {
        layer->enableFlow = params.enableFlow;
        layer->enableFlowAnimation = params.enabelFlowAnimation;
        layer->SetFlowmap(params.spritePath);
        PropertyLineHelper::SetValueLine(layer->flowSpeed, params.flowSpeed);
        PropertyLineHelper::SetValueLine(layer->flowSpeedVariation, params.flowSpeedVariation);
        PropertyLineHelper::SetValueLine(layer->flowOffset, params.flowOffset);
        PropertyLineHelper::SetValueLine(layer->flowOffsetVariation, params.flowOffsetVariation);
    }
}

CommandChangeNoiseProperties::CommandChangeNoiseProperties(DAVA::ParticleLayer* layer_, NoiseParams&& params)
    : RECommand(CMDID_PARTICLE_LAYER_CHANGED_NOISE_VALUES, "Change Noise Properties")
    , newParams(params)
    , layer(layer_)
{
    DVASSERT(layer != nullptr);
    if (layer != nullptr)
    {
        oldParams.noisePath = layer->noisePath;
        oldParams.enableNoise = layer->enableNoise;
        oldParams.noiseScale = layer->noiseScale;
        oldParams.noiseScaleVariation = layer->noiseScaleVariation;
        oldParams.noiseScaleOverLife = layer->noiseScaleOverLife;
        oldParams.enableNoiseScroll = layer->enableNoiseScroll;
        oldParams.noiseUScrollSpeed = layer->noiseUScrollSpeed;
        oldParams.noiseUScrollSpeedVariation = layer->noiseUScrollSpeedVariation;
        oldParams.noiseUScrollSpeedOverLife = layer->noiseUScrollSpeedOverLife;
        oldParams.noiseVScrollSpeed = layer->noiseVScrollSpeed;
        oldParams.noiseVScrollSpeedVariation = layer->noiseVScrollSpeedVariation;
        oldParams.noiseVScrollSpeedOverLife = layer->noiseVScrollSpeedOverLife;
    }
}

void CommandChangeNoiseProperties::Undo()
{
    ApplyParams(oldParams);
}

void CommandChangeNoiseProperties::Redo()
{
    ApplyParams(newParams);
}

DAVA::ParticleLayer* CommandChangeNoiseProperties::GetLayer() const
{
    return layer;
}

void CommandChangeNoiseProperties::ApplyParams(NoiseParams& params)
{
    if (layer != nullptr)
    {
        layer->enableNoise = params.enableNoise;
        layer->enableNoiseScroll = params.enableNoiseScroll;
        layer->SetNoise(params.noisePath);
        PropertyLineHelper::SetValueLine(layer->noiseScale, params.noiseScale);
        PropertyLineHelper::SetValueLine(layer->noiseScaleVariation, params.noiseScaleVariation);
        PropertyLineHelper::SetValueLine(layer->noiseScaleOverLife, params.noiseScaleOverLife);
        PropertyLineHelper::SetValueLine(layer->noiseUScrollSpeed, params.noiseUScrollSpeed);
        PropertyLineHelper::SetValueLine(layer->noiseUScrollSpeedVariation, params.noiseUScrollSpeedVariation);
        PropertyLineHelper::SetValueLine(layer->noiseUScrollSpeedOverLife, params.noiseUScrollSpeedOverLife);
        PropertyLineHelper::SetValueLine(layer->noiseVScrollSpeed, params.noiseVScrollSpeed);
        PropertyLineHelper::SetValueLine(layer->noiseVScrollSpeedVariation, params.noiseVScrollSpeedVariation);
        PropertyLineHelper::SetValueLine(layer->noiseVScrollSpeedOverLife, params.noiseVScrollSpeedOverLife);
    }
}

CommandChangeFresnelToAlphaProperties::CommandChangeFresnelToAlphaProperties(DAVA::ParticleLayer* layer_, FresnelToAlphaParams&& params)
    : RECommand(CMDID_PARTICLE_LAYER_CHANGED_FRES_TO_ALPHA_VALUES, "Change Fresnel to Alpha Properties")
    , newParams(params)
    , layer(layer_)
{
    DVASSERT(layer != nullptr);
    if (layer != nullptr)
    {
        oldParams.useFresnelToAlpha = layer->useFresnelToAlpha;
        oldParams.fresnelToAlphaPower = layer->fresnelToAlphaPower;
        oldParams.fresnelToAlphaBias = layer->fresnelToAlphaBias;
    }
}

void CommandChangeFresnelToAlphaProperties::Undo()
{
    ApplyParams(oldParams);
}

void CommandChangeFresnelToAlphaProperties::Redo()
{
    ApplyParams(newParams);
}

DAVA::ParticleLayer* CommandChangeFresnelToAlphaProperties::GetLayer() const
{
    return layer;
}

void CommandChangeFresnelToAlphaProperties::ApplyParams(FresnelToAlphaParams& params)
{
    if (layer != nullptr)
    {
        layer->useFresnelToAlpha = params.useFresnelToAlpha;
        layer->fresnelToAlphaBias = params.fresnelToAlphaBias;
        layer->fresnelToAlphaPower = params.fresnelToAlphaPower;
    }
}

CommandChangeParticlesStripeProperties::CommandChangeParticlesStripeProperties(DAVA::ParticleLayer* layer_, StripeParams&& params)
    : RECommand(CMDID_PARTICLE_LAYER_CHANGED_STRIPE_VALUES, "Change Stripe Properties")
    , newParams(params)
    , layer(layer_)
{
    DVASSERT(layer != nullptr);
    if (layer != nullptr)
    {
        oldParams.stripeLifetime = layer->stripeLifetime;
        oldParams.stripeVertexSpawnStep = layer->stripeVertexSpawnStep;
        oldParams.stripeStartSize = layer->stripeStartSize;
        oldParams.stripeTextureTileOverLife = layer->stripeTextureTileOverLife;
        oldParams.stripeUScrollSpeed = layer->stripeUScrollSpeed;
        oldParams.stripeVScrollSpeed = layer->stripeVScrollSpeed;
        oldParams.stripeFadeDistanceFromTop = layer->stripeFadeDistanceFromTop;
        oldParams.stripeInheritPositionForBase = layer->GetInheritPositionForStripeBase();
        oldParams.stripeSizeOverLife = layer->stripeSizeOverLife;
        oldParams.stripeNoiseUScrollSpeedOverLife = layer->stripeNoiseUScrollSpeedOverLife;
        oldParams.stripeNoiseVScrollSpeedOverLife = layer->stripeNoiseVScrollSpeedOverLife;
        oldParams.stripeColorOverLife = layer->stripeColorOverLife;
        oldParams.usePerspectiveMapping = layer->usePerspectiveMapping;
    }
}

void CommandChangeParticlesStripeProperties::Undo()
{
    ApplyParams(oldParams);
}

void CommandChangeParticlesStripeProperties::Redo()
{
    ApplyParams(newParams);
}

DAVA::ParticleLayer* CommandChangeParticlesStripeProperties::GetLayer() const
{
    return layer;
}

void CommandChangeParticlesStripeProperties::ApplyParams(StripeParams& params)
{
    if (layer != nullptr)
    {
        layer->stripeLifetime = params.stripeLifetime;
        layer->stripeVertexSpawnStep = params.stripeVertexSpawnStep;
        layer->stripeStartSize = params.stripeStartSize;
        layer->stripeUScrollSpeed = params.stripeUScrollSpeed;
        layer->stripeVScrollSpeed = params.stripeVScrollSpeed;
        layer->stripeFadeDistanceFromTop = params.stripeFadeDistanceFromTop;
        layer->SetInheritPositionForStripeBase(params.stripeInheritPositionForBase);
        layer->usePerspectiveMapping = params.usePerspectiveMapping;
        PropertyLineHelper::SetValueLine(layer->stripeTextureTileOverLife, params.stripeTextureTileOverLife);
        PropertyLineHelper::SetValueLine(layer->stripeSizeOverLife, params.stripeSizeOverLife);
        PropertyLineHelper::SetValueLine(layer->stripeNoiseUScrollSpeedOverLife, params.stripeNoiseUScrollSpeedOverLife);
        PropertyLineHelper::SetValueLine(layer->stripeNoiseVScrollSpeedOverLife, params.stripeNoiseVScrollSpeedOverLife);
        PropertyLineHelper::SetValueLine(layer->stripeColorOverLife, params.stripeColorOverLife);
        layer->isMaxStripeOverLifeDirty = true;
    }
}

CommandChangeAlphaRemapProperties::CommandChangeAlphaRemapProperties(DAVA::ParticleLayer* layer_, AlphaRemapParams&& params)
    : RECommand(CMDID_PARTICLE_LAYER_CHANGED_ALPHA_REMAP, "Change Alpha Remap Properties")
    , newParams(params)
    , layer(layer_)
{
    DVASSERT(layer != nullptr);
    if (layer != nullptr)
    {
        oldParams.alphaRemapOverLife = layer->alphaRemapOverLife;
        oldParams.alphaRemapPath = layer->alphaRemapPath;
        oldParams.enableAlphaRemap = layer->enableAlphaRemap;
        oldParams.alphaRemapLoopCount = layer->alphaRemapLoopCount;
    }
}

void CommandChangeAlphaRemapProperties::Undo()
{
    ApplyParams(oldParams);
}

void CommandChangeAlphaRemapProperties::Redo()
{
    ApplyParams(newParams);
}

DAVA::ParticleLayer* CommandChangeAlphaRemapProperties::GetLayer() const
{
    return layer;
}

void CommandChangeAlphaRemapProperties::ApplyParams(AlphaRemapParams& params)
{
    if (layer != nullptr)
    {
        layer->alphaRemapOverLife = params.alphaRemapOverLife;
        layer->SetAlphaRemap(params.alphaRemapPath);
        layer->enableAlphaRemap = params.enableAlphaRemap;
        layer->alphaRemapLoopCount = params.alphaRemapLoopCount;
    }
}

CommandChangeThreePointGradientProperties::CommandChangeThreePointGradientProperties(DAVA::ParticleLayer* layer_, ThreePointGradientParams&& params)
    : RECommand(CMDID_PARTICLE_LAYER_CHANGED_THREE_POINT_GRADIENT, "Change Three Point Gradient Properties")
    , newParams(params)
    , layer(layer_)
{
    DVASSERT(layer != nullptr);
    if (layer != nullptr)
    {
        oldParams.gradientColorForBlack = layer->gradientColorForBlack;
        oldParams.gradientColorForMiddle = layer->gradientColorForMiddle;
        oldParams.gradientColorForWhite = layer->gradientColorForWhite;
        oldParams.useThreePointGradient = layer->useThreePointGradient;
        oldParams.gradientMiddlePointLine = layer->gradientMiddlePointLine;
        oldParams.gradientMiddlePoint = layer->gradientMiddlePoint;
    }
}

void CommandChangeThreePointGradientProperties::Undo()
{
    ApplyParams(oldParams);
}

void CommandChangeThreePointGradientProperties::Redo()
{
    ApplyParams(newParams);
}

DAVA::ParticleLayer* CommandChangeThreePointGradientProperties::GetLayer() const
{
    return layer;
}

void CommandChangeThreePointGradientProperties::ApplyParams(ThreePointGradientParams& params)
{
    if (layer != nullptr)
    {
        PropertyLineHelper::SetValueLine(layer->gradientColorForBlack, params.gradientColorForBlack);
        PropertyLineHelper::SetValueLine(layer->gradientColorForMiddle, params.gradientColorForMiddle);
        PropertyLineHelper::SetValueLine(layer->gradientColorForWhite, params.gradientColorForWhite);
        PropertyLineHelper::SetValueLine(layer->gradientMiddlePointLine, params.gradientMiddlePointLine);

        layer->gradientMiddlePoint = params.gradientMiddlePoint;
        layer->useThreePointGradient = params.useThreePointGradient;
    }
}
