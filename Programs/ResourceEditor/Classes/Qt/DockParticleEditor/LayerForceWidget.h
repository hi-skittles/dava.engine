#pragma once

#include "BaseParticleEditorContentWidget.h"

#include <DAVAEngine.h>
#include <Particles/ParticleForce.h>

#include <QWidget>

class ParticleVector3Widget;
class TimeLineWidget;
class QVBoxLayout;
class QCheckBox;
class WheellIgnorantComboBox;
class EventFilterDoubleSpinBox;
class QLabel;
class QLineEdit;
class QFrame;

namespace DAVA
{
class SceneEditor2;
} // namespace DAVA

class LayerForceWidget : public BaseParticleEditorContentWidget
{
    Q_OBJECT

public:
    explicit LayerForceWidget(QWidget* parent = nullptr);

    void BuildTimingSection();

    ~LayerForceWidget() = default;

    void Init(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::uint32 forceIndex, bool updateMinimized);

    DAVA::ParticleLayer* GetLayer() const;
    DAVA::int32 GetForceIndex() const;

    void Update();

    void StoreVisualState(DAVA::KeyedArchive* visualStateProps) override;
    void RestoreVisualState(DAVA::KeyedArchive* visualStateProps) override;

signals:
    void ValueChanged();

protected slots:
    void OnValueChanged();

private:
    void BuildShapeSection();
    void BuildCommonSection();
    void BuilDirectionSection();
    void BuildWindSection();
    void BuildPointGravitySection();
    void BuildPlaneCollisionSection();
    void UpdateVisibility(DAVA::ParticleForce::eShape shape, DAVA::ParticleForce::eTimingType timingType, DAVA::ParticleForce::eType forceType, bool isInfinityRange, bool isGlobalForce);
    void SetupSpin(EventFilterDoubleSpinBox* spin, DAVA::float32 singleStep = 0.0001, DAVA::int32 decimals = 4);

    QVBoxLayout* mainLayout = nullptr;
    QLabel* forceTypeLabel = nullptr;
    QLineEdit* forceNameEdit = nullptr;
    QCheckBox* isActive = nullptr;
    QCheckBox* infinityRange = nullptr;
    QCheckBox* isGlobal = nullptr;
    QLabel* isGlobalWarning = nullptr;
    QCheckBox* killParticles = nullptr;
    QCheckBox* worldAlign = nullptr;

    QFrame* shapeSeparator = nullptr;
    QLabel* shapeLabel = nullptr;
    WheellIgnorantComboBox* shapeComboBox = nullptr;
    ParticleVector3Widget* boxSize = nullptr;
    QWidget* radiusWidget = nullptr;
    EventFilterDoubleSpinBox* radiusSpin = nullptr;

    QLabel* timingLabel = nullptr;
    QFrame* timingTypeSeparator = nullptr;
    WheellIgnorantComboBox* timingTypeComboBox = nullptr;
    QLabel* forcePowerLabel = nullptr;
    ParticleVector3Widget* forcePower = nullptr;
    EventFilterDoubleSpinBox* forcePowerSpin = nullptr;
    TimeLineWidget* forcePowerTimeLine = nullptr;

    QLabel* startTimeLabel = nullptr;
    EventFilterDoubleSpinBox* startTimeSpin = nullptr;
    QLabel* endTimeLabel = nullptr;
    EventFilterDoubleSpinBox* endTimeSpin = nullptr;

    DAVA::ParticleLayer* layer = nullptr;
    DAVA::int32 forceIndex = -1;

    QFrame* directionSeparator = nullptr;
    ParticleVector3Widget* direction = nullptr;

    QFrame* windSeparator = nullptr;
    QLabel* windFreqLabel = nullptr;
    EventFilterDoubleSpinBox* windFreqSpin = nullptr;
    QLabel* windTurbLabel = nullptr;
    EventFilterDoubleSpinBox* windTurbSpin = nullptr;
    QLabel* windTurbFreqLabel = nullptr;
    EventFilterDoubleSpinBox* windTurbFreqSpin = nullptr;
    QLabel* windBiasLabel = nullptr;
    EventFilterDoubleSpinBox* windBiasSpin = nullptr;
    TimeLineWidget* turbulenceTimeLine = nullptr;
    QLabel* backTurbLabel = nullptr;
    EventFilterDoubleSpinBox* backTurbSpin = nullptr;

    QFrame* pointGravitySeparator = nullptr;
    QLabel* pointGravityRadiusLabel = nullptr;
    EventFilterDoubleSpinBox* pointGravityRadiusSpin = nullptr;
    QCheckBox* pointGravityUseRnd = nullptr;

