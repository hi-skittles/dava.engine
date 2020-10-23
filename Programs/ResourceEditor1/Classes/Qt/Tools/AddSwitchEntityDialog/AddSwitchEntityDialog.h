#pragma once

#include "Classes/Qt/Tools/BaseAddEntityDialog/BaseAddEntityDialog.h"

#include <Base/Vector.h>

namespace DAVA
{
class Entity;
}

class SelectEntityPathWidget;
class AddSwitchEntityDialog : public BaseAddEntityDialog
{
    Q_OBJECT

public:
    AddSwitchEntityDialog(QWidget* parent = 0);
    ~AddSwitchEntityDialog();

    void accept() override;
    void reject() override;

protected:
    void GetPathEntities(DAVA::Vector<DAVA::Entity*>& entities, SceneEditor2* editor);
    void FillPropertyEditorWithContent() override;

private:
    void CleanupPathWidgets();

    DAVA::Vector<SelectEntityPathWidget*> pathWidgets;
    DAVA::Vector<QWidget*> additionalWidgets;
};
