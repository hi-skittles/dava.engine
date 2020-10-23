#pragma once

#include "Base/BaseTypes.h"
#include "Scene3D/Lod/LodComponent.h"

#include <QWidget>

class QLabel;
class EventFilterDoubleSpinBox;
class QLineEdit;
class QPushButton;

class LODDistanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LODDistanceWidget(QWidget* parent = nullptr);

    void SetDistance(DAVA::float32 distance, bool isMultiple);
    DAVA::float32 GetDistance() const;

    void SetMinMax(DAVA::float64 min, DAVA::float64 max);
    void SetActive(bool active);
    void SetCanDelete(bool canDelete);
    void SetIndex(DAVA::int32 widgetIndex);
    void SetTrianglesCount(DAVA::uint32 trianglesCount);

signals:
    void DistanceChanged();
    void DistanceRemoved();

private slots:

    void DistanceChangedBySpinBox(double value);
    void DistanceChangedByResetButton();
    void DistanceChangedByLineEdit();

private:
    void CreateUI();
    void SetupSignals();

    void UpdateLabelText();
    void UpdateDistanceValues();
    void UpdateDisplayingStyle();

    DAVA::float32 ClampDistanceBySpinBox(DAVA::float32 newDistance);

    QLabel* label = nullptr;
    EventFilterDoubleSpinBox* spinBox = nullptr;
    QPushButton* deleteButton = nullptr;
    QPushButton* resetButton = nullptr;
    QLineEdit* multipleText = nullptr;

    DAVA::float64 maxValue = DAVA::LodComponent::MAX_LOD_DISTANCE;
    DAVA::float64 minValue = DAVA::LodComponent::MIN_LOD_DISTANCE;
    DAVA::float32 distance = DAVA::LodComponent::MIN_LOD_DISTANCE;

    DAVA::uint32 trianglesCount = 0;
    DAVA::uint32 widgetIndex = 0;
    bool active = true;
    bool canDelete = false;
    bool isMultiple = false;
};
