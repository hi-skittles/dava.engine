#include "Particles/ParticleLayer.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleEmitterInstance.h"
#include "Utils/StringFormat.h"
#include "Render/Image/Image.h"
#include "FileSystem/FileSystem.h"
#include "Logger/Logger.h"
#include "Particles/ParticleForce.h"

#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(ParticleLayer)
{
    ReflectionRegistrator<ParticleLayer>::Begin()
    .End();
}

using ForceShape = ParticleForce::eShape;
using ForceTimingType = ParticleForce::eTimingType;
using ForceType = ParticleForce::eType;

namespace ParticleLayerDetail
{
struct ShapeMap
{
    ForceShape elemType;
    String name;
};
const Array<ShapeMap, 2> shapeMap =
{ {
{ ForceShape::BOX, "box" },
{ ForceShape::SPHERE, "sphere" }
} };

struct TimingTypeMap
{
    ForceTimingType elemType;
    String name;
};
const Array<TimingTypeMap, 4> timingTypesMap =
{ {
{ ForceTimingType::CONSTANT, "const" },
{ ForceTimingType::OVER_LAYER_LIFE, "ovr_layer" },
{ ForceTimingType::OVER_PARTICLE_LIFE, "ovr_prt" },
{ ForceTimingType::SECONDS_PARTICLE_LIFE, "sec" }
} };

struct ForceTypeMap
{
    ForceType elemType;
    String name;
};
const Array<ForceTypeMap, 6> forceTypesMap =
{ {
{ ForceType::DRAG_FORCE, "drag" },
{ ForceType::VORTEX, "vortex" },
{ ForceType::POINT_GRAVITY, "pointgr" },
{ ForceType::PLANE_COLLISION, "plncoll" },
{ ForceType::GRAVITY, "grav" },
{ ForceType::WIND, "wind" }
} };

template <typename T, typename U, size_t sz>
T StringToType(const String& typeName, T defaultVal, const Array<U, sz> map)
{
    for (const auto& e : map)
    {
        if (e.name == typeName)
            return e.elemType;
    }

    return defaultVal;
}

template <typename T, typename U, size_t sz>
String TypeToString(T type, const String& defaultName, const Array<U, sz> map)
{
    for (const auto& e : map)
    {
        if (e.elemType == type)
            return e.name;
    }
    return defaultName;
}
}

const ParticleLayer::LayerTypeNamesInfo ParticleLayer::layerTypeNamesInfoMap[] =
{
  { TYPE_SINGLE_PARTICLE, "single" },
  { TYPE_PARTICLES, "particles" },
  { TYPE_PARTICLE_STRIPE, "particlesStripe" },
  { TYPE_SUPEREMITTER_PARTICLES, "superEmitter" }
};

/*the following code is legacy compatibility to load original particle blending nodes*/
enum eBlendMode
{
    BLEND_NONE = 0,
    BLEND_ZERO,
    BLEND_ONE,
    BLEND_DST_COLOR,
    BLEND_ONE_MINUS_DST_COLOR,
    BLEND_SRC_ALPHA,
    BLEND_ONE_MINUS_SRC_ALPHA,
    BLEND_DST_ALPHA,
    BLEND_ONE_MINUS_DST_ALPHA,
    BLEND_SRC_ALPHA_SATURATE,
    BLEND_SRC_COLOR,
    BLEND_ONE_MINUS_SRC_COLOR,

    BLEND_MODE_COUNT,
};
const char* BLEND_MODE_NAMES[BLEND_MODE_COUNT] =
{
  "BLEND_NONE",
  "BLEND_ZERO",
  "BLEND_ONE",
  "BLEND_DST_COLOR",
  "BLEND_ONE_MINUS_DST_COLOR",
  "BLEND_SRC_ALPHA",
  "BLEND_ONE_MINUS_SRC_ALPHA",
  "BLEND_DST_ALPHA",
  "BLEND_ONE_MINUS_DST_ALPHA",
  "BLEND_SRC_ALPHA_SATURATE",
  "BLEND_SRC_COLOR",
  "BLEND_ONE_MINUS_SRC_COLOR"
};

eBlendMode GetBlendModeByName(const String& blendStr)
{
    for (uint32 i = 0; i < BLEND_MODE_COUNT; i++)
        if (blendStr == BLEND_MODE_NAMES[i])
            return static_cast<eBlendMode>(i);

    return BLEND_MODE_COUNT;
}

/*end of legacy compatibility code*/

ParticleLayer::ParticleLayer()
{
    activeLODS.resize(4, true);
}

ParticleLayer::~ParticleLayer()
{
    SafeRelease(innerEmitter);

    CleanupForces();
    CleanupSimplifiedForces();
    // dynamic cache automatically delete all particles
}

