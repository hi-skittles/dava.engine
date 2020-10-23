#pragma once

#include <QLabel>
#include "DAVAEngine.h"

class QMimeData;

class ImageArea : public QLabel
{
    Q_OBJECT

public:
    explicit ImageArea(QWidget* parent = 0);
    void SetImage(const DAVA::FilePath& filePath);
    void SetImage(const DAVA::ScopedPtr<DAVA::Image>& image);
    inline const DAVA::ScopedPtr<DAVA::Image>& GetImage() const;
    DAVA::Vector2 GetAcceptableSize() const;
    const DAVA::FilePath& GetImagePath() const;

    void SetRequestedImageFormat(const DAVA::PixelFormat format);
    DAVA::PixelFormat GetRequestedImageFormat() const;

public slots:
    void ClearArea();
    void UpdatePreviewPicture();

    void SetAcceptableSize(const DAVA::Vector2& size);

signals:

    void changed();

private:
    void mousePressEvent(QMouseEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

    void ConnectSignals();
    DAVA::String GetDefaultPath() const;

    DAVA::ScopedPtr<DAVA::Image> image;
    DAVA::Vector2 acceptableSize;
    DAVA::FilePath imagePath;

    DAVA::PixelFormat requestedFormat;
};

inline const DAVA::ScopedPtr<DAVA::Image>& ImageArea::GetImage() const
{
    return image;
}
