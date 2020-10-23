#include "UI/Preview/Guides/Guide.h"
#include "UI/Preview/Guides/GuideLabel.h"

#include <TArc/Utils/ScopedValueGuard.h>

#include <QWidget>

void Guide::Show()
{
    DAVA::ScopedValueGuard<bool> guard(inWork, true);
    line->show();
    text->show();
}

void Guide::Raise()
{
    DAVA::ScopedValueGuard<bool> guard(inWork, true);
    line->raise();
    text->raise();
}

void Guide::Hide()
{
    line->hide();
    text->hide();
}
