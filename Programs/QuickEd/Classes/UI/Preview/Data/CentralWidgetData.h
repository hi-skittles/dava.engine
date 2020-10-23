#pragma once

#include <TArc/DataProcessing/TArcDataNode.h>

#include <Math/Vector.h>

class QWidget;

class CentralWidgetData : public DAVA::TArcDataNode
{
public:
    CentralWidgetData(QWidget* renderWidget, QWidget* hRulerWidget, QWidget* vRulerWidget);

    //size of render widget
    static DAVA::FastName viewSizePropertyName;

    //guides start position
    //as an example for vertical guides it is horizontal ruler top
    static DAVA::FastName guidesPosPropertyName;

    //guides size
    static DAVA::FastName guidesSizePropertyName;

    //guides offset to calculate position from value
    //as an example for vertical guides it is horizontal ruler left
    static DAVA::FastName guidesRelativePosPropertyName;

    DAVA::Vector2 GetViewSize() const;
    DAVA::Vector2 GetGuidesPos() const;
    DAVA::Vector2 GetGuidesSize() const;
    DAVA::Vector2 GetGuidesRelativePos() const;

private:
    QWidget* renderWidget = nullptr;
    QWidget* hRulerWidget = nullptr;
    QWidget* vRulerWidget = nullptr;

    DAVA_VIRTUAL_REFLECTION(CentralWidgetData, DAVA::TArcDataNode);
};
