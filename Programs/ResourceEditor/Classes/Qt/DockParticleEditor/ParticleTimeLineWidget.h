#pragma once

#include "Classes/Qt/DockParticleEditor/ScrollZoomWidget.h"
#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include <REPlatform/Scene/SceneEditor2.h>

#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QWidget>

#define LEFT_INDENT 20
#define TOP_INDENT 14
#define BOTTOM_INDENT 24
#define RIGHT_INDENT 317
#define LINE_STEP 16
#define RECT_SIZE 3
#define LINE_WIDTH 3

#define PARTICLES_INFO_CONTROL_OFFSET 8

#define UPDATE_LAYERS_EXTRA_INFO_PERIOD 250 // in milliseconds

namespace DAVA
{
class FieldBinder;
class SelectableGroup;
}

class ParticlesExtraInfoColumn;
class ParticleTimeLineWidget : public ScrollZoomWidget
{
    Q_OBJECT
    friend class ParticlesExtraInfoColumn;

public:
    explicit ParticleTimeLineWidget(QWidget* parent = 0);
    ~ParticleTimeLineWidget();

    void Init(DAVA::float32 minTime, DAVA::float32 maxTime) override;

    struct LINE
    {
        DAVA::float32 startTime;
        DAVA::float32 endTime;
        DAVA::float32 deltaTime;
        DAVA::float32 loopEndTime;
        bool isLooped;
        bool hasLoopVariation;
        QColor color;
        QString legend;
        DAVA::ParticleLayer* layer;
    };
    typedef DAVA::Map<DAVA::uint32, LINE> LINE_MAP;

signals:
    void ChangeVisible(bool visible);

protected slots:
    // Scene Tree signals - values are changed for Particles.
    void OnParticleEmitterValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter);
    void OnParticleLayerValueChanged(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer);
    void OnParticleEffectStateChanged(DAVA::SceneEditor2* scene, DAVA::Entity* effect, bool isStarted);

    // Scene Tree signals - Particle Emitter is loaded.
    void OnParticleEmitterLoaded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter);

    // Scene Tree signals - structure changes.
    void OnParticleLayerAdded(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer);
    void OnParticleLayerRemoved(DAVA::SceneEditor2* scene, DAVA::ParticleEmitterInstance* emitter);

    void OnUpdate();

    void OnUpdateLayersExtraInfoNeeded();

protected:
    void OnParticleEffectSelected(DAVA::ParticleEffectComponent* effect);

    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

private:
    bool GetLineRect(DAVA::uint32 id, QRect& startPoint, QRect& endPoint) const;
    bool GetLoopedLineRect(DAVA::uint32 id, QRect& startPoint, QRect& endPoint, DAVA::float32 startTime, DAVA::float32 endTime) const;
    QRect GetGraphRect() const override;
    QPoint GetPoint(const QPoint&) const;

    void AddLayerLine(DAVA::uint32 layerLineID, DAVA::float32 minTime, DAVA::float32 maxTime,
                      const QColor& layerColor, DAVA::ParticleLayer* layer);
    void AddLine(DAVA::uint32 lineId, DAVA::float32 startTime, DAVA::float32 endTime, DAVA::float32 deltaTime,
                 DAVA::float32 loopEndTime, bool isLooped, bool hasLoopVariation, const QColor& color,
                 const QString& legend, DAVA::ParticleLayer* layer);

    void OnValueChanged(int lineId);
    void UpdateSizePolicy() override;

    void ShowLayersExtraInfoValues(bool isVisible);

    void NotifyLayersExtraInfoChanged();
    void UpdateLayersExtraInfoPosition();
    void UpdateLayersExtraInfoValues();
    void ResetLayersExtraInfoValues();

    // Handle situation when the Particle Emitter Node is selected (including
    // case when separate Layer node is selected.
    void HandleEmitterSelected(DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter, DAVA::ParticleLayer* layer);

    QRect GetSliderRect() const override;
    QRect GetIncreaseRect() const override;
    QRect GetScaleRect() const override;
    QRect GetDecreaseRect() const override;

private:
    void OnSelectionChanged(const DAVA::Any& selection);
    void ProcessSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup& selection);

    // Get the width/height for particle counter label.
    void GetParticlesCountWidthHeight(const LINE& line, DAVA::int32& width, DAVA::int32& height);

    // Cleanup all timelines and info.
    void CleanupTimelines();

    LINE_MAP lines;
    QPoint selectedPoint;
    QFont nameFont;

    DAVA::ParticleEffectComponent* selectedEffect = nullptr;
    DAVA::ParticleEmitterInstance* selectedEmitter = nullptr;

    DAVA::int32 selectedLine;
    DAVA::int32 selectedLineOrigin;
    DAVA::Entity* emitterNode = nullptr;
    DAVA::Entity* effectNode = nullptr;
    DAVA::ParticleLayer* selectedLayer = nullptr;

    QTimer updateTimer;

    // List of data columns.
    DAVA::List<ParticlesExtraInfoColumn*> infoColumns;

    // Scene currently selected.
    DAVA::SceneEditor2* activeScene = nullptr;

    class SetPointValueDlg : public QDialog
    {
        //Q_OBJECT

    public:
        explicit SetPointValueDlg(DAVA::float32 value, DAVA::float32 minValue, DAVA::float32 maxValue, QWidget* parent = 0);

        DAVA::float32 GetValue() const;

    private:
        EventFilterDoubleSpinBox* valueSpin = nullptr;
    };

    std::unique_ptr<DAVA::FieldBinder> selectionFieldBinder;
};
