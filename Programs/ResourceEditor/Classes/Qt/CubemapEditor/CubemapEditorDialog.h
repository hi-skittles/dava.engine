#ifndef _QT_CUBEMAPEDITORDIALOG_H_
#define _QT_CUBEMAPEDITORDIALOG_H_

#include <QDialog>
#include <QImage>
#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "Render/Image/ImageSystem.h"

class ClickableQLabel;

namespace Ui
{
class CubemapEditorDialog;
}

class CubemapEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CubemapEditorDialog(QWidget* parent = 0);
    ~CubemapEditorDialog();

    void InitForEditing(DAVA::FilePath& textureDescriptorPath, DAVA::FilePath& rootPath);
    void InitForCreating(DAVA::FilePath& textureDescriptorPath, DAVA::FilePath& rootPath);

public slots:

    virtual void done(int);

protected:
    typedef enum {
        eEditorModeNone,
        eEditorModeEditing,
        eEditorModeCreating
    } eEditorMode;

protected:
    DAVA::ImageInfo facesInfo;
    DAVA::Vector<DAVA::FilePath> facePathes;
    QString rootPath;

    bool faceChanged;

    eEditorMode editorMode;
    DAVA::FilePath targetFile;

protected:
    void ConnectSignals();
    void LoadImageFromUserFile(float rotation, int face);
    bool VerifyFirstImage(DAVA::ImageInfo imgInfo, QString& errorString);
    bool VerifyNextImage(DAVA::ImageInfo imgInfo, QString& errorString);
    void UpdateFaceInfo();
    void UpdateButtonState();
    bool AnyFaceLoaded();
    bool AllFacesLoaded();
    int GetLoadedFaceCount();
    void LoadCubemap(const QString& path);
    void SaveCubemap(const QString& path);
    DAVA::uint8 GetFaceMask();
    bool LoadImageTo(const DAVA::FilePath& filePath, int face, bool silent);
    ClickableQLabel* GetLabelForFace(int face);
    bool IsCubemapEdited();

    void mouseMoveEvent(QMouseEvent* ev);

protected slots:
    void OnPXClicked();
    void OnNXClicked();
    void OnPYClicked();
    void OnNYClicked();
    void OnPZClicked();
    void OnNZClicked();
    void OnRotationChanged();

    void OnLoadTexture();
    void OnSave();

private:
    Ui::CubemapEditorDialog* ui;
};

#endif // _QT_CUBEMAPEDITORDIALOG_H_
