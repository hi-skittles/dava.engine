#include "IconHelper.h"

QString IconHelper::GetIconPathForClassName(const QString& className)
{
    QString s = ":/Icons/QtControlIcons/" + className.toLower() + ".png";

    return s;
}

QString IconHelper::GetCustomIconPath()
{
    return ":/Icons/QtControlIcons/custom_icon.png";
}
