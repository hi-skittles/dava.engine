#include "UI/Preview/Ruler/RulerController.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Core/FieldBinder.h>

#include <cmath>

RulerController::RulerController(DAVA::ContextAccessor* accessor_, QObject* parent)
    : QObject(parent)
    , screenScale(0.0f)
    , canvasDataAdapter(accessor_)
    , accessor(accessor_)
{
    canvasDataAdapterWrapper = accessor->CreateWrapper([this](const DAVA::DataContext*) { return DAVA::Reflection::Create(&canvasDataAdapter); });
    canvasDataAdapterWrapper.SetListener(this);

    OnScaleChanged(1.0f);
}

RulerController::~RulerController() = default;

void RulerController::UpdateRulerMarkers(QPoint curMousePos)
{
    emit HorisontalRulerMarkPositionChanged(curMousePos.x());
    emit VerticalRulerMarkPositionChanged(curMousePos.y());
}

void RulerController::UpdateRulers()
{
    emit HorisontalRulerSettingsChanged(horisontalRulerSettings);
    emit VerticalRulerSettingsChanged(verticalRulerSettings);
}

void RulerController::RecalculateRulerSettings()
{
    static const struct
    {
        float scaleLevel;
        int smallTicksDelta;
        int bigTicksDelta;
    } ticksMap[] =
    {
      { 0.1f, 100, 500 },
      { 0.25f, 32, 320 },
      { 0.5f, 20, 200 },
      { 0.75f, 16, 80 },
      { 1.0f, 5, 50 },
      { 2.0f, 5, 50 },
      { 4.0f, 2, 20 },
      { 8.0f, 1, 10 },
      { 12.0f, 1, 10 },
      { 16.0f, 1, 10 },
      { 32.0f, 1, 10 }
    };

    // Look for the closest value.
    int closestValueIndex = 0;
    float closestScaleDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < sizeof(ticksMap) / sizeof(ticksMap[0]); i++)
    {
        float curScaleDistance = std::fabs(ticksMap[i].scaleLevel - screenScale);
        if (curScaleDistance < closestScaleDistance)
        {
            closestScaleDistance = curScaleDistance;
            closestValueIndex = i;
        }
    }

    horisontalRulerSettings.smallTicksDelta = ticksMap[closestValueIndex].smallTicksDelta;
    horisontalRulerSettings.bigTicksDelta = ticksMap[closestValueIndex].bigTicksDelta;
    verticalRulerSettings.smallTicksDelta = ticksMap[closestValueIndex].smallTicksDelta;
    verticalRulerSettings.bigTicksDelta = ticksMap[closestValueIndex].bigTicksDelta;

    UpdateRulers();
}

void RulerController::OnStartValueChanged(const DAVA::Any& startValue)
{
    QPoint pos(0, 0);
    if (startValue.CanGet<DAVA::Vector2>())
    {
        DAVA::Vector2 davaPos = startValue.Get<DAVA::Vector2>();
        pos.setX(davaPos.x);
        pos.setY(davaPos.y);
    }

    if (viewPos != pos)
    {
        viewPos = pos;
        horisontalRulerSettings.startPos = viewPos.x();
        verticalRulerSettings.startPos = viewPos.y();

        UpdateRulers();
    }
}

void RulerController::OnScaleChanged(const DAVA::Any& scaleValue)
{
    screenScale = 1.0f;
    if (scaleValue.CanGet<DAVA::float32>())
    {
        screenScale = scaleValue.Get<DAVA::float32>();
    }

    horisontalRulerSettings.zoomLevel = screenScale;
    verticalRulerSettings.zoomLevel = screenScale;

    RecalculateRulerSettings();
}

void RulerController::OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    bool startValueChanged = std::find(fields.begin(), fields.end(), CanvasDataAdapter::startValuePropertyName) != fields.end();
    if (startValueChanged)
    {
        OnStartValueChanged(wrapper.GetFieldValue(CanvasDataAdapter::startValuePropertyName));
    }

    bool scaleChanged = std::find(fields.begin(), fields.end(), CanvasDataAdapter::scalePropertyName) != fields.end();
    if (scaleChanged)
    {
        OnScaleChanged(wrapper.GetFieldValue(CanvasDataAdapter::scalePropertyName));
    }
}
