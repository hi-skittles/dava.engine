#pragma once

#include "UI/Preview/Ruler/RulerSettings.h"

#include "Modules/CanvasModule/CanvasDataAdapter.h"

#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/DataProcessing/DataListener.h>

#include <QObject>
#include <QPoint>

#include <memory>

namespace DAVA
{
class Any;
class ContextAccessor;
}

class RulerController : public QObject, DAVA::DataListener
{
    Q_OBJECT

public:
    // Construction/destruction.
    RulerController(DAVA::ContextAccessor* accessor, QObject* parent = nullptr);
    ~RulerController() override;

public slots:
    // Update the ruler markers with the mouse position.
    void UpdateRulerMarkers(QPoint curMousePos);

signals:
    // Horizontal/Vertical ruler settings are changed.
    void HorisontalRulerSettingsChanged(const RulerSettings& settings);
    void VerticalRulerSettingsChanged(const RulerSettings& settings);

    // Horizontal/Vertical mark positions are changed.
    void HorisontalRulerMarkPositionChanged(int position);
    void VerticalRulerMarkPositionChanged(int position);

protected:
    // Update the rulers by sending "settings changed" signal to them.
    void UpdateRulers();

    void SetupInitialRulerSettings(RulerSettings& settings);

    // Recalculate the ruler settings depending on position/zoom level and emit signals.
    void RecalculateRulerSettings();

private:
    void OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields) override;

    void OnStartValueChanged(const DAVA::Any& startValue);
    void OnScaleChanged(const DAVA::Any& scaleValue);

    // Screen view pos and scale.
    QPoint viewPos;
    float screenScale = 1.0f;

    // Ruler settings.
    RulerSettings horisontalRulerSettings;
    RulerSettings verticalRulerSettings;

    CanvasDataAdapter canvasDataAdapter;

    DAVA::DataWrapper canvasDataAdapterWrapper;
    DAVA::ContextAccessor* accessor = nullptr;
};
