#ifndef __DAVAENGINE_UI_PARTICLES__
#define __DAVAENGINE_UI_PARTICLES__

#include "UI/UIControl.h"
#include "Reflection/Reflection.h"
#include "Math/Matrix4.h"

namespace DAVA
{
class ParticleEffectComponent;
class ParticleEffectSystem;
class Camera;

class UIParticles : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UIParticles, UIControl);

protected:
    virtual ~UIParticles();

public:
    UIParticles(const Rect& rect = Rect());

    void Update(float32 timeElapsed) override;
    void Draw(const UIGeometricData& geometricData) override;

    void OnActive() override;

    UIParticles* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    /*methods analogical to once in ParticleEffectComponent*/
    void Start();
    void Stop(bool isDeleteAllParticles = true);
    void Restart(bool isDeleteAllParticles = true);
    bool IsStopped() const;
    void Pause(bool isPaused = true);
    bool IsPaused() const;

    void SetEffectPath(const FilePath& path);
    const FilePath& GetEffectPath() const;
    void ReloadEffect();

    void SetAutostart(bool value);
    bool IsAutostart() const;

    // Start delay, in seconds.
    float32 GetStartDelay() const;
    void SetStartDelay(float32 value);

    //external
    void SetExtertnalValue(const String& name, float32 value);

    void SetInheritControlTransform(bool inherit);
    bool GetInheritControlTransform() const;

protected:
    void LoadEffect(const FilePath& path);
    void UnloadEffect();

    // Start the playback in case Autostart flag is set.
    void HandleAutostart();

    // Handle the delayed action if requested.
    void HandleDelayedAction(float32 timeElapsed);

    // Start/Restart methods which can be called either immediately of after start delay.
    void DoStart();
    void DoRestart();

    enum eDelayedActionType
    {
        actionNone = 0,
        actionStart,
        actionRestart
    };

private:
    FilePath effectPath;
    bool isAutostart = false;
    float32 startDelay = 0.f;

    ParticleEffectComponent* effect = nullptr;
    ParticleEffectSystem* system = nullptr;
    Matrix4 matrix;
    float32 updateTime = 0.f;

    eDelayedActionType delayedActionType = actionNone;
    float32 delayedActionTime = 0.f;
    bool delayedDeleteAllParticles = false;
    bool needHandleAutoStart = false;
    bool inheritControlTransform = true;

    static Camera* defaultCamera;
};
};

#endif