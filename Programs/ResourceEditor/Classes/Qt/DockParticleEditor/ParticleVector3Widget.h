#pragma once

#include <DAVAEngine.h>

#include <QWidget>

class EventFilterDoubleSpinBox;
class QLabel;
class QGroupBox;

class ParticleVector3Widget : public QWidget
{
    Q_OBJECT

public:
    ParticleVector3Widget(const std::string& label, const DAVA::Vector3& initVector);
    ~ParticleVector3Widget() = default;

    DAVA::Vector3 GetValue() const;
    void SetValue(const DAVA::Vector3& value);

protected slots:
    void OnValueChanged();

signals:
    void valueChanged();

private:
    void InitSpinBox(EventFilterDoubleSpinBox* spin, DAVA::float32 value);
    EventFilterDoubleSpinBox* xSpin = nullptr;
    EventFilterDoubleSpinBox* ySpin = nullptr;
    EventFilterDoubleSpinBox* zSpin = nullptr;
};