#include "ToolButtonWithWidget.h"

#include <QWidgetAction>
#include <QMenu>

ToolButtonWithWidget::ToolButtonWithWidget(QWidget* parent /*= 0*/)
    : QToolButton(parent)
{
    setPopupMode(QToolButton::MenuButtonPopup);
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setAutoRaise(false);
}

void ToolButtonWithWidget::SetWidget(QWidget* widget)
{
    QWidgetAction* wa = new QWidgetAction(this);
    wa->setDefaultWidget(widget);
    QMenu* m = new QMenu(this);
    m->addAction(wa);

    setMenu(m);
}
