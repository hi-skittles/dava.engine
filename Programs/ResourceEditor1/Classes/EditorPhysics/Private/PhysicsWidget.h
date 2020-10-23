#pragma once

#include <Reflection/Reflection.h>

#include <QWidget>
#include <QIcon>

namespace DAVA
{
namespace TArc
{
class ContextAccessor;
class UI;
} // namespace TArc
} // namespace DAVA

class PhysicsWidget : public QWidget
{
public:
    PhysicsWidget(DAVA::TArc::ContextAccessor* accessor, DAVA::TArc::UI* ui);

private:
    void OnStartPauseClick();
    void OnStopClick();

    QIcon GetStartPauseIcon() const;
    QIcon GetStopIcon() const;

    QString GetLabelText() const;
    bool IsEnabled() const;

private:
    DAVA::TArc::ContextAccessor* accessor = nullptr;
    DAVA::TArc::UI* ui = nullptr;

    QIcon startIcon;
    QIcon stopIcon;
    QIcon pauseIcon;

    DAVA_REFLECTION(PhysicsWidget);
};