#pragma once

#include "Render/Image/Image.h"

#include <QDialog>
#include <QScopedPointer>

#include <array>

namespace Ui
{
class ImageSplitterNormal;
}

class ImageArea;
class ImageSplitterDialogNormal : public QDialog
{
    Q_OBJECT

    enum ChannelsID
    {
        RED = 0,
        GREEN,
        BLUE,
        ALPHA,

        CHANNELS_COUNT
    };

public:
    explicit ImageSplitterDialogNormal(QWidget* parent = nullptr);
    ~ImageSplitterDialogNormal();

private slots:
    void OnSaveClicked();

private:
    void SaveAndReloadNormal(const DAVA::FilePath& pathname, int first, int second);

    DAVA::Image* CreateMergedImage(DAVA::Image* firstImage, DAVA::Image* secondImage);

private:
    QScopedPointer<Ui::ImageSplitterNormal> ui;
    DAVA::Array<ImageArea*, CHANNELS_COUNT> imageArreas;
};
