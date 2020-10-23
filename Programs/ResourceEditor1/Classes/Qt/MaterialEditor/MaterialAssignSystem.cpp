#include "MaterialAssignSystem.h"
#include "Scene/SceneEditor2.h"

#include "Commands2/MaterialSwitchParentCommand.h"

#include "Base/BaseTypes.h"
#include "MaterialModel.h"

//Qt
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QCursor>

void MaterialAssignSystem::AssignMaterial(SceneEditor2* scene, DAVA::NMaterial* instance, DAVA::NMaterial* newMaterialParent)
{
    scene->Exec(std::unique_ptr<DAVA::Command>(new MaterialSwitchParentCommand(instance, newMaterialParent)));
}

DAVA::NMaterial* MaterialAssignSystem::SelectMaterial(const DAVA::Set<DAVA::NMaterial*>& materials)
{
    if (materials.size() > 1)
    {
        QMenu selectMaterialMenu;

        auto endIt = materials.end();
        for (auto it = materials.begin(); it != endIt; ++it)
        {
            QVariant materialAsVariant = QVariant::fromValue<DAVA::NMaterial*>(*it);

            QString text = QString((*it)->GetMaterialName().c_str());
            QAction* action = selectMaterialMenu.addAction(text);
            action->setData(materialAsVariant);
        }

        QAction* selectedMaterialAction = selectMaterialMenu.exec(QCursor::pos());
        if (selectedMaterialAction)
        {
            QVariant materialAsVariant = selectedMaterialAction->data();
            return materialAsVariant.value<DAVA::NMaterial*>();
        }
    }
    else if (materials.size() == 1)
    {
        return *materials.begin();
    }

    return NULL;
}
