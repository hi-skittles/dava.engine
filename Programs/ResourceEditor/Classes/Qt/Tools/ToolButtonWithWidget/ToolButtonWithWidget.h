#ifndef __TOOL_BUTTON_WITH_WIDGET_H__
#define __TOOL_BUTTON_WITH_WIDGET_H__

#include <QToolButton>

class ToolButtonWithWidget : public QToolButton
{
    Q_OBJECT

public:
    ToolButtonWithWidget(QWidget* parent = 0);

    void SetWidget(QWidget* widget);
};

#endif // __TOOL_BUTTON_WITH_WIDGET_H__
