#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Base/DynamicObjectCache.h"
#include "Render/2D/Sprite.h"

#include "FileSystem/YamlParser.h"
#include "Particles/Particle.h"
#include "Particles/ParticleForceSimplified.h"
#include "Particles/ParticlePropertyLine.h"
#include "FileSystem/FilePath.h"
#include <Reflection/Reflection.h>

namespace DAVA
{
class ParticleEmitterInstance;
class ParticleForce;

/**
    In most cases you'll not need to use this class directly 
    and should use ParticleEffect instead. 
    
    Few cases when you actually need ParticleLayers: 
    - You want to get information about layer lifeTime or layer sprite
    - You want to change something on the fly inside layer
 */
struct ParticleLayer : public BaseObject
{
    float32 stripeLifetime = 20.0f;
    float32 stripeVertexSpawnStep = 1.0f;
    float32 stripeStartSize = 1.0f;
    RefPtr<PropertyLine<float32>> stripeSizeOverLife;
    RefPtr<PropertyLine<float32>> stripeTextureTileOverLife;
    RefPtr<PropertyLine<float32>> stripeNoiseUScrollSpeedOverLife;
    RefPtr<PropertyLine<float32>> stripeNoiseVScrollSpeedOverLife;
    float32 stripeUScrollSpeed = 0.0f;
    float32 stripeVScrollSpeed = -0.01f;
    float32 stripeFadeDistanceFromTop = 0.0f;
    RefPtr<PropertyLine<Color>> stripeColorOverLife;

    float32 maxStripeOverLife = 0.0f;

    enum eType
    {
        TYPE_SINGLE_PARTICLE,
        TYPE_PARTICLES, // default for any particle layer loaded from yaml file
        TYPE_PARTICLE_STRIPE,
        TYPE_SUPEREMITTER_PARTICLES
    };

    enum eParticleOrientation
    {
        PARTICLE_ORIENTATION_CAMERA_FACING = 1 << 0, //default
        PARTICLE_ORIENTATION_X_FACING = 1 << 1,
        PARTICLE_ORIENTATION_Y_FACING = 1 << 2,
        PARTICLE_ORIENTATION_Z_FACING = 1 << 3,
        PARTICLE_ORIENTATION_WORLD_ALIGN = 1 << 4,
        PARTICLE_ORIENTATION_CAMERA_FACING_STRIPE_SPHERICAL = 1 << 5
    };

    enum eDegradeStrategy
    {
        DEGRADE_KEEP = 0,
        DEGRADE_CUT_PARTICLES = 1,
        DEGRADE_REMOVE = 2
    };

    ParticleLayer();
    virtual ~ParticleLayer();
    virtual ParticleLayer* Clone();

    void LoadFromYaml(const FilePath& configPath, const YamlNode* node, bool preserveInheritPosition);
    void SaveToYamlNode(const FilePath& configPath, YamlNode* parentNode, int32 layerIndex);

    void SaveSpritePath(FilePath& path, const FilePath& configPath, YamlNode* layerNode, std::string name);

    void SaveSimplifiedForcesToYamlNode(YamlNode* layerNode);
    void SaveForcesToYamlNode(YamlNode* layerNode);

    void AddSimplifiedForce(ParticleForceSimplified* force);
    void RemoveSimplifiedForce(ParticleForceSimplified* force);
    void RemoveSimplifiedForce(int32 forceIndex);
    void CleanupSimplifiedForces();

    float32 CalculateMaxStripeSizeOverLife();
    void AddForce(ParticleForce* force);
    void RemoveForce(ParticleForce* force);
    void RemoveForce(int32 forceIndex);
    void CleanupForces();

    void GetModifableLines(List<ModifiablePropertyLineBase*>& modifiables);

    // Convert from Layer Type to its name and vice versa.
    eType StringToLayerType(const String& layerTypeName, eType defaultLayerType);
    String LayerTypeToString(eType layerType, const String& defaultLayerTypeName);

    void UpdateLayerTime(float32 startTime, float32 endTime);

    bool IsLodActive(int32 lod);
    void SetLodActive(int32 lod, bool active);

    void SetSprite(const FilePath& spritePath);
    void SetFlowmap(const FilePath& spritePath_);
    void SetPivotPoint(Vector2 pivot);
    void SetNoise(const FilePath& spritePath_);
    void SetAlphaRemap(const FilePath& spritePath_);

