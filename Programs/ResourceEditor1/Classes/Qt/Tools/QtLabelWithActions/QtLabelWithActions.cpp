#include "QtLabelWithActions.h"

#include "DAVAEngine.h"

#include <QMenu>
#include <QAction>
#include <QMouseEvent>

QtLabelWithActions::QtLabelWithActions(QWidget* parent /*= 0*/)
    : QLabel(parent)
    , menu(NULL)
{
    SetTextColor(Qt::white);
}

QtLabelWithActions::~QtLabelWithActions()
{
}

void QtLabelWithActions::mousePressEvent(QMouseEvent* event)
{
    if (menu)
    {
        menu->exec(mapToGlobal(geometry().bottomLeft()));
    }
}

void QtLabelWithActions::enterEvent(QEvent* event)
{
    SetTextColor(Qt::yellow);
}

void QtLabelWithActions::leaveEvent(QEvent* event)
{
    SetTextColor(Qt::white);
}

void QtLabelWithActions::setMenu(QMenu* _menu)
{
    if (menu)
    {
        QObject::disconnect(this, SLOT(MenuTriggered(QAction*)));
    }

    menu = _menu;

    if (menu)
    {
        QObject::connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(MenuTriggered(QAction*)));
    }
}

void QtLabelWithActions::setDefaultAction(QAction* action)
{
    if (action)
    {
        setText(DAVA::Format("[ %s ]", action->text().toStdString().c_str()).c_str());
    }
    else
    {
        setText("");
    }
}

void QtLabelWithActions::MenuTriggered(QAction* action)
{
    setDefaultAction(action);
}

void QtLabelWithActions::SetTextColor(const QColor& color)
{
    QPalette pal = palette();
    pal.setColor(QPalette::WindowText, color);
    setPalette(pal);
}
