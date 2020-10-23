#include "Classes/Application/REGlobal.h"

#include <TArc/Core/Core.h>
#include <TArc/Core/FieldBinder.h>

namespace REGlobal
{
namespace REGlobalDetails
{
DAVA::TArc::Core* coreInstance = nullptr;

DAVA::TArc::CoreInterface* GetCoreInterface()
{
    return coreInstance->GetCoreInterface();
}

DAVA::TArc::UI* GetUI()
{
    return coreInstance->GetUI();
}
}

DAVA::TArc::DataContext* GetGlobalContext()
{
    DAVA::TArc::CoreInterface* coreInterface = REGlobalDetails::GetCoreInterface();
    if (coreInterface == nullptr)
        return nullptr;
    return coreInterface->GetGlobalContext();
}

DAVA::TArc::DataContext* GetActiveContext()
{
    DAVA::TArc::CoreInterface* coreInterface = REGlobalDetails::GetCoreInterface();
    if (coreInterface == nullptr)
        return nullptr;
    return coreInterface->GetActiveContext();
}

DAVA::TArc::OperationInvoker* GetInvoker()
{
    return REGlobalDetails::GetCoreInterface();
}

DAVA::TArc::ContextAccessor* GetAccessor()
{
    return REGlobalDetails::GetCoreInterface();
}

DAVA::TArc::FieldBinder* CreateFieldBinder()
{
    return new DAVA::TArc::FieldBinder(GetAccessor());
}

DAVA::TArc::DataWrapper CreateDataWrapper(const DAVA::ReflectedType* type)
{
    DAVA::TArc::CoreInterface* coreInterface = REGlobalDetails::GetCoreInterface();
    if (coreInterface == nullptr)
        return DAVA::TArc::DataWrapper();
    return coreInterface->CreateWrapper(type);
}

DAVA::TArc::ModalMessageParams::Button ShowModalMessage(const DAVA::TArc::ModalMessageParams& params)
{
    DAVA::TArc::UI* ui = REGlobalDetails::GetUI();
    DVASSERT(ui != nullptr);
    if (ui == nullptr)
    {
        return DAVA::TArc::ModalMessageParams::NoButton;
    }
    return ui->ShowModalMessage(DAVA::TArc::mainWindowKey, params);
}

void ShowNotification(const DAVA::TArc::NotificationParams& params)
{
    DAVA::TArc::UI* ui = REGlobalDetails::GetUI();
    DVASSERT(ui != nullptr);
    if (ui != nullptr)
    {
        ui->ShowNotification(DAVA::TArc::mainWindowKey, params);
    }
}

void InitTArcCore(DAVA::TArc::Core* core)
{
    REGlobalDetails::coreInstance = core;
}

IMPL_OPERATION_ID(OpenLastProjectOperation);
IMPL_OPERATION_ID(CreateFirstSceneOperation);
IMPL_OPERATION_ID(OpenSceneOperation);
IMPL_OPERATION_ID(AddSceneOperation);
IMPL_OPERATION_ID(SaveCurrentScene);
IMPL_OPERATION_ID(CloseAllScenesOperation);
IMPL_OPERATION_ID(ReloadAllTextures);
IMPL_OPERATION_ID(ReloadTextures);
IMPL_OPERATION_ID(ShowMaterial);
IMPL_OPERATION_ID(ConvertTaggedTextures);

} // namespace REGlobal
