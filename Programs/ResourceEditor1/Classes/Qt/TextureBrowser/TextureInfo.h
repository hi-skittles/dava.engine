#pragma once

#include "Base/BaseTypes.h"

#include <QList>
#include <QImage>

struct TextureInfo
{
    QList<QImage> images;
    DAVA::uint32 dataSize = 0;
    DAVA::uint32 fileSize = 0;
    QSize imageSize = QSize(0, 0);
};
