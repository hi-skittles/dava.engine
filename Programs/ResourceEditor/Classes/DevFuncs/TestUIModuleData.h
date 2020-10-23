#pragma once

#include <Base/String.h>

#include <QIcon>
#include <QPixmap>
#include <QImage>

struct ComboBoxTestDataDescr
{
    DAVA::String text;
    QIcon icon;

    bool operator==(const ComboBoxTestDataDescr& other) const
    {
        if (text != other.text)
        {
            return false;
        }

        QList<QSize> leftSizes = icon.availableSizes();
        QList<QSize> rightSizes = other.icon.availableSizes();

        if (leftSizes != rightSizes)
            return false;

        if (leftSizes.size() == 0)
            return true;

        QPixmap left = icon.pixmap(leftSizes.at(0));
        QPixmap right = other.icon.pixmap(leftSizes.at(0));

        return (left.toImage() == right.toImage());
    }
};

inline DAVA::String ComboBoxTestDataDescrToString(const DAVA::Any& value)
{
    return value.Get<ComboBoxTestDataDescr>().text;
}

inline QIcon ComboBoxTestDataDescrToQIcon(const DAVA::Any& value)
{
    return value.Get<ComboBoxTestDataDescr>().icon;
}
