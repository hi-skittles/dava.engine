#pragma once

#include "Classes/Qt/DockParticleEditor/GradientPickerWidget.h"
#include "Classes/Qt/DockParticleEditor/TimeLineWidget.h"
#include "Classes/Qt/DockParticleEditor/BaseParticleEditorContentWidget.h"
#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Commands/RECommandNotificationObject.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

class ParticleEmitterPropertiesWidget : public BaseParticleEditorContentWidget
{
    Q_OBJECT

public:
    explicit ParticleEmitterPropertiesWidget(QWidget* parent = nullptr);

    void Init(DAVA::SceneEditor2* scene, DAVA::ParticleEffectComponent* effect, DAVA::ParticleEmitterInstance* emitter,
              bool updateMinimize, bool needUpdateTimeLimits = true);
    void Update();

    bool eventFilter(QObject* o, QEvent* e) override;

    void StoreVisualState(DAVA::KeyedArchive* visualStateProps) override;
    void RestoreVisualState(DAVA::KeyedArchive* visualStateProps) override;

    // Accessors to timelines.
    TimeLineWidget* GetEmitterRadiusTimeline()
    {
        return emitterRadius;
    };
    TimeLineWidget* GetEmitterInnerRadiusTimeline()
    {
        return emitterInnerRadius;
    };
    TimeLineWidget* GetEmitterAngleTimeline()
    {
        return emitterAngle;
    };
    TimeLineWidget* GetEmitterSizeTimeline()
    {
        return emitterSize;
    };
    TimeLineWidget* GetEmissionVectorTimeline()
    {
        return emitterEmissionVector;
    };
    QCheckBox* GetGenerateOnSurfaceCheckBox()
    {
        return generateOnSurfaceCheckBox;
    }
    QLabel* GetShockwaveLabel()
    {
        return shockwaveLabel;
    }
    QComboBox* GetShockwaveBox()
    {
        return shockwaveBox;
    }

signals:
    void ValueChanged();

public slots:
    void OnValueChanged();
    void OnEmitterYamlPathChanged(const QString& newPath);
    void OnEmitterPositionChanged();
    void OnCommand(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification);

protected:
    void UpdateProperties();
    void UpdateTooltip();

private:
    QVBoxLayout* mainLayout = nullptr;
    QLineEdit* emitterNameLineEdit = nullptr;
    QLineEdit* originalEmitterYamlPath = nullptr;
    QLineEdit* emitterYamlPath = nullptr;
    QComboBox* emitterType = nullptr;
    QComboBox* shockwaveBox = nullptr;
    QLabel* shockwaveLabel = nullptr;
    EventFilterDoubleSpinBox* positionXSpinBox = nullptr;
    EventFilterDoubleSpinBox* positionYSpinBox = nullptr;
    EventFilterDoubleSpinBox* positionZSpinBox = nullptr;
    QCheckBox* shortEffectCheckBox = nullptr;
    QCheckBox* generateOnSurfaceCheckBox = nullptr;

    TimeLineWidget* emitterEmissionRange = nullptr;
    TimeLineWidget* emitterEmissionVector = nullptr;
    TimeLineWidget* emissionVelocityVector = nullptr;
    TimeLineWidget* emitterRadius = nullptr;
    TimeLineWidget* emitterInnerRadius = nullptr;
    TimeLineWidget* emitterSize = nullptr;
    TimeLineWidget* emitterAngle = nullptr;
    EventFilterDoubleSpinBox* emitterLife = nullptr;
    GradientPickerWidget* emitterColorWidget = nullptr;

    bool blockSignals = false;
    bool updateMinimize = false;
    bool needUpdateTimeLimits = false;

    void InitWidget(QWidget* widget, bool connectWidget = true);
};