    bool GetInheritPosition() const;
    void SetInheritPosition(bool inheritPosition_);

    bool GetInheritPositionForStripeBase() const;
    void SetInheritPositionForStripeBase(bool inheritPositionForBase_);

    // Sprites
    ScopedPtr<Sprite> sprite;
    ScopedPtr<Sprite> flowmap;
    ScopedPtr<Sprite> noise;
    ScopedPtr<Sprite> alphaRemapSprite;

    /*
     Properties of particle layer that describe particle system logic
     */
    RefPtr<PropertyLine<float32>> life; // in seconds
    RefPtr<PropertyLine<float32>> lifeVariation; // variation part of life that added to particle life during generation of the particle

    // Flow
    RefPtr<PropertyLine<float32>> flowSpeed;
    RefPtr<PropertyLine<float32>> flowSpeedVariation;

    RefPtr<PropertyLine<float32>> flowOffset;
    RefPtr<PropertyLine<float32>> flowOffsetVariation;

    // Noise
    RefPtr<PropertyLine<float32>> noiseScale;
    RefPtr<PropertyLine<float32>> noiseScaleVariation;
    RefPtr<PropertyLine<float32>> noiseScaleOverLife;

    RefPtr<PropertyLine<float32>> noiseUScrollSpeed;
    RefPtr<PropertyLine<float32>> noiseUScrollSpeedVariation;
    RefPtr<PropertyLine<float32>> noiseUScrollSpeedOverLife; // Noise texcoord u scrollSpeed;

    RefPtr<PropertyLine<float32>> noiseVScrollSpeed;
    RefPtr<PropertyLine<float32>> noiseVScrollSpeedVariation;
    RefPtr<PropertyLine<float32>> noiseVScrollSpeedOverLife; // Noise texcoord v scrollSpeed;

    // Alpha remap
    RefPtr<PropertyLine<float32>> alphaRemapOverLife;

    // Number
    RefPtr<PropertyLine<float32>> number; // number of particles per second
    RefPtr<PropertyLine<float32>> numberVariation; // variation part of number that added to particle count during generation of the particle

    RefPtr<PropertyLine<Vector2>> size; // size of particles in pixels
    RefPtr<PropertyLine<Vector2>> sizeVariation; // size variation in pixels
    RefPtr<PropertyLine<Vector2>> sizeOverLifeXY;

    RefPtr<PropertyLine<float32>> velocity; // velocity in pixels
    RefPtr<PropertyLine<float32>> velocityVariation;
    RefPtr<PropertyLine<float32>> velocityOverLife;

    RefPtr<PropertyLine<float32>> spin; // spin of angle / second
    RefPtr<PropertyLine<float32>> spinVariation;
    RefPtr<PropertyLine<float32>> spinOverLife;

    RefPtr<PropertyLine<Color>> colorRandom;
    RefPtr<PropertyLine<float32>> alphaOverLife;
    RefPtr<PropertyLine<Color>> colorOverLife;

    RefPtr<PropertyLine<Color>> gradientColorForWhite;
    RefPtr<PropertyLine<Color>> gradientColorForBlack;
    RefPtr<PropertyLine<Color>> gradientColorForMiddle;

    RefPtr<PropertyLine<float32>> angle; // sprite angle in degrees
    RefPtr<PropertyLine<float32>> angleVariation; // variations in degrees

    RefPtr<PropertyLine<float32>> animSpeedOverLife;
    RefPtr<PropertyLine<float32>> gradientMiddlePointLine;

    ParticleEmitterInstance* innerEmitter = nullptr;

    // Layer loop paremeters
    float32 deltaTime = 0.0f;
    float32 deltaVariation = 0.0f;
    float32 loopVariation = 0.0f;
    float32 loopEndTime = 0.0f;
    float32 startTime = 0.0f;
    float32 endTime = 100.0f;

    float32 frameOverLifeFPS = 0;

    float32 gradientMiddlePoint = 0.5f;

    //for long particles
    float32 scaleVelocityBase = 1.0f;
    float32 scaleVelocityFactor = 0.0f;

    float32 fresnelToAlphaBias = 0.0f;
    float32 fresnelToAlphaPower = 0.0f;

    float32 alphaRemapLoopCount = 1.0f;

    int32 particleOrientation = PARTICLE_ORIENTATION_CAMERA_FACING;
    eType type = TYPE_PARTICLES;
    eBlending blending = BLENDING_ALPHABLEND;
    eDegradeStrategy degradeStrategy = DEGRADE_KEEP;

