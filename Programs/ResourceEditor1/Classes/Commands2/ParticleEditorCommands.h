#pragma once

#include "Commands2/Base/RECommand.h"
#include "Commands2/Base/CommandAction.h"

#include <DAVAEngine.h>
#include <Particles/ParticleForce.h>

class CommandAddParticleEmitter : public CommandAction
{
public:
    CommandAddParticleEmitter(DAVA::Entity* effect);
    void Redo() override;

    DAVA::Entity* GetEntity() const;

protected:
    DAVA::Entity* effectEntity = nullptr;
};

// Start/stop/restart Particle Effect.
class CommandStartStopParticleEffect : public CommandAction
{
public:
    CommandStartStopParticleEffect(DAVA::Entity* effect, bool isStart);

    DAVA::Entity* GetEntity() const;
    void Redo() override;

    bool GetStarted() const
    {
        return isStart;
    };

    bool IsClean() const override
    {
        return true;
    }

protected:
    DAVA::Entity* effectEntity;
    bool isStart = false;
};

class CommandRestartParticleEffect : public CommandAction
{
public:
    CommandRestartParticleEffect(DAVA::Entity* effect);

    DAVA::Entity* GetEntity() const;
    void Redo() override;

    bool IsClean() const override
    {
        return true;
    }

protected:
    DAVA::Entity* effectEntity = nullptr;
};

// Add new layer to Particle Emitter.
class CommandAddParticleEmitterLayer : public CommandAction
{
public:
    CommandAddParticleEmitterLayer(DAVA::ParticleEffectComponent* component, DAVA::ParticleEmitterInstance* emitter);
    ~CommandAddParticleEmitterLayer();

    void Redo() override;

    DAVA::ParticleLayer* GetCreatedLayer() const
    {
        return createdLayer;
    };
    DAVA::ParticleEmitterInstance* GetParentEmitter() const
    {
        return instance;
    }

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleEmitterInstance* instance = nullptr;
    DAVA::ParticleLayer* createdLayer = nullptr;
};

// Remove a layer from Particle Emitter.
class CommandRemoveParticleEmitterLayer : public RECommand
{
public:
    CommandRemoveParticleEmitterLayer(DAVA::ParticleEffectComponent* component, DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer);
    ~CommandRemoveParticleEmitterLayer();

    void Redo() override;
    void Undo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleEmitterInstance* instance = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
    DAVA::int32 selectedLayerIndex = -1;
};

class CommandRemoveParticleEmitter : public RECommand
{
public:
    CommandRemoveParticleEmitter(DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter);
    ~CommandRemoveParticleEmitter();

    void Redo() override;
    void Undo() override;

    DAVA::ParticleEffectComponent* GetEffect() const;
    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

protected:
    DAVA::ParticleEffectComponent* selectedEffect = nullptr;
    DAVA::ParticleEmitterInstance* instance = nullptr;
};

inline DAVA::ParticleEffectComponent* CommandRemoveParticleEmitter::GetEffect() const
{
    return selectedEffect;
}

