#include "Classes/Qt/DockParticleEditor/BaseParticleEditorContentWidget.h"
#include "Classes/Qt/Scene/SceneSignals.h"

const QChar BaseParticleEditorContentWidget::DEGREE_MARK_CHARACTER = QChar(0x00B0);
#define PARTICLE_EMITTER_MIN_PLAYBACK_SPEED 0.25f
#define PARTICLE_EMITTER_MAX_PLAYBACK_SPEED 4.0f

BaseParticleEditorContentWidget::BaseParticleEditorContentWidget(QWidget* parent)
    : QWidget(parent)
{
    SceneSignals* dispatcher = SceneSignals::Instance();
    connect(dispatcher, &SceneSignals::Closed, this, &BaseParticleEditorContentWidget::OnSceneClosed);
    connect(dispatcher, &SceneSignals::Activated, this, &BaseParticleEditorContentWidget::OnSceneActivated);
}

void BaseParticleEditorContentWidget::OnSceneActivated(DAVA::SceneEditor2* editor)
{
    activeScene = editor;
}

void BaseParticleEditorContentWidget::OnSceneClosed(DAVA::SceneEditor2* editor)
{
    if (editor == activeScene)
        activeScene = nullptr;

    objectsForScene.erase(editor);
}

int BaseParticleEditorContentWidget::ConvertFromPlaybackSpeedToSliderValue(DAVA::float32 playbackSpeed)
{
    playbackSpeed = DAVA::Clamp(playbackSpeed, PARTICLE_EMITTER_MIN_PLAYBACK_SPEED, PARTICLE_EMITTER_MAX_PLAYBACK_SPEED);

    // Our scale is logarithmic.
    uint playbackSpeedInt = (uint)(playbackSpeed / PARTICLE_EMITTER_MIN_PLAYBACK_SPEED);
    uint logValue = -1;
    while (playbackSpeedInt)
    {
        logValue++;
        playbackSpeedInt >>= 1;
    }

    return logValue;
}

float BaseParticleEditorContentWidget::ConvertFromSliderValueToPlaybackSpeed(int sliderValue)
{
    // Our scale is logarithmic.
    uint scaleFactor = (1 << sliderValue);
    return PARTICLE_EMITTER_MIN_PLAYBACK_SPEED * scaleFactor;
}

BaseParticleEditorContentWidget::Objects BaseParticleEditorContentWidget::GetCurrentObjectsForScene(DAVA::SceneEditor2* scene) const
{
    auto i = objectsForScene.find(scene);
    return (i == objectsForScene.end()) ? emptyObjects : i->second;
}

void BaseParticleEditorContentWidget::SetObjectsForScene(DAVA::SceneEditor2* s, DAVA::ParticleEffectComponent* e, DAVA::ParticleEmitterInstance* i)
{
    objectsForScene[s].effect = e;
    objectsForScene[s].instance = i;
}

DAVA::SceneEditor2* BaseParticleEditorContentWidget::GetActiveScene() const
{
    return activeScene;
}

DAVA::ParticleEffectComponent* BaseParticleEditorContentWidget::GetEffect(DAVA::SceneEditor2* scene)
{
    return objectsForScene[scene].effect;
}

DAVA::ParticleEmitterInstance* BaseParticleEditorContentWidget::GetEmitterInstance(DAVA::SceneEditor2* scene)
{
    return objectsForScene[scene].instance.Get();
}