ParticleLayer* ParticleLayer::Clone()
{
    ParticleLayer* dstLayer = new ParticleLayer();

    dstLayer->stripeLifetime = stripeLifetime;
    dstLayer->stripeVertexSpawnStep = stripeVertexSpawnStep;
    dstLayer->stripeStartSize = stripeStartSize;
    dstLayer->stripeUScrollSpeed = stripeUScrollSpeed;
    dstLayer->stripeVScrollSpeed = stripeVScrollSpeed;
    dstLayer->stripeFadeDistanceFromTop = stripeFadeDistanceFromTop;
    dstLayer->alphaOverLife = alphaOverLife;

    if (stripeSizeOverLife)
        dstLayer->stripeSizeOverLife.Set(stripeSizeOverLife->Clone());

    if (stripeTextureTileOverLife)
        dstLayer->stripeTextureTileOverLife.Set(stripeTextureTileOverLife->Clone());

    if (stripeNoiseUScrollSpeedOverLife)
        dstLayer->stripeNoiseUScrollSpeedOverLife.Set(stripeNoiseUScrollSpeedOverLife->Clone());

    if (stripeNoiseVScrollSpeedOverLife)
        dstLayer->stripeNoiseVScrollSpeedOverLife.Set(stripeNoiseVScrollSpeedOverLife->Clone());

    if (stripeColorOverLife)
        dstLayer->stripeColorOverLife.Set(stripeColorOverLife->Clone());

    if (flowSpeed)
        dstLayer->flowSpeed.Set(flowSpeed->Clone());
    if (flowSpeedVariation)
        dstLayer->flowSpeedVariation.Set(flowSpeedVariation->Clone());

    if (flowOffset)
        dstLayer->flowOffset.Set(flowOffset->Clone());
    if (flowOffsetVariation)
        dstLayer->flowOffsetVariation.Set(flowOffsetVariation->Clone());

    if (noiseScale)
        dstLayer->noiseScale.Set(noiseScale->Clone());
    if (noiseScaleVariation)
        dstLayer->noiseScaleVariation.Set(noiseScaleVariation->Clone());
    if (noiseScaleOverLife)
        dstLayer->noiseScaleOverLife.Set(noiseScaleOverLife->Clone());

    if (noiseUScrollSpeed)
        dstLayer->noiseUScrollSpeed.Set(noiseUScrollSpeed->Clone());
    if (noiseUScrollSpeedVariation)
        dstLayer->noiseUScrollSpeedVariation.Set(noiseUScrollSpeedVariation->Clone());
    if (noiseUScrollSpeedOverLife)
        dstLayer->noiseUScrollSpeedOverLife.Set(noiseUScrollSpeedOverLife->Clone());

    if (noiseVScrollSpeed)
        dstLayer->noiseVScrollSpeed.Set(noiseVScrollSpeed->Clone());
    if (noiseVScrollSpeedVariation)
        dstLayer->noiseVScrollSpeedVariation.Set(noiseVScrollSpeedVariation->Clone());
    if (noiseVScrollSpeedOverLife)
        dstLayer->noiseVScrollSpeedOverLife.Set(noiseVScrollSpeedOverLife->Clone());

    if (life)
        dstLayer->life.Set(life->Clone());

    if (lifeVariation)
        dstLayer->lifeVariation.Set(lifeVariation->Clone());

    if (number)
        dstLayer->number.Set(number->Clone());

    if (numberVariation)
        dstLayer->numberVariation.Set(numberVariation->Clone());

    if (size)
        dstLayer->size.Set(size->Clone());

    if (sizeVariation)
        dstLayer->sizeVariation.Set(sizeVariation->Clone());

    if (sizeOverLifeXY)
        dstLayer->sizeOverLifeXY.Set(sizeOverLifeXY->Clone());

    if (velocity)
        dstLayer->velocity.Set(velocity->Clone());

    if (velocityVariation)
        dstLayer->velocityVariation.Set(velocityVariation->Clone());

    if (velocityOverLife)
        dstLayer->velocityOverLife.Set(velocityOverLife->Clone());

    // Copy the forces.
    dstLayer->CleanupSimplifiedForces();
    dstLayer->forcesSimplified.reserve(forcesSimplified.size());
    for (size_t f = 0; f < forcesSimplified.size(); ++f)
    {
        ParticleForceSimplified* clonedForce = this->forcesSimplified[f]->Clone();
        dstLayer->AddSimplifiedForce(clonedForce);
        clonedForce->Release();
    }

    dstLayer->CleanupForces();
    dstLayer->particleForces.reserve(particleForces.size());
    for (size_t f = 0; f < particleForces.size(); ++f)
    {
        ParticleForce* clonedForce = particleForces[f]->Clone();
        dstLayer->AddForce(clonedForce);
        clonedForce->Release();
    }

    if (spin)
        dstLayer->spin.Set(spin->Clone());

    if (spinVariation)
        dstLayer->spinVariation.Set(spinVariation->Clone());

    if (spinOverLife)
        dstLayer->spinOverLife.Set(spinOverLife->Clone());
    dstLayer->randomSpinDirection = randomSpinDirection;

    if (animSpeedOverLife)
        dstLayer->animSpeedOverLife.Set(animSpeedOverLife->Clone());

    if (colorOverLife)
        dstLayer->colorOverLife.Set(colorOverLife->Clone());

    if (colorRandom)
        dstLayer->colorRandom.Set(colorRandom->Clone());

    if (gradientColorForWhite)
        dstLayer->gradientColorForWhite.Set(gradientColorForWhite->Clone());

    if (gradientColorForBlack)
        dstLayer->gradientColorForBlack.Set(gradientColorForBlack->Clone());

    if (gradientColorForMiddle)
        dstLayer->gradientColorForMiddle.Set(gradientColorForMiddle->Clone());

    if (alphaOverLife)
        dstLayer->alphaOverLife.Set(alphaOverLife->Clone());

    if (angle)
        dstLayer->angle.Set(angle->Clone());

    if (angleVariation)
        dstLayer->angleVariation.Set(angleVariation->Clone());

    if (gradientMiddlePointLine)
        dstLayer->gradientMiddlePointLine.Set(gradientMiddlePointLine->Clone());

    SafeRelease(dstLayer->innerEmitter);
    if (innerEmitter)
        dstLayer->innerEmitter = innerEmitter->Clone();
    dstLayer->innerEmitterPath = innerEmitterPath;

    dstLayer->layerName = layerName;

    dstLayer->enableFlow = enableFlow;
    dstLayer->enableFlowAnimation = enableFlowAnimation;

    dstLayer->enableNoise = enableNoise;
    dstLayer->enableNoiseScroll = enableNoiseScroll;

    dstLayer->blending = blending;
    dstLayer->enableFog = enableFog;
    dstLayer->enableFrameBlend = enableFrameBlend;
    dstLayer->inheritPosition = inheritPosition;
    dstLayer->stripeInheritPositionOnlyForBaseVertex = stripeInheritPositionOnlyForBaseVertex;
    dstLayer->usePerspectiveMapping = usePerspectiveMapping;
    dstLayer->useThreePointGradient = useThreePointGradient;
    dstLayer->startTime = startTime;
    dstLayer->endTime = endTime;

    dstLayer->isLooped = isLooped;
    dstLayer->deltaTime = deltaTime;
    dstLayer->deltaVariation = deltaVariation;
    dstLayer->loopVariation = loopVariation;
    dstLayer->loopEndTime = loopEndTime;

    dstLayer->isDisabled = isDisabled;

    dstLayer->type = type;
    dstLayer->degradeStrategy = degradeStrategy;
    dstLayer->sprite = sprite;
    dstLayer->flowmap = flowmap;
    dstLayer->noise = noise;
    dstLayer->alphaRemapSprite = alphaRemapSprite;
    dstLayer->layerPivotPoint = layerPivotPoint;
    dstLayer->layerPivotSizeOffsets = layerPivotSizeOffsets;

    dstLayer->frameOverLifeEnabled = frameOverLifeEnabled;
    dstLayer->frameOverLifeFPS = frameOverLifeFPS;
    dstLayer->randomFrameOnStart = randomFrameOnStart;
    dstLayer->loopSpriteAnimation = loopSpriteAnimation;
    dstLayer->particleOrientation = particleOrientation;

    dstLayer->scaleVelocityBase = scaleVelocityBase;
    dstLayer->scaleVelocityFactor = scaleVelocityFactor;

    dstLayer->spritePath = spritePath;
    dstLayer->flowmapPath = flowmapPath;
    dstLayer->activeLODS = activeLODS;
    dstLayer->isLong = isLong;
    dstLayer->useFresnelToAlpha = useFresnelToAlpha;
    dstLayer->fresnelToAlphaBias = fresnelToAlphaBias;
    dstLayer->fresnelToAlphaPower = fresnelToAlphaPower;
    dstLayer->noisePath = noisePath;
    dstLayer->enableNoise = enableNoise;
    dstLayer->enableNoiseScroll = enableNoiseScroll;

    dstLayer->gradientMiddlePoint = gradientMiddlePoint;

    dstLayer->applyGlobalForces = applyGlobalForces;

    dstLayer->alphaRemapPath = alphaRemapPath;
    if (alphaRemapOverLife)
        dstLayer->alphaRemapOverLife.Set(alphaRemapOverLife->Clone());
    dstLayer->enableAlphaRemap = enableAlphaRemap;
    dstLayer->alphaRemapLoopCount = alphaRemapLoopCount;

    return dstLayer;
}

bool ParticleLayer::IsLodActive(int32 lod)
{
    if ((lod >= 0) && (lod < static_cast<int32>(activeLODS.size())))
        return activeLODS[lod];

    return false;
}

void ParticleLayer::SetLodActive(int32 lod, bool active)
{
    if ((lod >= 0) && (lod < static_cast<int32>(activeLODS.size())))
        activeLODS[lod] = active;
}

template <class T>
void UpdatePropertyLineKeys(PropertyLine<T>* line, float32 startTime, float32 translateTime, float32 endTime)
{
    if (!line)
        return;
    Vector<typename PropertyLine<T>::PropertyKey>& keys = line->GetValues();
    int32 size = static_cast<int32>(keys.size());
    int32 i;
    for (i = 0; i < size; ++i)
    {
        keys[i].t += translateTime;
        if (keys[i].t > endTime)
            break;
    }
    if (i == 0)
        i += 1; //keep at least 1
    keys.erase(keys.begin() + i, keys.end());
    if (keys.size() == 1)
    {
        keys[0].t = startTime;
    }
}

template <class T>
void UpdatePropertyLineOnLoad(PropertyLine<T>* line, float32 startTime, float32 endTime)
{
    if (!line)
        return;
    Vector<typename PropertyLine<T>::PropertyKey>& keys = line->GetValues();
    int32 size = static_cast<int32>(keys.size());
    int32 i;
    /*drop keys before*/
    for (i = 0; i < size; ++i)
    {
        if (keys[i].t >= startTime)
            break;
    }
    if (i != 0)
    {
        T v0 = line->GetValue(startTime);
        keys.erase(keys.begin(), keys.begin() + i);
        typename PropertyLine<T>::PropertyKey key;
        key.t = startTime;
        key.value = v0;
        keys.insert(keys.begin(), key);
    }

    /*drop keys after*/
    size = static_cast<int32>(keys.size());
    for (i = 0; i < size; i++)
    {
        if (keys[i].t > endTime)
            break;
    }
    if (i != size)
    {
        T v1 = line->GetValue(endTime);
        keys.erase(keys.begin() + i, keys.end());
        typename PropertyLine<T>::PropertyKey key;
        key.t = endTime;
        key.value = v1;
        keys.push_back(key);
    }
}

