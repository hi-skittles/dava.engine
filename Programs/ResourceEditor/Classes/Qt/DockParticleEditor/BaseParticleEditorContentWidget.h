#pragma once

#include <REPlatform/Scene/SceneEditor2.h>

#include <Base/RefPtr.h>
#include <FileSystem/KeyedArchive.h>
#include <Particles/ParticleEmitterInstance.h>
#include <Scene3D/Components/ParticleEffectComponent.h>

#include <QChar>
#include <QWidget>

class BaseParticleEditorContentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseParticleEditorContentWidget(QWidget* parent);
    virtual ~BaseParticleEditorContentWidget() = default;

    virtual void StoreVisualState(DAVA::KeyedArchive* visualStateProps) = 0;
    virtual void RestoreVisualState(DAVA::KeyedArchive* visualStateProps) = 0;

    struct Objects
    {
        DAVA::ParticleEffectComponent* effect = nullptr;
        DAVA::RefPtr<DAVA::ParticleEmitterInstance> instance;
    };

    Objects GetCurrentObjectsForScene(DAVA::SceneEditor2* scene) const;

    DAVA::SceneEditor2* GetActiveScene() const;
    DAVA::ParticleEffectComponent* GetEffect(DAVA::SceneEditor2* scene);
    DAVA::ParticleEmitterInstance* GetEmitterInstance(DAVA::SceneEditor2* scene);
    void SetObjectsForScene(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect,
                            DAVA::ParticleEmitterInstance* instance);

    void OnSceneActivated(DAVA::SceneEditor2* editor);
    void OnSceneClosed(DAVA::SceneEditor2* editor);

protected:
    // "Degree mark" character needed for some widgets.
    static const QChar DEGREE_MARK_CHARACTER;

    // Conversion from/to playback speed to/from slider value.
    int ConvertFromPlaybackSpeedToSliderValue(DAVA::float32 playbackSpeed);
    float ConvertFromSliderValueToPlaybackSpeed(int sliderValue);

private:
    Objects emptyObjects;
    DAVA::SceneEditor2* activeScene = nullptr;
    DAVA::Map<DAVA::SceneEditor2*, Objects> objectsForScene;
};
