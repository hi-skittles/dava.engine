#pragma once

#include "Classes/Qt/DockParticleEditor/ScrollZoomWidget.h"
#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include <QWidget>
#include <QString>
#include <QDialog>
#include <QScrollBar>
#include <QSlider>

class TimeLineWidget : public ScrollZoomWidget
{
    Q_OBJECT

public:
    explicit TimeLineWidget(QWidget* parent = 0);
    ~TimeLineWidget();

    void Init(DAVA::float32 minT, DAVA::float32 maxT, bool updateSizeState, bool aliasLinePoint = false, bool allowDeleteLine = true, bool integer = false, DAVA::int32 valueDecimalsPrecision = 2);
    void Init(DAVA::float32 minT, DAVA::float32 maxT, DAVA::float32 generalMinT, DAVA::float32 generalMaxT, bool updateSizeState, bool aliasLinePoint = false, bool allowDeleteLine = true, bool integer = false, DAVA::int32 valueDecimalsPrecision = 2);
    void SetMinLimits(DAVA::float32 minV);
    void SetMaxLimits(DAVA::float32 maxV);

    void EnableLock(bool enable);
    void SetVisualState(DAVA::KeyedArchive* visualStateProps);
    void GetVisualState(DAVA::KeyedArchive* visualStateProps);

    void AddLine(DAVA::uint32 lineId, const DAVA::Vector<DAVA::PropValue<DAVA::float32>>& line, const QColor& color, const QString& legend = "");
    void AddLines(const DAVA::Vector<DAVA::PropValue<DAVA::Vector2>>& lines, const DAVA::Vector<QColor>& colors, const DAVA::Vector<QString>& legends);
    void AddLines(const DAVA::Vector<DAVA::PropValue<DAVA::Vector3>>& lines, const DAVA::Vector<QColor>& colors, const DAVA::Vector<QString>& legends);

    bool GetValue(DAVA::uint32 lineId, DAVA::Vector<DAVA::PropValue<DAVA::float32>>* line) const;
    bool GetValues(DAVA::Vector<DAVA::PropValue<DAVA::Vector2>>* lines);
    bool GetValues(DAVA::Vector<DAVA::PropValue<DAVA::Vector3>>* lines);

    static bool SortPoints(const DAVA::Vector2& i, const DAVA::Vector2& j);

    // Add the mark to X/Y legend values (like 'deg' or 'pts').
    void SetXLegendMark(const QString& value);
    void SetYLegendMark(const QString& value);

signals:
    void TimeLineUpdated();

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    using ScrollZoomWidget::Init;
    typedef DAVA::Vector<DAVA::Vector2> LOGIC_POINTS;

    QRect GetGraphRect() const override;
    void DrawLine(QPainter* painter, DAVA::uint32 lineId);
    QPoint GetDrawPoint(const DAVA::Vector2& point) const;
    DAVA::Vector2 GetLogicPoint(const QPoint& point) const;
    QRect GetPointRect(const QPoint& point) const;

    QRect GetLineEnableRect(DAVA::uint32 lineId) const;
    int GetLegendHeight() const;
    QRect GetLineDrawRect() const;
    QRect GetMinimizeRect() const;
    QRect GetMaximizeRect() const;
    QRect GetLockRect() const;
    QRect GetIncreaseRect() const override;
    QRect GetScaleRect() const override;
    QRect GetDecreaseRect() const override;
    QRect GetSliderRect() const override;

    void SetPointValue(DAVA::uint32 lineId, DAVA::uint32 pointId, DAVA::Vector2 value, bool deleteSamePoints);

    void AddPoint(DAVA::uint32 lineId, const DAVA::Vector2& point);
    bool DeletePoint(DAVA::uint32 lineId, DAVA::uint32 pointId);

    DAVA::float32 GetYFromX(DAVA::uint32 lineId, DAVA::float32 x);

    void GraphRectClick(QMouseEvent* event);

    void UpdateLimits();

    void GetClickedPoint(const QPoint& point, DAVA::int32& pointId, DAVA::int32& lineId) const;
    void UpdateSizePolicy() override;

    void ChangePointValueDialog(DAVA::uint32 pointId, DAVA::int32 lineId);

    void PostAddLine();

    void PerformZoom(float newScale, bool moveScroll = true);

    void PerformOffset(int value, bool moveScroll = true);
    void DrawUITriangle(QPainter& painter, const QRect& rect, int rotateDegree);

    void GetCrossingPoint(const QPoint& firstPoint, const QPoint& secondPoint, QPoint& leftBorderCrossPoint, QPoint& rightBorderCrossPoint);

private:
    DAVA::int32 selectedPoint = -1;
    DAVA::int32 selectedLine = -1;
    DAVA::int32 drawLine = -1;
    DAVA::int32 valueDecimalsPrecision = 2;

    bool isLockEnable = false;
    bool isLocked = false;
    bool isInteger = false;

    enum eSizeState
    {
        SIZE_STATE_NORMAL,
        SIZE_STATE_MINIMIZED,
        SIZE_STATE_DOUBLE
    };
    eSizeState sizeState = SIZE_STATE_NORMAL;
    bool updateSizeState = true;
    bool aliasLinePoint = false;
    bool allowDeleteLine = true;

    typedef struct
    {
        LOGIC_POINTS line;
        QColor color;
        QString legend;
    } LINE;
    typedef DAVA::Map<DAVA::uint32, LINE> LINES_MAP;
    LINES_MAP lines;

    DAVA::Vector2 newPoint;

    QString xLegendMark;
    QString yLegendMark;
};

class SetPointValueDlg : public QDialog
{
    Q_OBJECT

public:
    explicit SetPointValueDlg(DAVA::float32 time, DAVA::float32 minTime, DAVA::float32 maxTime, DAVA::float32 value,
                              DAVA::float32 minValue, DAVA::float32 maxValue, QWidget* parent = 0, bool integer = false, DAVA::int32 valueDecimalsPrecision = 2);

    DAVA::float32 GetTime() const;
    DAVA::float32 GetValue() const;

private:
    bool isInteger;

    EventFilterDoubleSpinBox* timeSpin;
    EventFilterDoubleSpinBox* valueSpin;
    QSpinBox* valueSpinInt;
};
