#ifndef __QT_LABEL_WITH_ACTIONS_H__
#define __QT_LABEL_WITH_ACTIONS_H__

#include <QLabel>

class QMenu;
class QAction;
class QtLabelWithActions : public QLabel
{
    Q_OBJECT

public:
    QtLabelWithActions(QWidget* parent = 0);
    ~QtLabelWithActions();

    void setMenu(QMenu* menu);
    void setDefaultAction(QAction* action);

    void SetTextColor(const QColor& color);

protected slots:

    void MenuTriggered(QAction* action);

protected:
    void mousePressEvent(QMouseEvent* event);
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);

protected:
    QMenu* menu;
};

#endif // __QT_LABEL_WITH_ACTIONS_H__
