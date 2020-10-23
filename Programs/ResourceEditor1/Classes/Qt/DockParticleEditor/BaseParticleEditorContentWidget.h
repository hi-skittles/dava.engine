#ifndef __RESOURCEEDITORQT__BASEPARTICLEEDITORCONTENTWIDGET__
#define __RESOURCEEDITORQT__BASEPARTICLEEDITORCONTENTWIDGET__

#include "DAVAEngine.h"
#include <QChar>
#include <QWidget>
#include "Scene/SceneEditor2.h"

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

    Objects GetCurrentObjectsForScene(SceneEditor2* scene) const;

    SceneEditor2* GetActiveScene() const;
    DAVA::ParticleEffectComponent* GetEffect(SceneEditor2* scene);
    DAVA::ParticleEmitterInstance* GetEmitterInstance(SceneEditor2* scene);
    void SetObjectsForScene(SceneEditor2* scene, DAVA::ParticleEffectComponent* effect,
                            DAVA::ParticleEmitterInstance* instance);

protected:
    // "Degree mark" character needed for some widgets.
    static const QChar DEGREE_MARK_CHARACTER;

    // Conversion from/to playback speed to/from slider value.
    int ConvertFromPlaybackSpeedToSliderValue(DAVA::float32 playbackSpeed);
    float ConvertFromSliderValueToPlaybackSpeed(int sliderValue);

    void OnSceneActivated(SceneEditor2*);
    void OnSceneClosed(SceneEditor2*);

private:
    Objects emptyObjects;
    SceneEditor2* activeScene = nullptr;
    DAVA::Map<SceneEditor2*, Objects> objectsForScene;
};

#endif /* defined(__RESOURCEEDITORQT__BASEPARTICLEEDITORCONTENTWIDGET__) */