    QFrame* planeCollisionSeparator = nullptr;
    QLabel* planeScaleLabel = nullptr;
    EventFilterDoubleSpinBox* planeScaleSpin = nullptr;
    QCheckBox* normalAsReflectionVector = nullptr;
    QCheckBox* killParticlesAfterCollision = nullptr;
    QLabel* reflectionChaosLabel = nullptr;
    EventFilterDoubleSpinBox* reflectionChaosSpin = nullptr;
    QCheckBox* randomizeReflectionForce = nullptr;
    QLabel* rndReflectionForceMinLabel = nullptr;
    EventFilterDoubleSpinBox* rndReflectionForceMinSpin = nullptr;
    QLabel* rndReflectionForceMaxLabel = nullptr;
    EventFilterDoubleSpinBox* rndReflectionForceMaxSpin = nullptr;
    QLabel* reflectionPercentLabel = nullptr;
    EventFilterDoubleSpinBox* reflectionPercentSpin = nullptr;

    QLabel* velocityThresholdLabel = nullptr;
    EventFilterDoubleSpinBox* velocityThresholdSpin = nullptr;
    bool blockSignals = false;

    DAVA::ParticleForce* selectedForce = nullptr;

    template <typename T>
    void UpdateKeys(DAVA::PropertyLine<T>* line, DAVA::float32 startTime, DAVA::float32 endTime);
    template <typename T>
    void ResetKeys(DAVA::Vector<typename DAVA::PropertyLine<T>::PropertyKey>& keys, const T& newValue, DAVA::float32 time);
    template <typename T>
    DAVA::int32 FindKeyIndex(DAVA::Vector<typename DAVA::PropertyLine<T>::PropertyKey>& keys, DAVA::float32 time, typename DAVA::PropertyLine<T>::PropertyKey& interpolatedKey);
};

template <typename T>
void LayerForceWidget::UpdateKeys(DAVA::PropertyLine<T>* line, DAVA::float32 startTime, DAVA::float32 endTime)
{
    if (line == nullptr || line->GetValues().empty())
        return;
    DAVA::Vector<typename DAVA::PropertyLine<T>::PropertyKey>& keys = line->GetValues();
    if (keys.front().t > endTime)
    {
        ResetKeys(keys, keys.front().value, startTime);
        return;
    }
    if (keys.back().t < startTime)
    {
        ResetKeys(keys, keys.back().value, startTime);
        return;
    }
    if (keys.front().t < startTime)
    {
        typename DAVA::PropertyLine<T>::PropertyKey key;
        DAVA::int32 index = FindKeyIndex<T>(keys, startTime, key);
        if (index != -1)
        {
            keys.erase(keys.begin(), keys.begin() + index + 1);
            keys.insert(keys.begin(), key);
        }
    }
    if (keys.back().t > endTime)
    {
        typename DAVA::PropertyLine<T>::PropertyKey key;
        DAVA::int32 index = FindKeyIndex<T>(keys, endTime, key);
        if (index != -1)
        {
            keys.erase(keys.begin() + index + 1, keys.end());
            keys.push_back(key);
        }
    }
}

template <typename T>
void LayerForceWidget::ResetKeys(DAVA::Vector<typename DAVA::PropertyLine<T>::PropertyKey>& keys, const T& newValue, DAVA::float32 time)
{
    typename DAVA::PropertyLine<T>::PropertyKey key;
    key.t = time;
    key.value = newValue;
    keys.clear();
    keys.push_back(key);
}

template <typename T>
DAVA::int32 LayerForceWidget::FindKeyIndex(DAVA::Vector<typename DAVA::PropertyLine<T>::PropertyKey>& keys, DAVA::float32 time, typename DAVA::PropertyLine<T>::PropertyKey& interpolatedKey)
{
    DAVA::int32 size = static_cast<DAVA::int32>(keys.size());
    for (int i = 0; i < size - 1; ++i)
    {
        if (keys[i].t < time && keys[i + 1].t > time)
        {
            DAVA::float32 t = (time - keys[i].t) / (keys[i + 1].t - keys[i].t);
            T v = DAVA::Lerp(keys[i].value, keys[i + 1].value, t);
            interpolatedKey.t = time;
            interpolatedKey.value = v;
            return i;
        }
    }
    return -1;
}

inline DAVA::ParticleLayer* LayerForceWidget::GetLayer() const
{
    return layer;
}

inline DAVA::int32 LayerForceWidget::GetForceIndex() const
{
    return forceIndex;
}
