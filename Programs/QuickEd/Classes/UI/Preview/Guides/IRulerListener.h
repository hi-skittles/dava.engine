#pragma once

#include <TArc/Qt/QtRect.h>
#include <Base/BaseTypes.h>

#include <QAction>
#include <QList>

class IRulerListener
{
    friend class RulerWidget;

    virtual void OnMousePress(DAVA::float32 position) = 0;
    virtual void OnMouseMove(DAVA::float32 position) = 0;
    virtual void OnMouseRelease(DAVA::float32 position) = 0;
    virtual void OnMouseLeave() = 0;

    virtual QList<QAction*> GetActions(DAVA::float32 position, QObject* parent) = 0;
};
