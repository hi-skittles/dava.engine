#ifndef __RESOURCEEDITORQT__PLANELODDIALOG__
#define __RESOURCEEDITORQT__PLANELODDIALOG__

#include "DAVAEngine.h"
#include <QDialog>

namespace Ui
{
class QtPlaneLODDialog;
}

class PlaneLODDialog : public QDialog
{
    Q_OBJECT

public:
    PlaneLODDialog(DAVA::uint32 layersCount, const DAVA::FilePath& defaultTexturePath, QWidget* parent = 0);
    ~PlaneLODDialog();

    DAVA::int32 GetSelectedLayer();
    DAVA::uint32 GetSelectedTextureSize();
    DAVA::FilePath GetSelectedTexturePath();

private slots:
    void OnCancel();
    void OnOk();

    void OnTextureSelect();

private:
    Ui::QtPlaneLODDialog* ui;

    QString texturePath;
    DAVA::int32 selectedLayer;
    DAVA::uint32 selectedTextureSize;
};

#endif // __RESOURCEEDITORQT__PLANELODDIALOG__