void ParticleLayer::UpdateLayerTime(float32 startTime, float32 endTime)
{
    float32 translateTime = startTime - this->startTime;
    this->startTime = startTime;
    this->endTime = endTime;
    /*validate all time depended property lines*/
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(life).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(lifeVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(number).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(numberVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(size).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(sizeVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(velocity).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(velocityVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(spin).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(spinVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(angle).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(angleVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(flowSpeed).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(flowSpeedVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(flowOffset).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(flowOffsetVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(noiseScale).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(noiseScaleVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(noiseUScrollSpeed).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(noiseUScrollSpeedVariation).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(noiseVScrollSpeed).Get(), startTime, translateTime, endTime);
    UpdatePropertyLineKeys(PropertyLineHelper::GetValueLine(noiseVScrollSpeedVariation).Get(), startTime, translateTime, endTime);
}

void ParticleLayer::SetSprite(const FilePath& path)
{
    spritePath = path;
    if (type != TYPE_SUPEREMITTER_PARTICLES)
        sprite.reset(Sprite::Create(spritePath));
}

void ParticleLayer::SetPivotPoint(Vector2 pivot)
{
    layerPivotPoint = pivot;
    layerPivotSizeOffsets = Vector2(1 + std::abs(layerPivotPoint.x), 1 + std::abs(layerPivotPoint.y));
    layerPivotSizeOffsets *= 0.5f;
}

void ParticleLayer::SetFlowmap(const FilePath& spritePath_)
{
    flowmapPath = spritePath_;
    if (type != TYPE_SUPEREMITTER_PARTICLES)
        flowmap.reset(Sprite::Create(flowmapPath));
}

void ParticleLayer::SetNoise(const FilePath& spritePath_)
{
    noisePath = spritePath_;
    if (type != TYPE_SUPEREMITTER_PARTICLES)
        noise.reset(Sprite::Create(noisePath));
}

void ParticleLayer::SetAlphaRemap(const FilePath& spritePath_)
{
    alphaRemapPath = spritePath_;
    if (type != TYPE_SUPEREMITTER_PARTICLES)
        alphaRemapSprite.reset(Sprite::Create(alphaRemapPath));
}

void ParticleLayer::LoadFromYaml(const FilePath& configPath, const YamlNode* node, bool preserveInheritPosition)
{
    stripeSizeOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("stripeSizeOverLifeProp"));
    stripeTextureTileOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("stripeTextureTileOverLife"));
    stripeColorOverLife = PropertyLineYamlReader::CreatePropertyLine<Color>(node->Get("stripeColorOverLife"));
    stripeNoiseUScrollSpeedOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("stripeNoiseUScrollSpeedOverLife"));
    stripeNoiseVScrollSpeedOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("stripeNoiseVScrollSpeedOverLife"));

    stripeLifetime = 0.0f;
    const YamlNode* stripeLifetimeNode = node->Get("stripeLifetime");
    if (stripeLifetimeNode)
    {
        stripeLifetime = stripeLifetimeNode->AsFloat();
    }
    stripeVertexSpawnStep = 1.0f;
    const YamlNode* stripeVertexSpawnStepNode = node->Get("stripeVertexSpawnStep");
    if (stripeVertexSpawnStepNode)
    {
        stripeVertexSpawnStep = stripeVertexSpawnStepNode->AsFloat();
    }
    stripeStartSize = 0.0f;
    const YamlNode* stripeStartSizeNode = node->Get("stripeStartSize");
    if (stripeStartSizeNode)
    {
        stripeStartSize = stripeStartSizeNode->AsFloat();
    }
    stripeUScrollSpeed = 0.0f;
    const YamlNode* stripeUScrollSpeedNode = node->Get("stripeUScrollSpeed");
    if (stripeUScrollSpeedNode)
    {
        stripeUScrollSpeed = stripeUScrollSpeedNode->AsFloat();
    }
    stripeVScrollSpeed = 0.0f;
    const YamlNode* stripeVScrollSpeedNode = node->Get("stripeVScrollSpeed");
    if (stripeVScrollSpeedNode)
    {
        stripeVScrollSpeed = stripeVScrollSpeedNode->AsFloat();
    }

    stripeFadeDistanceFromTop = 0.0f;
    const YamlNode* stripeFadeDistanceFromTopNode = node->Get("stripeFadeDistanceFromTop");
    if (stripeFadeDistanceFromTopNode)
    {
        stripeFadeDistanceFromTop = stripeFadeDistanceFromTopNode->AsFloat();
    }

    // format processing
    int32 format = 0;
    const YamlNode* formatNode = node->Get("effectFormat");
    if (formatNode)
    {
        format = formatNode->AsInt32();
    }

    type = TYPE_PARTICLES;
    const YamlNode* typeNode = node->Get("layerType");
    if (typeNode)
    {
        type = StringToLayerType(typeNode->AsString(), TYPE_PARTICLES);
    }

    degradeStrategy = DEGRADE_KEEP;
    const YamlNode* degradeNode = node->Get("degradeStrategy");
    if (degradeNode)
    {
        degradeStrategy = static_cast<eDegradeStrategy>(degradeNode->AsInt());
    }

    const YamlNode* nameNode = node->Get("name");
    if (nameNode)
    {
        layerName = nameNode->AsString();
    }

    const YamlNode* longNode = node->Get("isLong");
    if (longNode)
    {
        isLong = longNode->AsBool();
    }

    const YamlNode* useFresToAlphaNode = node->Get("useFresToAlpha");
    if (useFresToAlphaNode)
        useFresnelToAlpha = useFresToAlphaNode->AsBool();
    const YamlNode* fresToAlphaBiasNode = node->Get("fresToAlphaBias");
    if (fresToAlphaBiasNode)
        fresnelToAlphaBias = fresToAlphaBiasNode->AsFloat();
    const YamlNode* fresToAlphaPowerNode = node->Get("fresToAlphaPower");
    if (fresToAlphaPowerNode)
        fresnelToAlphaPower = fresToAlphaPowerNode->AsFloat();

    const YamlNode* pivotPointNode = node->Get("pivotPoint");

    const YamlNode* spriteNode = node->Get("sprite");
    if (spriteNode && !spriteNode->AsString().empty())
    {
        // Store the absolute path to sprite.
        FilePath spritePath = configPath.GetDirectory() + spriteNode->AsString();
        SetSprite(spritePath);
    }
    const YamlNode* flowmapNode = node->Get("flowmap");
    if (flowmapNode && !flowmapNode->AsString().empty())
    {
        FilePath flowPath = configPath.GetDirectory() + flowmapNode->AsString();
        SetFlowmap(flowPath);
    }
    const YamlNode* noiseNode = node->Get("noise");
    if (noiseNode && !noiseNode->AsString().empty())
    {
        FilePath noisePath = configPath.GetDirectory() + noiseNode->AsString();
        SetNoise(noisePath);
    }
    const YamlNode* alphaRemapNode = node->Get("alphaRemap");
    if (alphaRemapNode && !alphaRemapNode->AsString().empty())
    {
        FilePath alphaRemapPath = configPath.GetDirectory() + alphaRemapNode->AsString();
        SetAlphaRemap(alphaRemapPath);
    }

    if (pivotPointNode)
    {
        Vector2 _pivot = pivotPointNode->AsPoint();
        if ((format == 0) && sprite)
        {
            float32 ny = -_pivot.x / sprite->GetWidth() * 2;
            float32 nx = -_pivot.y / sprite->GetHeight() * 2;
            _pivot.Set(nx, ny);
        }

        SetPivotPoint(_pivot);
    }

    const YamlNode* lodsNode = node->Get("activeLODS");
    if (lodsNode)
    {
        const auto& vec = lodsNode->AsVector();
        for (size_t i = 0; i < vec.size(); ++i)
            SetLodActive(static_cast<int32>(i), (vec[i]->AsInt()) != 0); //as AddToArray has no override for bool, flags are stored as int
    }

    colorOverLife = PropertyLineYamlReader::CreatePropertyLine<Color>(node->Get("colorOverLife"));
    colorRandom = PropertyLineYamlReader::CreatePropertyLine<Color>(node->Get("colorRandom"));
    alphaOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("alphaOverLife"));

    gradientColorForWhite = PropertyLineYamlReader::CreatePropertyLine<Color>(node->Get("gradientColorForWhite"));
    gradientColorForBlack = PropertyLineYamlReader::CreatePropertyLine<Color>(node->Get("gradientColorForBlack"));
    gradientColorForMiddle = PropertyLineYamlReader::CreatePropertyLine<Color>(node->Get("gradientColorForMiddle"));

    const YamlNode* frameOverLifeEnabledNode = node->Get("frameOverLifeEnabled");
    if (frameOverLifeEnabledNode)
    {
        frameOverLifeEnabled = frameOverLifeEnabledNode->AsBool();
    }

    const YamlNode* randomFrameOnStartNode = node->Get("randomFrameOnStart");
    if (randomFrameOnStartNode)
    {
        randomFrameOnStart = randomFrameOnStartNode->AsBool();
    }
    const YamlNode* loopSpriteAnimationNode = node->Get("loopSpriteAnimation");
    if (loopSpriteAnimationNode)
    {
        loopSpriteAnimation = loopSpriteAnimationNode->AsBool();
    }

    const YamlNode* particleOrientationNode = node->Get("particleOrientation");
    if (particleOrientationNode)
    {
        particleOrientation = particleOrientationNode->AsInt32();
    }

    const YamlNode* frameOverLifeFPSNode = node->Get("frameOverLifeFPS");
    if (frameOverLifeFPSNode)
    {
        frameOverLifeFPS = frameOverLifeFPSNode->AsFloat();
    }

    const YamlNode* scaleVelocityBaseNode = node->Get("scaleVelocityBase");
    if (scaleVelocityBaseNode)
    {
        scaleVelocityBase = scaleVelocityBaseNode->AsFloat();
    }

    const YamlNode* scaleVelocityFactorNode = node->Get("scaleVelocityFactor");
    if (scaleVelocityFactorNode)
    {
        scaleVelocityFactor = scaleVelocityFactorNode->AsFloat();
    }

    alphaRemapLoopCount = 1.0f;
    const YamlNode* alphaRemapLoopCountNode = node->Get("alphaRemapLoopCount");
    if (alphaRemapLoopCountNode)
    {
        alphaRemapLoopCount = alphaRemapLoopCountNode->AsFloat();
    }

    gradientMiddlePoint = 0.5f;
    const YamlNode* gradientMiddlePointNode = node->Get("gradientMiddlePoint");
    if (gradientMiddlePointNode)
    {
        gradientMiddlePoint = gradientMiddlePointNode->AsFloat();
    }

    gradientMiddlePointLine = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("gradientMiddlePointLine"));

    alphaRemapOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("alphaRemapOverLife"));

    flowSpeed = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("flowSpeed"));
    flowSpeedVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("flowSpeedVariation"));
    flowOffset = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("flowOffset"));
    flowOffsetVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("flowOffsetVariation"));

    noiseScale = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseScale"));
    noiseScaleVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseScaleVariation"));
    noiseScaleOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseScaleOverLife"));
    noiseUScrollSpeed = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseUScrollSpeed"));
    noiseUScrollSpeedVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseUScrollSpeedVariation"));
    noiseUScrollSpeedOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseUScrollSpeedOverLife"));
    noiseVScrollSpeed = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseVScrollSpeed"));
    noiseVScrollSpeedVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseVScrollSpeedVariation"));
    noiseVScrollSpeedOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("noiseVScrollSpeedOverLife"));

    life = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("life"));
    lifeVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("lifeVariation"));

    number = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("number"));
    numberVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("numberVariation"));

    size = PropertyLineYamlReader::CreatePropertyLine<Vector2>(node->Get("size"));

    sizeVariation = PropertyLineYamlReader::CreatePropertyLine<Vector2>(node->Get("sizeVariation"));

    sizeOverLifeXY = PropertyLineYamlReader::CreatePropertyLine<Vector2>(node->Get("sizeOverLifeXY"));

    // Yuri Coder, 2013/04/03. sizeOverLife is outdated and kept here for the backward compatibility only.
    // New property is sizeOverlifeXY and contains both X and Y components.
    RefPtr<PropertyLine<float32>> sizeOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("sizeOverLife"));
    if (sizeOverLife)
    {
        if (sizeOverLifeXY)
        {
            // Both properties can't be present in the same config.
            Logger::Error("Both sizeOverlife and sizeOverlifeXY are defined for Particle Layer %s, taking sizeOverlifeXY as default",
                          configPath.GetAbsolutePathname().c_str());
            DVASSERT(false);
        }
        else
        {
            // Only the outdated sizeOverlife is defined - create sizeOverlifeXY property based on outdated one.
            FillSizeOverlifeXY(sizeOverLife);
        }
    }

    velocity = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("velocity"));
    velocityVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("velocityVariation"));
    velocityOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("velocityOverLife"));

    angle = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("angle"));
    angleVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("angleVariation"));

    int32 forceCount = 0;
    const YamlNode* forceCountNode = node->Get("forceCount");
    if (forceCountNode)
        forceCount = forceCountNode->AsInt();

    for (int k = 0; k < forceCount; ++k)
    {
        // Any of the Force Parameters might be NULL, and this is acceptable.
        RefPtr<PropertyLine<Vector3>> force = PropertyLineYamlReader::CreatePropertyLine<Vector3>(node->Get(Format("force%d", k)));
        RefPtr<PropertyLine<float32>> forceOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get(Format("forceOverLife%d", k)));

        if (force && (!format)) //as no forceVariation anymore - add it directly to force
        {
            RefPtr<PropertyLine<Vector3>> forceVariation = PropertyLineYamlReader::CreatePropertyLine<Vector3>(node->Get(Format("forceVariation%d", k)));
            if (forceVariation)
            {
                Vector3 varriationToAdd = forceVariation->GetValue(0);
                Vector<typename PropertyLine<Vector3>::PropertyKey>& keys = force->GetValues();
                for (size_t i = 0, sz = keys.size(); i < sz; ++i)
                {
                    keys[i].value += varriationToAdd;
                }
            }
        }

        ParticleForceSimplified* particleForce = new ParticleForceSimplified(force, forceOverLife);
        AddSimplifiedForce(particleForce);
        particleForce->Release();
    }

    LoadForcesFromYaml(node);

    spin = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("spin"));
    spinVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("spinVariation"));
    spinOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("spinOverLife"));
    animSpeedOverLife = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get("animSpeedOverLife"));
    const YamlNode* randomSpinDirectionNode = node->Get("randomSpinDirection");
    if (randomSpinDirectionNode)
    {
        randomSpinDirection = randomSpinDirectionNode->AsBool();
    }

    blending = BLENDING_ALPHABLEND; //default

    //read blend node for backward compatibility with old effect files
    const YamlNode* blend = node->Get("blend");
    if (blend)
    {
        if (blend->AsString() == "alpha")
        {
            blending = BLENDING_ALPHABLEND;
        }
        if (blend->AsString() == "add")
        {
            blending = BLENDING_ALPHA_ADDITIVE;
        }
    }

    const YamlNode* blendSrcNode = node->Get("srcBlendFactor");
    const YamlNode* blendDestNode = node->Get("dstBlendFactor");

    if (blendSrcNode && blendDestNode)
    {
        eBlendMode srcBlendFactor = GetBlendModeByName(blendSrcNode->AsString());
        eBlendMode dstBlendFactor = GetBlendModeByName(blendDestNode->AsString());

        if ((srcBlendFactor == BLEND_ONE) && (dstBlendFactor == BLEND_ONE))
            blending = BLENDING_ADDITIVE;
        else if ((srcBlendFactor == BLEND_SRC_ALPHA) && (dstBlendFactor == BLEND_ONE))
            blending = BLENDING_ALPHA_ADDITIVE;
        else if ((srcBlendFactor == BLEND_ONE_MINUS_DST_COLOR) && (dstBlendFactor == BLEND_ONE))
            blending = BLENDING_SOFT_ADDITIVE;
        else if ((srcBlendFactor == BLEND_DST_COLOR) && (dstBlendFactor == BLEND_ZERO))
            blending = BLENDING_MULTIPLICATIVE;
        else if ((srcBlendFactor == BLEND_DST_COLOR) && (dstBlendFactor == BLEND_SRC_COLOR))
            blending = BLENDING_STRONG_MULTIPLICATIVE;
    }

    //end of legacy

    const YamlNode* blendingNode = node->Get("blending");
    if (blendingNode)
        blending = static_cast<eBlending>(blendingNode->AsInt());
    const YamlNode* fogNode = node->Get("enableFog");
    if (fogNode)
    {
        enableFog = fogNode->AsBool();
    }
    const YamlNode* enableFlowNode = node->Get("enableFlow");
    if (enableFlowNode)
    {
        enableFlow = enableFlowNode->AsBool();
    }
    const YamlNode* enableFlowAnimationNode = node->Get("enableFlowAnimation");
    if (enableFlowAnimationNode)
    {
        enableFlowAnimation = enableFlowAnimationNode->AsBool();
    }

    const YamlNode* enableNoiseNode = node->Get("enableNoise");
    if (enableNoiseNode)
    {
        enableNoise = enableNoiseNode->AsBool();
    }

    const YamlNode* useNoiseScrollNode = node->Get("useNoiseScroll");
    if (useNoiseScrollNode)
    {
        enableNoiseScroll = useNoiseScrollNode->AsBool();
    }

    const YamlNode* enableAlphaRemapNode = node->Get("enableAlphaRemap");
    if (enableAlphaRemapNode)
    {
        enableAlphaRemap = enableAlphaRemapNode->AsBool();
    }

    const YamlNode* frameBlendNode = node->Get("enableFrameBlend");
    if (frameBlendNode)
    {
        enableFrameBlend = frameBlendNode->AsBool();
    }

    startTime = 0.0f;
    endTime = 100000000.0f;
    const YamlNode* startTimeNode = node->Get("startTime");
    if (startTimeNode)
        startTime = startTimeNode->AsFloat();

    const YamlNode* endTimeNode = node->Get("endTime");
    if (endTimeNode)
        endTime = endTimeNode->AsFloat();

    isLooped = false;
    deltaTime = 0.0f;
    deltaVariation = 0.0f;
    loopVariation = 0.0f;

    const YamlNode* isLoopedNode = node->Get("isLooped");
    if (isLoopedNode)
        isLooped = isLoopedNode->AsBool();

    const YamlNode* deltaTimeNode = node->Get("deltaTime");
    if (deltaTimeNode)
        deltaTime = deltaTimeNode->AsFloat();

    const YamlNode* deltaVariationNode = node->Get("deltaVariation");
    if (deltaVariationNode)
        deltaVariation = deltaVariationNode->AsFloat();

    const YamlNode* loopVariationNode = node->Get("loopVariation");
    if (loopVariationNode)
        loopVariation = loopVariationNode->AsFloat();

    const YamlNode* loopEndTimeNode = node->Get("loopEndTime");
    if (loopEndTimeNode)
        loopEndTime = loopEndTimeNode->AsFloat();

    /*validate all time depended property lines*/
    UpdatePropertyLineOnLoad(flowSpeed.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(flowSpeedVariation.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(flowOffset.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(flowOffsetVariation.Get(), startTime, endTime);

    UpdatePropertyLineOnLoad(noiseScale.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(noiseScaleVariation.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(noiseUScrollSpeed.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(noiseUScrollSpeedVariation.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(noiseVScrollSpeed.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(noiseVScrollSpeedVariation.Get(), startTime, endTime);

    UpdatePropertyLineOnLoad(life.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(lifeVariation.Get(), startTime, endTime);

    UpdatePropertyLineOnLoad(number.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(numberVariation.Get(), startTime, endTime);

    UpdatePropertyLineOnLoad(size.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(sizeVariation.Get(), startTime, endTime);

    UpdatePropertyLineOnLoad(velocity.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(velocityVariation.Get(), startTime, endTime);

    UpdatePropertyLineOnLoad(spin.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(spinVariation.Get(), startTime, endTime);

    UpdatePropertyLineOnLoad(angle.Get(), startTime, endTime);
    UpdatePropertyLineOnLoad(angleVariation.Get(), startTime, endTime);

    const YamlNode* inheritPositionNode = node->Get("inheritPosition");
    if (inheritPositionNode)
    {
        inheritPosition = inheritPositionNode->AsBool();
    }
    const YamlNode* stripeInheritPositionForBaseNode = node->Get("stripeInheritPositionForBase");
    if (stripeInheritPositionForBaseNode)
        stripeInheritPositionOnlyForBaseVertex = stripeInheritPositionForBaseNode->AsBool();

    const YamlNode* usePerspectiveMappingNode = node->Get("usePerspectiveMapping");
    if (usePerspectiveMappingNode)
        usePerspectiveMapping = usePerspectiveMappingNode->AsBool();

    const YamlNode* useThreePointGradientNode = node->Get("useThreePointGradient");
    if (useThreePointGradientNode)
        useThreePointGradient = useThreePointGradientNode->AsBool();

    const YamlNode* applyGlobalForcesNode = node->Get("applyGlobalForces");
    if (applyGlobalForcesNode)
        applyGlobalForces = applyGlobalForcesNode->AsBool();

    // Load the Inner Emitter parameters.
    const YamlNode* innerEmitterPathNode = node->Get("innerEmitterPath");
    if ((type == TYPE_SUPEREMITTER_PARTICLES) && innerEmitterPathNode)
    {
        SafeRelease(innerEmitter);
        ScopedPtr<ParticleEmitter> emitter(new ParticleEmitter());
        // Since Inner Emitter path is stored as Relative, convert it to absolute when loading.
        String relativePath = innerEmitterPathNode->AsString();
        if (relativePath.empty())
        {
            Logger::Error("Failed to load inner emitter from empty config path");
        }
        else
        {
            innerEmitterPath = configPath.GetDirectory() + relativePath;
            if (innerEmitterPath == configPath) // prevent recursion
            {
                Logger::Error("Attempt to load inner emitter from super emitter's config will cause recursion");
            }
            else
            {
                emitter->LoadFromYaml(this->innerEmitterPath, true);
            }
        }
        innerEmitter = new ParticleEmitterInstance(nullptr, emitter.get());
    }
    if (format == 0) //update old stuff
    {
        UpdateSizeLine(size.Get(), true, !isLong);
        UpdateSizeLine(sizeVariation.Get(), true, !isLong);
        UpdateSizeLine(sizeOverLifeXY.Get(), false, !isLong);
        inheritPosition &= preserveInheritPosition;
    }
}

void ParticleLayer::UpdateSizeLine(PropertyLine<Vector2>* line, bool rescaleSize, bool swapXY)
{
    //conversion from old format
    if (!line)
        return;
    if ((!rescaleSize) && (!swapXY))
        return; //nothing to update

    Vector<typename PropertyLine<Vector2>::PropertyKey>& keys = PropertyLineHelper::GetValueLine(line)->GetValues();
    for (size_t i = 0, sz = keys.size(); i < sz; ++i)
    {
        if (rescaleSize)
        {
            keys[i].value.x *= 0.5f;
            keys[i].value.y *= 0.5f;
        }
        if (swapXY)
        {
            float x = keys[i].value.x;
            keys[i].value.x = keys[i].value.y;
            keys[i].value.y = x;
        }
    }
}

void ParticleLayer::SaveToYamlNode(const FilePath& configPath, YamlNode* parentNode, int32 layerIndex)
{
    YamlNode* layerNode = new YamlNode(YamlNode::TYPE_MAP);
    String layerNodeName = Format("layer%d", layerIndex);
    parentNode->AddNodeToMap(layerNodeName, layerNode);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "stripeSizeOverLifeProp", stripeSizeOverLife);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "stripeTextureTileOverLife", stripeTextureTileOverLife);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "stripeNoiseUScrollSpeedOverLife", stripeNoiseUScrollSpeedOverLife);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "stripeNoiseVScrollSpeedOverLife", stripeNoiseVScrollSpeedOverLife);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(layerNode, "stripeColorOverLife", stripeColorOverLife);

    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "stripeLifetime", stripeLifetime);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "stripeVertexSpawnStep", stripeVertexSpawnStep);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "stripeStartSize", stripeStartSize);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "stripeUScrollSpeed", stripeUScrollSpeed);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "stripeVScrollSpeed", stripeVScrollSpeed);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "stripeFadeDistanceFromTop", stripeFadeDistanceFromTop);

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, "name", layerName);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, "type", "layer");
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, "layerType",
                                                                 LayerTypeToString(type, "particles"));

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<int32>(layerNode, "degradeStrategy", static_cast<int32>(degradeStrategy));
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, "isLong", isLong);

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<Vector2>(layerNode, "pivotPoint", layerPivotPoint);

    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "useFresToAlpha", useFresnelToAlpha);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "fresToAlphaBias", fresnelToAlphaBias);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "fresToAlphaPower", fresnelToAlphaPower);

    PropertyLineYamlWriter::WritePropertyValueToYamlNode(layerNode, "alphaRemapLoopCount", alphaRemapLoopCount);

    SaveSpritePath(spritePath, configPath, layerNode, "sprite");
    SaveSpritePath(flowmapPath, configPath, layerNode, "flowmap");
    SaveSpritePath(noisePath, configPath, layerNode, "noise");
    SaveSpritePath(alphaRemapPath, configPath, layerNode, "alphaRemap");

    layerNode->Add("enableAlphaRemap", enableAlphaRemap);

    layerNode->Add("blending", blending);

    layerNode->Add("enableFlow", enableFlow);
    layerNode->Add("enableFlowAnimation", enableFlowAnimation);

    layerNode->Add("enableNoise", enableNoise);
    layerNode->Add("useNoiseScroll", enableNoiseScroll);

    layerNode->Add("enableFog", enableFog);
    layerNode->Add("enableFrameBlend", enableFrameBlend);

    layerNode->Add("scaleVelocityBase", scaleVelocityBase);
    layerNode->Add("scaleVelocityFactor", scaleVelocityFactor);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "gradientMiddlePointLine", gradientMiddlePointLine);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "gradientMiddlePoint", gradientMiddlePoint);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "alphaRemapOverLife", this->alphaRemapOverLife);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "flowSpeed", this->flowSpeed);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "flowSpeedVariation", this->flowSpeedVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "flowOffset", this->flowOffset);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "flowOffsetVariation", this->flowOffsetVariation);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseScale", this->noiseScale);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseScaleVariation", this->noiseScaleVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseScaleOverLife", this->noiseScaleOverLife);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseUScrollSpeed", this->noiseUScrollSpeed);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseUScrollSpeedVariation", this->noiseUScrollSpeedVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseUScrollSpeedOverLife", this->noiseUScrollSpeedOverLife);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseVScrollSpeed", this->noiseVScrollSpeed);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseVScrollSpeedVariation", this->noiseVScrollSpeedVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "noiseVScrollSpeedOverLife", this->noiseVScrollSpeedOverLife);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "life", this->life);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "lifeVariation", this->lifeVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "number", this->number);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "numberVariation", this->numberVariation);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector2>(layerNode, "size", this->size);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector2>(layerNode, "sizeVariation", this->sizeVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector2>(layerNode, "sizeOverLifeXY", this->sizeOverLifeXY);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "velocity", this->velocity);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "velocityVariation", this->velocityVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "velocityOverLife", this->velocityOverLife);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "spin", this->spin);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "spinVariation", this->spinVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "spinOverLife", this->spinOverLife);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "animSpeedOverLife", this->animSpeedOverLife);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, "randomSpinDirection", this->randomSpinDirection);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "angle", this->angle);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "angleVariation", this->angleVariation);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(layerNode, "colorRandom", this->colorRandom);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, "alphaOverLife", this->alphaOverLife);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(layerNode, "gradientColorForWhite", this->gradientColorForWhite);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(layerNode, "gradientColorForBlack", this->gradientColorForBlack);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(layerNode, "gradientColorForMiddle", this->gradientColorForMiddle);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(layerNode, "colorOverLife", this->colorOverLife);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, "frameOverLifeEnabled", this->frameOverLifeEnabled);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "frameOverLifeFPS", this->frameOverLifeFPS);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, "randomFrameOnStart", this->randomFrameOnStart);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, "loopSpriteAnimation", this->loopSpriteAnimation);

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "startTime", this->startTime);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "endTime", this->endTime);

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, "isLooped", this->isLooped);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "deltaTime", this->deltaTime);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "deltaVariation", this->deltaVariation);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "loopVariation", this->loopVariation);
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, "loopEndTime", this->loopEndTime);

    layerNode->Set("inheritPosition", inheritPosition);
    layerNode->Set("stripeInheritPositionForBase", stripeInheritPositionOnlyForBaseVertex);
    layerNode->Set("usePerspectiveMapping", usePerspectiveMapping);
    layerNode->Set("applyGlobalForces", applyGlobalForces);

    layerNode->Set("useThreePointGradient", useThreePointGradient);

    layerNode->Set("particleOrientation", particleOrientation);

    YamlNode* lodsNode = new YamlNode(YamlNode::TYPE_ARRAY);
    for (int32 i = 0; i < 4; i++)
        lodsNode->Add(static_cast<int32>(activeLODS[i])); //as for now AddValueToArray has no bool type - force it to int
    layerNode->SetNodeToMap("activeLODS", lodsNode);

    if ((type == TYPE_SUPEREMITTER_PARTICLES) && innerEmitter)
    {
        String innerRelativePath = innerEmitterPath.GetRelativePathname(configPath.GetDirectory());
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, "innerEmitterPath", innerRelativePath);
    }

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<int32>(layerNode, "effectFormat", 1);

    // Now write the forces.
    SaveSimplifiedForcesToYamlNode(layerNode);

    SaveForcesToYamlNode(layerNode);
}

