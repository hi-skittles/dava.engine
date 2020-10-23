#ifndef __RESOURCEEDITORQT__IMAGESPLITTER_DIALOG_H__
#define __RESOURCEEDITORQT__IMAGESPLITTER_DIALOG_H__

#include "DAVAEngine.h"

#include <QDialog>
#include <QScopedPointer>

namespace Ui
{
class ImageSplitter;
}

class ImageArea;
class ImageSplitterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageSplitterDialog(QWidget* parent = nullptr);
    ~ImageSplitterDialog();

private slots:
    void PathChanged(const QString& path);
    void ImageAreaChanged();
    void OnRestoreClicked();
    void OnSaveAsClicked(bool saveSplittedImages = false);
    void OnSaveClicked();
    void OnSaveChannelsClicked();
    void OnFillBtnClicked();
    void OnReload();
    void OnReloadSpecularMap();

private:
    void ConnectSignals();
    void SetAcceptableImageSize(const DAVA::Vector2& newSize);
    void Save(const DAVA::FilePath& filePath, bool saveSplittedImagesSeparately);
    DAVA::String GetDefaultPath() const;

    QScopedPointer<Ui::ImageSplitter> ui;
    DAVA::Vector2 acceptableSize;
    DAVA::String lastSelectedFile;
    DAVA::Vector<ImageArea*> rgbaControls;
};

#endif // __RESOURCEEDITORQT__IMAGESPLITTER_DIALOG_H__
