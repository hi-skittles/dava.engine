#include "EditorSystems/CursorSystem.h"
#include "Model/PackageHierarchy/ControlNode.h"

#include <Engine/Qt/RenderWidget.h>
#include <Debug/DVAssert.h>
#include <UI/UIControl.h>
#include <Engine/PlatformApiQt.h>

#include <QPixmap>
#include <QTransform>

using namespace DAVA;

QMap<QString, QPixmap> CursorSystem::cursorpixes;

CursorSystem::CursorSystem(DAVA::ContextAccessor* accessor)
    : BaseEditorSystem(accessor)
{
    GetSystemsManager()->activeAreaChanged.Connect(this, &CursorSystem::OnActiveAreaChanged);
}

void CursorSystem::OnActiveAreaChanged(const HUDAreaInfo& areaInfo)
{
    using namespace DAVA;

    RenderWidget* renderWidget = PlatformApi::Qt::GetRenderWidget();
    if (areaInfo.area == eArea::NO_AREA)
    {
        renderWidget->unsetCursor();
    }
    else
    {
        auto control = areaInfo.owner->GetControl();
        float angle = control->GetGeometricData().angle;
        QPixmap pixmap = CreatePixmapForArea(angle, areaInfo.area);
        renderWidget->setCursor(QCursor(pixmap));
    }
}

void CursorSystem::OnDragStateChanged(eDragState currentState, eDragState previousState)
{
    using namespace DAVA;

    RenderWidget* renderWidget = PlatformApi::Qt::GetRenderWidget();

    if (currentState == eDragState::AddingControl)
    {
        renderWidget->setCursor(Qt::CrossCursor);
    }
    else if (currentState == eDragState::DragScreen)
    {
        renderWidget->setCursor(Qt::OpenHandCursor);
    }
    else if (previousState == eDragState::AddingControl || previousState == eDragState::DragScreen)
    {
        renderWidget->unsetCursor();
    }
}

QPixmap CursorSystem::CreatePixmapForArea(float angle, const eArea area) const
{
    QTransform transform;
    transform.rotateRadians(angle);
    QPixmap pixmap;
    switch (area)
    {
    case eArea::FRAME_AREA:
        return CreatePixmap(":/Cursors/moveCursor.png");
    case eArea::PIVOT_POINT_AREA:
        return CreatePixmap(":/Cursors/cursorCross.png");
    case eArea::TOP_LEFT_AREA:
    case eArea::BOTTOM_RIGHT_AREA:
        pixmap = CreatePixmap(":/Cursors/northWestSouthEastResizeCursor.png");
        return pixmap.transformed(transform);
    case eArea::TOP_RIGHT_AREA:
    case eArea::BOTTOM_LEFT_AREA:
        pixmap = CreatePixmap(":/Cursors/northEastSouthWestResizeCursor.png");
        return pixmap.transformed(transform);
    case eArea::TOP_CENTER_AREA:
    case eArea::BOTTOM_CENTER_AREA:
        pixmap = CreatePixmap(":/Cursors/northSouthResizeCursor.png");
        return pixmap.transformed(transform);
    case eArea::CENTER_LEFT_AREA:
    case eArea::CENTER_RIGHT_AREA:
        pixmap = CreatePixmap(":/Cursors/eastWestResizeCursor.png");
        return pixmap.transformed(transform);
    case eArea::ROTATE_AREA:
        return CreatePixmap(":/Cursors/cursorRotate.png");
    default:
        DVASSERT(false, "unexpected enum value");
        return QPixmap();
    }
}

QPixmap CursorSystem::CreatePixmap(const QString& address) const
{
    if (cursorpixes.contains(address))
    {
        return cursorpixes[address];
    }
    else
    {
        QPixmap pixmap(address);
        DVASSERT(!pixmap.isNull());
        cursorpixes.insert(address, pixmap);
        return pixmap;
    }
}

eSystems CursorSystem::GetOrder() const
{
    return eSystems::CURSOR;
}