void ParticleLayer::SaveSpritePath(FilePath& path, const FilePath& configPath, YamlNode* layerNode, std::string name)
{
    if (!path.IsEmpty())
    {
        path.TruncateExtension();
        String relativePath = path.GetRelativePathname(configPath.GetDirectory());
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, name, relativePath);
    }
}

void ParticleLayer::SaveSimplifiedForcesToYamlNode(YamlNode* layerNode)
{
    int32 forceCount = static_cast<int32>(this->forcesSimplified.size());
    if (forceCount == 0)
    {
        // No forces to write.
        return;
    }

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<int32>(layerNode, "forceCount", forceCount);
    for (int32 i = 0; i < forceCount; i++)
    {
        ParticleForceSimplified* currentForce = this->forcesSimplified[i];

        String forceDataName = Format("force%d", i);
        PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector3>(layerNode, forceDataName, currentForce->force);

        forceDataName = Format("forceOverLife%d", i);
        PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, forceDataName, currentForce->forceOverLife);
    }
}

void ParticleLayer::SaveForcesToYamlNode(YamlNode* layerNode)
{
    using namespace ParticleLayerDetail;
    int32 forceCount = static_cast<int32>(particleForces.size());
    if (forceCount == 0)
    {
        // No forces to write.
        return;
    }

    PropertyLineYamlWriter::WritePropertyValueToYamlNode<int32>(layerNode, "dragForceCount", forceCount);
    for (int32 i = 0; i < forceCount; i++)
    {
        ParticleForce* currentForce = particleForces[i];

        String forceDataName = Format("forceName%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, forceDataName, currentForce->forceName);

        forceDataName = Format("forceType%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, forceDataName, TypeToString(currentForce->type, "drag", forceTypesMap));

        forceDataName = Format("forceIsActive%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->isActive);

        forceDataName = Format("dragForcePosition%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<Vector3>(layerNode, forceDataName, currentForce->position);

        forceDataName = Format("dragForceRotation%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<Vector3>(layerNode, forceDataName, currentForce->rotation);

        forceDataName = Format("dragForceInfinityRange%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->isInfinityRange);

        forceDataName = Format("killParticles%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->killParticles);

        forceDataName = Format("normalAsReflectionVector%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->normalAsReflectionVector);

        forceDataName = Format("randomizeReflectionForce%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->randomizeReflectionForce);

        forceDataName = Format("worldAlign%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->worldAlign);

        forceDataName = Format("pointGravityUseRandomPointsOnSphere%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->pointGravityUseRandomPointsOnSphere);

        forceDataName = Format("isGlobal%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(layerNode, forceDataName, currentForce->isGlobal);

        forceDataName = Format("dragForcePower%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<Vector3>(layerNode, forceDataName, currentForce->forcePower);

        forceDataName = Format("dragForceBoxSize%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<Vector3>(layerNode, forceDataName, currentForce->GetBoxSize());

        forceDataName = Format("dragForceRadius%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->GetRadius());

        forceDataName = Format("dragForceShape%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, forceDataName, TypeToString(currentForce->GetShape(), "box", shapeMap));

        forceDataName = Format("dragForceTimingType%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<String>(layerNode, forceDataName, TypeToString(currentForce->timingType, "const", timingTypesMap));

        forceDataName = Format("dragForceLine%d", i);
        PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector3>(layerNode, forceDataName, currentForce->forcePowerLine);

        forceDataName = Format("forceDirection%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<Vector3>(layerNode, forceDataName, currentForce->direction);

        forceDataName = Format("forceWindFreq%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->windFrequency);

        forceDataName = Format("windTurbulenceFrequency%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->windTurbulenceFrequency);

        forceDataName = Format("forceWindTurb%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->windTurbulence);

        forceDataName = Format("pointGravityRadius%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->pointGravityRadius);

        forceDataName = Format("rndReflectionForceMin%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->rndReflectionForceMin);

        forceDataName = Format("rndReflectionForceMax%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->rndReflectionForceMax);

        forceDataName = Format("velocityThreshold%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->velocityThreshold);

        forceDataName = Format("planeScale%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->planeScale);

        forceDataName = Format("reflectionChaos%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->reflectionChaos);

        forceDataName = Format("backwardTurbulenceProbability%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<int32>(layerNode, forceDataName, static_cast<int32>(currentForce->backwardTurbulenceProbability));

        forceDataName = Format("reflectionPercent%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<int32>(layerNode, forceDataName, static_cast<int32>(currentForce->reflectionPercent));

        forceDataName = Format("forceWindBias%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->windBias);

        forceDataName = Format("turbulenceLine%d", i);
        PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(layerNode, forceDataName, currentForce->turbulenceLine);

        forceDataName = Format("startTime%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->startTime);

        forceDataName = Format("endTime%d", i);
        PropertyLineYamlWriter::WritePropertyValueToYamlNode<float32>(layerNode, forceDataName, currentForce->endTime);
    }
}

void ParticleLayer::GetModifableLines(List<ModifiablePropertyLineBase*>& modifiables)
{
    PropertyLineHelper::AddIfModifiable(stripeSizeOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(stripeTextureTileOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(stripeNoiseUScrollSpeedOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(stripeNoiseVScrollSpeedOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(stripeColorOverLife.Get(), modifiables);

    PropertyLineHelper::AddIfModifiable(alphaRemapOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(gradientMiddlePointLine.Get(), modifiables);

    PropertyLineHelper::AddIfModifiable(flowSpeed.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(flowSpeedVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(flowOffset.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(flowOffsetVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseScale.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseScaleVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseScaleOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseUScrollSpeed.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseUScrollSpeedVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseUScrollSpeedOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseVScrollSpeed.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseVScrollSpeedVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(noiseVScrollSpeedOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(life.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(lifeVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(number.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(numberVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(size.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(sizeVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(sizeOverLifeXY.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(velocity.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(velocityVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(velocityOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(spin.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(spinVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(spinOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(colorRandom.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(gradientColorForWhite.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(gradientColorForBlack.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(gradientColorForMiddle.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(alphaOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(colorOverLife.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(angle.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(angleVariation.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(animSpeedOverLife.Get(), modifiables);

    int32 simplifiedForceCount = static_cast<int32>(this->forcesSimplified.size());
    for (int32 i = 0; i < simplifiedForceCount; i++)
    {
        forcesSimplified[i]->GetModifableLines(modifiables);
    }

    size_t forcesCount = particleForces.size();
    for (size_t i = 0; i < forcesCount; ++i)
        particleForces[i]->GetModifableLines(modifiables);

    if ((type == TYPE_SUPEREMITTER_PARTICLES) && innerEmitter)
    {
        innerEmitter->GetEmitter()->GetModifableLines(modifiables);
    }
}

void ParticleLayer::AddSimplifiedForce(ParticleForceSimplified* force)
{
    SafeRetain(force);
    this->forcesSimplified.push_back(force);
}

void ParticleLayer::RemoveSimplifiedForce(ParticleForceSimplified* force)
{
    Vector<ParticleForceSimplified*>::iterator iter = std::find(this->forcesSimplified.begin(),
                                                                this->forcesSimplified.end(),
                                                                force);
    if (iter != this->forcesSimplified.end())
    {
        SafeRelease(*iter);
        this->forcesSimplified.erase(iter);
    }
}

void ParticleLayer::RemoveSimplifiedForce(int32 forceIndex)
{
    if (forceIndex <= static_cast<int32>(this->forcesSimplified.size()))
    {
        SafeRelease(this->forcesSimplified[forceIndex]);
        this->forcesSimplified.erase(this->forcesSimplified.begin() + forceIndex);
    }
}

void ParticleLayer::CleanupSimplifiedForces()
{
    for (Vector<ParticleForceSimplified*>::iterator iter = this->forcesSimplified.begin();
         iter != this->forcesSimplified.end(); iter++)
    {
        SafeRelease(*iter);
    }

    this->forcesSimplified.clear();
}

void ParticleLayer::AddForce(ParticleForce* force)
{
    if (force->CanAlterPosition())
        ++alterPositionForcesCount;
    if (force->type == ParticleForce::eType::PLANE_COLLISION)
        ++planeCollisionForcesCount;

    SafeRetain(force);
    particleForces.push_back(force);
    std::sort(particleForces.begin(), particleForces.end(), [](const ParticleForce* a, const ParticleForce* b)
              {
                  return static_cast<int32>(a->type) < static_cast<int32>(b->type);
              });
}

void ParticleLayer::RemoveForce(ParticleForce* force)
{
    auto iter = std::find(particleForces.begin(), particleForces.end(), force);
    if (iter != particleForces.end())
    {
        if (force->CanAlterPosition())
            --alterPositionForcesCount;
        if (force->type == ParticleForce::eType::PLANE_COLLISION)
            --planeCollisionForcesCount;
        SafeRelease(*iter);
        particleForces.erase(iter);
    }
}

void ParticleLayer::RemoveForce(int32 forceIndex)
{
    if (forceIndex <= static_cast<int32>(particleForces.size()))
    {
        if (particleForces[forceIndex]->CanAlterPosition())
            --alterPositionForcesCount;
        if (particleForces[forceIndex]->type == ParticleForce::eType::PLANE_COLLISION)
            --planeCollisionForcesCount;
        SafeRelease(particleForces[forceIndex]);
        particleForces.erase(particleForces.begin() + forceIndex);
    }
}

void ParticleLayer::CleanupForces()
{
    for (auto& force : particleForces)
    {
        SafeRelease(force);
    }

    particleForces.clear();
}

void ParticleLayer::FillSizeOverlifeXY(RefPtr<PropertyLine<float32>> sizeOverLife)
{
    Vector<PropValue<float32>> wrappedPropertyValues = PropLineWrapper<float32>(sizeOverLife).GetProps();
    if (wrappedPropertyValues.empty())
    {
        this->sizeOverLifeXY = nullptr;
        return;
    }
    else if (wrappedPropertyValues.size() == 1)
    {
        Vector2 singleValue(wrappedPropertyValues[0].v, wrappedPropertyValues[0].v);
        this->sizeOverLifeXY = RefPtr<PropertyLine<Vector2>>(new PropertyLineValue<Vector2>(singleValue));
        return;
    }

    RefPtr<PropertyLineKeyframes<Vector2>> sizeOverLifeXYKeyframes =
    RefPtr<PropertyLineKeyframes<Vector2>>(new PropertyLineKeyframes<Vector2>);
    size_t propsCount = wrappedPropertyValues.size();
    for (size_t i = 0; i < propsCount; i++)
    {
        Vector2 curValue(wrappedPropertyValues[i].v, wrappedPropertyValues[i].v);
        sizeOverLifeXYKeyframes->AddValue(wrappedPropertyValues[i].t, curValue);
    }

    this->sizeOverLifeXY = sizeOverLifeXYKeyframes;
}

ParticleLayer::eType ParticleLayer::StringToLayerType(const String& layerTypeName, eType defaultLayerType)
{
    int32 layerTypesCount = sizeof(layerTypeNamesInfoMap) / sizeof(*layerTypeNamesInfoMap);
    for (int32 i = 0; i < layerTypesCount; i++)
    {
        if (layerTypeNamesInfoMap[i].layerTypeName == layerTypeName)
        {
            return layerTypeNamesInfoMap[i].layerType;
        }
    }

    return defaultLayerType;
}

String ParticleLayer::LayerTypeToString(eType layerType, const String& defaultLayerTypeName)
{
    int32 layerTypesCount = sizeof(layerTypeNamesInfoMap) / sizeof(*layerTypeNamesInfoMap);
    for (int32 i = 0; i < layerTypesCount; i++)
    {
        if (layerTypeNamesInfoMap[i].layerType == layerType)
        {
            return layerTypeNamesInfoMap[i].layerTypeName;
        }
    }

    return defaultLayerTypeName;
}

void ParticleLayer::LoadForcesFromYaml(const YamlNode* node)
{
    using namespace ParticleLayerDetail;

    int32 forcesCount = 0;
    const YamlNode* forceCountNode = node->Get("dragForceCount");
    if (forceCountNode)
        forcesCount = forceCountNode->AsInt();

    for (int32 i = 0; i < forcesCount; ++i)
    {
        ParticleForce* force = new ParticleForce(this);

        String forceDataName = Format("forceName%d", i);
        const YamlNode* nameNode = node->Get(forceDataName);
        if (nameNode)
            force->forceName = nameNode->AsString();

        forceDataName = Format("forceType%d", i);
        const YamlNode* typeNode = node->Get(forceDataName);
        if (typeNode)
        {
            String type = typeNode->AsString();
            force->type = StringToType(type, ForceType::DRAG_FORCE, forceTypesMap);
        }

        forceDataName = Format("forceIsActive%d", i);
        const YamlNode* activeNode = node->Get(forceDataName);
        if (activeNode)
            force->isActive = activeNode->AsBool();

        forceDataName = Format("dragForcePosition%d", i);
        const YamlNode* positionNode = node->Get(forceDataName);
        if (positionNode)
            force->position = positionNode->AsVector3();

        forceDataName = Format("dragForceRotation%d", i);
        const YamlNode* rotationNode = node->Get(forceDataName);
        if (rotationNode)
            force->rotation = rotationNode->AsVector3();

        forceDataName = Format("dragForceInfinityRange%d", i);
        const YamlNode* rangeNode = node->Get(forceDataName);
        if (rangeNode)
            force->isInfinityRange = rangeNode->AsBool();

        forceDataName = Format("killParticles%d", i);
        const YamlNode* killParticlesNode = node->Get(forceDataName);
        if (killParticlesNode)
            force->killParticles = killParticlesNode->AsBool();

        forceDataName = Format("normalAsReflectionVector%d", i);
        const YamlNode* normalAsReflectionVectorNode = node->Get(forceDataName);
        if (normalAsReflectionVectorNode)
            force->normalAsReflectionVector = normalAsReflectionVectorNode->AsBool();

        forceDataName = Format("randomizeReflectionForce%d", i);
        const YamlNode* randomizeReflectionForceNode = node->Get(forceDataName);
        if (randomizeReflectionForceNode)
            force->randomizeReflectionForce = randomizeReflectionForceNode->AsBool();

        forceDataName = Format("worldAlign%d", i);
        const YamlNode* worldAlignForceNode = node->Get(forceDataName);
        if (worldAlignForceNode)
            force->worldAlign = worldAlignForceNode->AsBool();

        forceDataName = Format("pointGravityUseRandomPointsOnSphere%d", i);
        const YamlNode* pointGravityUseRandomPointsOnSphereNode = node->Get(forceDataName);
        if (pointGravityUseRandomPointsOnSphereNode)
            force->pointGravityUseRandomPointsOnSphere = pointGravityUseRandomPointsOnSphereNode->AsBool();

        forceDataName = Format("isGlobal%d", i);
        const YamlNode* isGlobalNode = node->Get(forceDataName);
        if (isGlobalNode)
            force->isGlobal = isGlobalNode->AsBool();

        forceDataName = Format("dragForcePower%d", i);
        const YamlNode* powerNode = node->Get(forceDataName);
        if (powerNode)
            force->forcePower = powerNode->AsVector3();

        forceDataName = Format("dragForceBoxSize%d", i);
        const YamlNode* sizeNode = node->Get(forceDataName);
        if (sizeNode)
            force->SetBoxSize(sizeNode->AsVector3());

        forceDataName = Format("dragForceRadius%d", i);
        const YamlNode* radiusNode = node->Get(forceDataName);
        if (radiusNode)
            force->SetRadius(radiusNode->AsFloat());

        forceDataName = Format("dragForceShape%d", i);
        const YamlNode* shapeNode = node->Get(forceDataName);
        if (shapeNode)
        {
            String shapeName = shapeNode->AsString();
            force->SetShape(StringToType(shapeName, ForceShape::BOX, shapeMap));
        }

        forceDataName = Format("dragForceTimingType%d", i);
        const YamlNode* timingNode = node->Get(forceDataName);
        if (timingNode)
        {
            String name = timingNode->AsString();
            force->timingType = StringToType(name, ForceTimingType::CONSTANT, timingTypesMap);
        }

        forceDataName = Format("forceDirection%d", i);
        const YamlNode* directionNode = node->Get(forceDataName);
        if (directionNode)
            force->direction = directionNode->AsVector3();

        forceDataName = Format("forceWindFreq%d", i);
        const YamlNode* windFreqNode = node->Get(forceDataName);
        if (windFreqNode)
            force->windFrequency = windFreqNode->AsFloat();

        forceDataName = Format("windTurbulenceFrequency%d", i);
        const YamlNode* windTurbFreqNode = node->Get(forceDataName);
        if (windTurbFreqNode)
            force->windTurbulenceFrequency = windTurbFreqNode->AsFloat();

        forceDataName = Format("forceWindTurb%d", i);
        const YamlNode* windTurbNode = node->Get(forceDataName);
        if (windTurbNode)
            force->windTurbulence = windTurbNode->AsFloat();

        forceDataName = Format("pointGravityRadius%d", i);
        const YamlNode* pointGravityRadiusNode = node->Get(forceDataName);
        if (pointGravityRadiusNode)
            force->pointGravityRadius = pointGravityRadiusNode->AsFloat();

        forceDataName = Format("rndReflectionForceMin%d", i);
        const YamlNode* rndReflectionForceMinNode = node->Get(forceDataName);
        if (rndReflectionForceMinNode)
            force->rndReflectionForceMin = rndReflectionForceMinNode->AsFloat();

        forceDataName = Format("rndReflectionForceMax%d", i);
        const YamlNode* rndReflectionForceMaxNode = node->Get(forceDataName);
        if (rndReflectionForceMaxNode)
            force->rndReflectionForceMax = rndReflectionForceMaxNode->AsFloat();

        forceDataName = Format("velocityThreshold%d", i);
        const YamlNode* velocityThresholdNode = node->Get(forceDataName);
        if (velocityThresholdNode)
            force->velocityThreshold = velocityThresholdNode->AsFloat();

        forceDataName = Format("startTime%d", i);
        const YamlNode* startTimeNode = node->Get(forceDataName);
        if (startTimeNode)
            force->startTime = startTimeNode->AsFloat();

        forceDataName = Format("endTime%d", i);
        const YamlNode* endTimeNode = node->Get(forceDataName);
        if (endTimeNode)
            force->endTime = endTimeNode->AsFloat();

        forceDataName = Format("planeScale%d", i);
        const YamlNode* planeScaleNode = node->Get(forceDataName);
        if (planeScaleNode)
            force->planeScale = planeScaleNode->AsFloat();

        forceDataName = Format("reflectionChaos%d", i);
        const YamlNode* reflectionChaosNode = node->Get(forceDataName);
        if (reflectionChaosNode)
            force->reflectionChaos = reflectionChaosNode->AsFloat();

        forceDataName = Format("backwardTurbulenceProbability%d", i);
        const YamlNode* backwardTurbulenceProbabilityNode = node->Get(forceDataName);
        if (backwardTurbulenceProbabilityNode)
            force->backwardTurbulenceProbability = backwardTurbulenceProbabilityNode->AsUInt32();

        forceDataName = Format("reflectionPercent%d", i);
        const YamlNode* reflectionPercentNode = node->Get(forceDataName);
        if (reflectionPercentNode)
            force->reflectionPercent = reflectionPercentNode->AsUInt32();

        forceDataName = Format("forceWindBias%d", i);
        const YamlNode* windBiasNode = node->Get(forceDataName);
        if (windBiasNode)
            force->windBias = windBiasNode->AsFloat();

        RefPtr<PropertyLine<Vector3>> forcePowerLine = PropertyLineYamlReader::CreatePropertyLine<Vector3>(node->Get(Format("dragForceLine%d", i)));
        force->forcePowerLine = forcePowerLine;

        RefPtr<PropertyLine<float32>> turbulenceLine = PropertyLineYamlReader::CreatePropertyLine<float32>(node->Get(Format("turbulenceLine%d", i)));
        force->turbulenceLine = turbulenceLine;

        AddForce(force);
        force->Release();
    }
}
};
