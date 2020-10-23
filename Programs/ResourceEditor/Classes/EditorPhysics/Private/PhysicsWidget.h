#pragma once

#include <Reflection/Reflection.h>

#include <QWidget>
#include <QIcon>

namespace DAVA
{
class ContextAccessor;
class UI;
} // namespace DAVA

class PhysicsWidget : public QWidget
{
public:
    PhysicsWidget(DAVA::ContextAccessor* accessor, DAVA::UI* ui);

private:
    void OnStartPauseClick();
    void OnStopClick();

    QIcon GetStartPauseIcon() const;
    QIcon GetStopIcon() const;

    QString GetLabelText() const;
    bool IsEnabled() const;

private:
    DAVA::ContextAccessor* accessor = nullptr;
    DAVA::UI* ui = nullptr;

    QIcon startIcon;
    QIcon stopIcon;
    QIcon pauseIcon;

    DAVA_REFLECTION(PhysicsWidget);
};