    Vector2 layerPivotPoint;
    Vector2 layerPivotSizeOffsets; //precached for faster bbox computation

    FilePath spritePath;
    FilePath flowmapPath;
    FilePath noisePath;
    FilePath alphaRemapPath;
    FilePath innerEmitterPath;

    Vector<bool> activeLODS;

    String layerName;

    bool isDisabled = false;
    bool enableFog = true;
    bool randomSpinDirection = false;
    bool isLong = false;
    bool isLooped = false;

    bool loopSpriteAnimation = true;
    bool randomFrameOnStart = false;
    bool frameOverLifeEnabled = false;
    bool enableFrameBlend = false;

    bool useFresnelToAlpha = false;
    bool enableAlphaRemap = false;
    bool enableNoiseScroll = false;
    bool enableNoise = false;
    bool enableFlow = false;
    bool enableFlowAnimation = false;
    bool usePerspectiveMapping = false;
    bool isMaxStripeOverLifeDirty = true;

    bool useThreePointGradient = false;
    bool applyGlobalForces = false;

    const Vector<ParticleForce*>& GetParticleForces();
    const Vector<ParticleForceSimplified*>& GetSimplifiedParticleForces();
    int8 GetAlterPositionForcesCount() const;
    int8 GetPlaneCollisiontForcesCount() const;

private:
    struct LayerTypeNamesInfo
    {
        eType layerType;
        String layerTypeName;
    };
    int8 alterPositionForcesCount = 0;
    int8 planeCollisionForcesCount = 0;
    static const LayerTypeNamesInfo layerTypeNamesInfoMap[];

    void FillSizeOverlifeXY(RefPtr<PropertyLine<float32>> sizeOverLife);
    void UpdateSizeLine(PropertyLine<Vector2>* line, bool rescaleSize, bool swapXY); //conversion from old format

    void LoadForcesFromYaml(const YamlNode* node);

    bool stripeInheritPositionOnlyForBaseVertex = false; // For stripe particles. Move only base vertex when in stripe.
    bool inheritPosition = false; //for super emitter - if true the whole emitter would be moved, otherwise just emission point

    Vector<ParticleForce*> particleForces;
    Vector<ParticleForceSimplified*> forcesSimplified;

public:
    DAVA_VIRTUAL_REFLECTION(ParticleLayer, BaseObject);
};

inline float32 ParticleLayer::CalculateMaxStripeSizeOverLife()
{
    using Key = PropertyLine<float32>::PropertyKey;

    if (!isMaxStripeOverLifeDirty)
        return maxStripeOverLife;
    if (stripeSizeOverLife.Get() == nullptr)
        return 1.0f;

    isMaxStripeOverLifeDirty = false;
    const Vector<Key>& keys = stripeSizeOverLife->GetValues();
    auto max = std::max_element(keys.begin(), keys.end(),
                                [](const Key& a, const Key& b)
                                {
                                    return a.value < b.value;
                                }
                                );
    maxStripeOverLife = (*max).value;
    return maxStripeOverLife;
}

inline bool ParticleLayer::GetInheritPosition() const
{
    return inheritPosition;
}

inline void ParticleLayer::SetInheritPosition(bool inheritPosition_)
{
    if (inheritPosition_)
        stripeInheritPositionOnlyForBaseVertex = false;

    inheritPosition = inheritPosition_;
}

inline bool ParticleLayer::GetInheritPositionForStripeBase() const
{
    return stripeInheritPositionOnlyForBaseVertex;
}

inline void ParticleLayer::SetInheritPositionForStripeBase(bool inheritPositionOnlyForBaseVertex_)
{
    if (inheritPositionOnlyForBaseVertex_)
        inheritPosition = false;

    stripeInheritPositionOnlyForBaseVertex = inheritPositionOnlyForBaseVertex_;
}

inline const Vector<ParticleForce*>& ParticleLayer::GetParticleForces()
{
    return particleForces;
}

inline const Vector<ParticleForceSimplified*>& ParticleLayer::GetSimplifiedParticleForces()
{
    return forcesSimplified;
}

inline int8 ParticleLayer::GetAlterPositionForcesCount() const
{
    return alterPositionForcesCount;
}

inline int8 ParticleLayer::GetPlaneCollisiontForcesCount() const
{
    return planeCollisionForcesCount;
}
}
