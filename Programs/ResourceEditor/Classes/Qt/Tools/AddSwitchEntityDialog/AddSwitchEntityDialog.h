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
    AddSwitchEntityDialog(QWidget* parent = nullptr);
    ~AddSwitchEntityDialog();

    void accept() override;
    void reject() override;

private:
    DAVA::Vector<DAVA::RefPtr<DAVA::Entity>> GetPathEntities();

    void OnPathChanged();
    void CleanupPathWidgets();

    DAVA::Vector<SelectEntityPathWidget*> pathWidgets;
    DAVA::Vector<QWidget*> additionalWidgets;
};