// Clone a layer inside Particle Emitter.
class CommandCloneParticleEmitterLayer : public CommandAction
{
public:
    CommandCloneParticleEmitterLayer(DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

protected:
    DAVA::ParticleEmitterInstance* instance = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

// Add new force to Particle Emitter layer.
class CommandAddParticleEmitterSimplifiedForce : public CommandAction
{
public:
    CommandAddParticleEmitterSimplifiedForce(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

// Remove a force from Particle Emitter layer.
class CommandRemoveParticleEmitterSimplifiedForce : public RECommand
{
public:
    CommandRemoveParticleEmitterSimplifiedForce(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer, DAVA::ParticleForceSimplified* force);
    ~CommandRemoveParticleEmitterSimplifiedForce();
    void Redo() override;
    void Undo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

    DAVA::ParticleForceSimplified* GetForce() const
    {
        return selectedForce;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
    DAVA::ParticleForceSimplified* selectedForce = nullptr;
};

class CommandAddParticleDrag : public CommandAction
{
public:
    CommandAddParticleDrag(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

class CommandAddParticleVortex : public CommandAction
{
public:
    CommandAddParticleVortex(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

class CommandAddParticleGravity : public CommandAction
{
public:
    CommandAddParticleGravity(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

class CommandAddParticleWind : public CommandAction
{
public:
    CommandAddParticleWind(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

class CommandAddParticlePointGravity : public CommandAction
{
public:
    CommandAddParticlePointGravity(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

class CommandAddParticlePlaneCollision : public CommandAction
{
public:
    CommandAddParticlePlaneCollision(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
};

class CommandRemoveParticleForce : public RECommand
{
public:
    CommandRemoveParticleForce(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer, DAVA::ParticleForce* force);
    ~CommandRemoveParticleForce();

    void Redo() override;
    void Undo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

    DAVA::ParticleForce* GetForce() const
    {
        return selectedForce;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
    DAVA::ParticleForce* selectedForce = nullptr;
};

class CommandCloneParticleForce : public CommandAction
{
public:
    CommandCloneParticleForce(DAVA::ParticleEffectComponent* component, DAVA::ParticleLayer* layer, DAVA::ParticleForce* force);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffectComponent() const
    {
        return component;
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return selectedLayer;
    }

    DAVA::ParticleForce* GetForce() const
    {
        return selectedForce;
    }

protected:
    DAVA::ParticleEffectComponent* component = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;
    DAVA::ParticleForce* selectedForce = nullptr;
};

class CommandUpdateEffect : public CommandAction
{
public:
    CommandUpdateEffect(DAVA::ParticleEffectComponent* particleEffect);
    void Init(DAVA::float32 playbackSpeed);
    void Redo() override;

protected:
    DAVA::ParticleEffectComponent* particleEffect = nullptr;
    DAVA::float32 playbackSpeed = 1.0f;
};

class CommandUpdateEmitter : public CommandAction
{
public:
    CommandUpdateEmitter(DAVA::ParticleEmitterInstance* emitter);

    void Init(const DAVA::FastName& name,
              DAVA::ParticleEmitter::eType emitterType,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> emissionRange,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> emissionVector,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> emissionVelocityVector,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> radius,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> emissionAngle,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> emissionAngleVariation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> colorOverLife,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> size,
              DAVA::float32 life,
              bool isShortEffect);

    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

    void Redo() override;

protected:
    DAVA::FastName name;
    DAVA::ParticleEmitterInstance* instance = nullptr;

    DAVA::ParticleEmitter::eType emitterType;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> emissionRange;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> emissionAngle;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> emissionAngleVariation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> emissionVector;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> emissionVelocityVector;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> radius;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> colorOverLife;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> size;
    DAVA::float32 life = 0.0f;
    bool isShortEffect = false;
};

class CommandUpdateParticleLayerBase : public CommandAction
{
public:
    CommandUpdateParticleLayerBase(DAVA::uint32 cmdID)
        : CommandAction(cmdID)
    {
    }

    DAVA::ParticleLayer* GetLayer() const
    {
        return layer;
    };

protected:
    DAVA::ParticleLayer* layer;
};

class CommandUpdateParticleLayer : public CommandUpdateParticleLayerBase
{
public:
    CommandUpdateParticleLayer(DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer);
    void Init(const DAVA::String& layerName,
              DAVA::ParticleLayer::eType layerType,
              DAVA::ParticleLayer::eDegradeStrategy degradeStrategy,
              bool isDisabled,
              bool inheritPosition,
              bool isLong,
              DAVA::float32 scaleVelocityBase,
              DAVA::float32 scaleVelocityFactor,
              bool isLooped,
              DAVA::int32 particleOrientation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> life,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> lifeVariation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> number,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> numberVariation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector2>> size,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector2>> sizeVariation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector2>> sizeOverLife,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> velocity,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> velocityVariation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> velocityOverLife,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> spin,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> spinVariation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> spinOverLife,
              bool randomSpinDirection,

              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> colorRandom,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> alphaOverLife,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> colorOverLife,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> angle,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> angleVariation,

              DAVA::float32 startTime,
              DAVA::float32 endTime,
              DAVA::float32 deltaTime,
              DAVA::float32 deltaVariation,
              DAVA::float32 loopEndTime,
              DAVA::float32 loopVariation,
              bool frameOverLifeEnabled,
              DAVA::float32 frameOverLifeFPS,
              bool randomFrameOnStart,
              bool loopSpriteAnimation,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> animSpeedOverLife,

              DAVA::float32 pivotPointX,
              DAVA::float32 pivotPointY,
              bool applyGlobalForces);

    void Redo() override;

    DAVA::ParticleEmitterInstance* GetDeletedEmitter() const
    {
        return deletedEmitter.Get();
    }

    DAVA::ParticleEmitterInstance* GetCreatedEmitter() const
    {
        return createdEmitter.Get();
    }

protected:
    DAVA::ParticleEmitterInstance* emitter = nullptr;

    DAVA::String layerName;
    DAVA::ParticleLayer::eType layerType;
    DAVA::ParticleLayer::eDegradeStrategy degradeStrategy;
    bool isDisabled;
    bool isLong;
    DAVA::float32 scaleVelocityBase;
    DAVA::float32 scaleVelocityFactor;
    bool inheritPosition;
    bool isLooped;
    DAVA::int32 particleOrientation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> life;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> lifeVariation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> number;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> numberVariation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector2>> size;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector2>> sizeVariation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector2>> sizeOverLife;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> velocity;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> velocityVariation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> velocityOverLife;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> spin;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> spinVariation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> spinOverLife;
    bool randomSpinDirection;

    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> colorRandom;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> alphaOverLife;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Color>> colorOverLife;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> angle;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> angleVariation;

    DAVA::float32 startTime;
    DAVA::float32 endTime;
    DAVA::float32 deltaTime;
    DAVA::float32 deltaVariation;
    DAVA::float32 loopEndTime;
    DAVA::float32 loopVariation;
    bool frameOverLifeEnabled;
    DAVA::float32 frameOverLifeFPS;
    bool randomFrameOnStart;
    bool loopSpriteAnimation;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> animSpeedOverLife;

    DAVA::float32 pivotPointX;
    DAVA::float32 pivotPointY;

    DAVA::RefPtr<DAVA::ParticleEmitterInstance> deletedEmitter;
    DAVA::RefPtr<DAVA::ParticleEmitterInstance> createdEmitter;
    bool applyGlobalForces;
};

class CommandUpdateParticleLayerTime : public CommandUpdateParticleLayerBase
{
public:
    CommandUpdateParticleLayerTime(DAVA::ParticleLayer* layer);
    void Init(DAVA::float32 startTime, DAVA::float32 endTime);

    void Redo() override;

protected:
    DAVA::float32 startTime;
    DAVA::float32 endTime;
};

class CommandUpdateParticleLayerEnabled : public CommandUpdateParticleLayerBase
{
public:
    CommandUpdateParticleLayerEnabled(DAVA::ParticleLayer* layer, bool isEnabled);
    void Redo() override;

protected:
    bool isEnabled;
};

class CommandUpdateParticleLayerLods : public CommandUpdateParticleLayerBase
{
public:
    CommandUpdateParticleLayerLods(DAVA::ParticleLayer* layer, const DAVA::Vector<bool>& lods);
    void Redo() override;

protected:
    DAVA::Vector<bool> lods;
};

class CommandUpdateParticleSimplifiedForce : public CommandAction
{
public:
    CommandUpdateParticleSimplifiedForce(DAVA::ParticleLayer* layer, DAVA::uint32 forceId);

    void Init(DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> force,
              DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> forcesOverLife);

    void Redo() override;

    DAVA::ParticleLayer* GetLayer() const
    {
        return layer;
    };
    DAVA::uint32 GetForceIndex() const
    {
        return forceId;
    };

protected:
    DAVA::ParticleLayer* layer;
    DAVA::uint32 forceId;

    DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> force;
    DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> forcesOverLife;
};

class CommandUpdateParticleForce : public CommandAction
{
public:
    struct ForceParams
    {
        DAVA::String forceName;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::Vector3>> forcePowerLine;
        DAVA::RefPtr<DAVA::PropertyLine<DAVA::float32>> turbulenceLine;
        DAVA::ParticleForce::eShape shape = DAVA::ParticleForce::eShape::BOX;
        DAVA::ParticleForce::eTimingType timingType = DAVA::ParticleForce::eTimingType::CONSTANT;
        DAVA::float32 radius = 0.0f;
        DAVA::float32 windFrequency = 0.0f;
        DAVA::float32 windTurbulence = 0.0f;
        DAVA::float32 windTurbulenceFrequency = 0.0f;
        DAVA::float32 windBias = 1.0f;
        DAVA::uint32 backwardTurbulenceProbability = 0;
        DAVA::uint32 reflectionPercent = 0;
        DAVA::float32 pointGravityRadius = 1.0f;
        DAVA::float32 planeScale = 1.0f;
        DAVA::float32 reflectionChaos = 0.0f;
        DAVA::float32 rndReflectionForceMin = 1.0f;
        DAVA::float32 rndReflectionForceMax = 1.0f;
        DAVA::float32 velocityThreshold = 1.0f;
        DAVA::float32 startTime = 0.0f;
        DAVA::float32 endTime = 15.0f;
        DAVA::Vector3 boxSize;
        DAVA::Vector3 forcePower;
        DAVA::Vector3 direction;
        bool isActive = true;
        bool worldAlign = false;
        bool useInfinityRange = false;
        bool pointGravityUseRandomPointsOnSphere = false;
        bool isGlobal = false;
        bool killParticles = false;
        bool normalAsReflectionVector = true;
        bool randomizeReflectionForce = true;
    };

    CommandUpdateParticleForce(DAVA::ParticleLayer* layer_, DAVA::uint32 forceId_, ForceParams&& params);

    void Redo() override;
    void Undo() override;

    DAVA::ParticleLayer* GetLayer() const;
    DAVA::uint32 GetForceIndex() const;

protected:
    void ApplyParams(ForceParams& params);

    ForceParams newParams;
    ForceParams oldParams;
    DAVA::ParticleLayer* layer = nullptr;
    DAVA::uint32 forceId = -1;
};

inline DAVA::ParticleLayer* CommandUpdateParticleForce::GetLayer() const
{
    return layer;
}

inline DAVA::uint32 CommandUpdateParticleForce::GetForceIndex() const
{
    return forceId;
}

// Load/save Particle Emitter Node.
class CommandLoadParticleEmitterFromYaml : public CommandAction
{
public:
    CommandLoadParticleEmitterFromYaml(DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter, const DAVA::FilePath& path);

    void Redo() override;

    DAVA::ParticleEffectComponent* GetEffect() const
    {
        return selectedEffect;
    }

    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

protected:
    DAVA::ParticleEffectComponent* selectedEffect = nullptr;
    DAVA::ParticleEmitterInstance* instance = nullptr;
    DAVA::FilePath filePath;
};

class CommandSaveParticleEmitterToYaml : public CommandAction
{
public:
    CommandSaveParticleEmitterToYaml(DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter, const DAVA::FilePath& path);

    void Redo() override;

    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

protected:
    DAVA::ParticleEffectComponent* selectedEffect = nullptr;
    DAVA::ParticleEmitterInstance* instance = nullptr;
    DAVA::FilePath filePath;
};

// Load/save Particle Inner Emitter Node.
class CommandLoadInnerParticleEmitterFromYaml : public CommandAction
{
public:
    CommandLoadInnerParticleEmitterFromYaml(DAVA::ParticleEmitterInstance* emitter, const DAVA::FilePath& path);

    void Redo() override;

    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

protected:
    DAVA::ParticleEmitterInstance* instance = nullptr;
    DAVA::FilePath filePath;
};

class CommandSaveInnerParticleEmitterToYaml : public CommandAction
{
public:
    CommandSaveInnerParticleEmitterToYaml(DAVA::ParticleEmitterInstance* emitter, const DAVA::FilePath& path);
    void Redo() override;

    DAVA::ParticleEmitterInstance* GetEmitterInstance() const
    {
        return instance;
    }

protected:
    DAVA::ParticleEmitterInstance* instance = nullptr;
    DAVA::FilePath filePath;
};

class CommandReloadEmitters : public CommandAction
{
public:
    CommandReloadEmitters(DAVA::ParticleEffectComponent* component_);
    void Redo() override;

    DAVA::ParticleEffectComponent* GetComponent() const;

    bool IsClean() const override
    {
        return true;
    }

protected:
    DAVA::ParticleEffectComponent* component;
};
