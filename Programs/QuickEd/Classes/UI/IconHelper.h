#ifndef __UIEDITOR__ICONHELPER__
#define __UIEDITOR__ICONHELPER__

#include <TArc/Qt/QtString.h>

class IconHelper
{
public:
    static QString GetIconPathForClassName(const QString& className);
    static QString GetCustomIconPath();
};

#endif /* defined(__UIEDITOR__ICONHELPER__) */
