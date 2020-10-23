#if defined(__DAVAENGINE_SPEEDTREE__)

#include "Classes/SpeedTreeModule/SpeedTreeModule.h"
#include "Classes/SpeedTreeModule/Private/SpeedTreeImportDialog.h"

#include "Classes/Application/REGlobalOperationsData.h"
#include "Classes/Project/ProjectManagerData.h"

#include "TArc/WindowSubSystem/ActionUtils.h"
#include "TArc/WindowSubSystem/UI.h"
#include "TArc/WindowSubSystem/QtAction.h"
#include "TArc/Utils/ModuleCollection.h"

void SpeedTreeModule::PostInit()
{
    using namespace DAVA::TArc;

    const QString importSpeedTreeName("SpeedTree from XML...");

    {
        QtAction* importAction = new QtAction(GetAccessor(), importSpeedTreeName);

        FieldDescriptor fieldDescriptor(DAVA::ReflectedTypeDB::Get<ProjectManagerData>(), DAVA::FastName(ProjectManagerData::ProjectPathProperty));
        importAction->SetStateUpdationFunction(QtAction::Enabled, fieldDescriptor, [](const DAVA::Any& fieldValue) -> DAVA::Any
                                               {
                                                   return fieldValue.Cast<DAVA::FilePath>().IsEmpty() == false;
                                               });

        ActionPlacementInfo menuPlacement(CreateMenuPoint("File", InsertionParams(InsertionParams::eInsertionMethod::AfterItem, QStringLiteral("exportSeparator"))));
        GetUI()->AddAction(DAVA::TArc::mainWindowKey, menuPlacement, importAction);
        connections.AddConnection(importAction, &QAction::triggered, DAVA::MakeFunction(this, &SpeedTreeModule::OnImportSpeedTree));
    }

    {
        QAction* separator = new QAction(nullptr);
        separator->setObjectName(QStringLiteral("oimportSeparator"));
        separator->setSeparator(true);

        ActionPlacementInfo placementInfo;
        placementInfo.AddPlacementPoint(CreateMenuPoint(MenuItems::menuFile, { InsertionParams::eInsertionMethod::AfterItem, importSpeedTreeName }));
        GetUI()->AddAction(mainWindowKey, placementInfo, separator);
    }
}

void SpeedTreeModule::OnImportSpeedTree()
{
    REGlobalOperationsData* globalOpData = GetAccessor()->GetGlobalContext()->GetData<REGlobalOperationsData>();
    DVASSERT(globalOpData != nullptr);

    SpeedTreeImportDialog importDialog(globalOpData->GetGlobalOperations(), GetUI()->GetWindow(DAVA::TArc::mainWindowKey));
    importDialog.exec();
}

DAVA_VIRTUAL_REFLECTION_IMPL(SpeedTreeModule)
{
    DAVA::ReflectionRegistrator<SpeedTreeModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_GUI_MODULE(SpeedTreeModule);

#endif //#if defined (__DAVAENGINE_SPEEDTREE__)
