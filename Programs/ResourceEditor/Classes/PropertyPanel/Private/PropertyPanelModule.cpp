#include "Classes/PropertyPanel/PropertyPanelModule.h"
#include "Classes/PropertyPanel/PropertyModelExt.h"
#include "Classes/PropertyPanel/QualitySettingsComponentExt.h"
#include "Classes/PropertyPanel/KeyedArchiveExtensions.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/DataNodes/SelectionData.h>

#include <TArc/Controls/PropertyPanel/PropertiesView.h>
#include <TArc/Controls/PropertyPanel/TimerUpdater.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/DataProcessing/TArcDataNode.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/QtDelayedExecutor.h>

#include <Scene3D/Entity.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Reflection/ReflectedObject.h>
#include <Base/FastName.h>
#include <Base/BaseTypes.h>

#include <QPointer>
#include <QList>
#include <QString>
#include <QTimer>

namespace PropertyPanelModuleDetail
{
class PropertyPanelUpdater : public DAVA::PropertiesView::Updater
{
public:
    PropertyPanelUpdater()
        : timerUpdater(1000, 100)
    {
        SceneSignals* sceneSignals = SceneSignals::Instance();
        connections.AddConnection(sceneSignals, &SceneSignals::CommandExecuted, [this](DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
                                  {
                                      QueueFullUpdate();
                                  });

        timerUpdater.update.Connect(this, &PropertyPanelUpdater::EmitUpdate);
    }

private:
    void EmitUpdate(DAVA::PropertiesView::UpdatePolicy policy)
    {
        update.Emit(policy);
    }

    void QueueFullUpdate()
    {
        executor.DelayedExecute(DAVA::Bind(&PropertyPanelUpdater::EmitUpdate, this, DAVA::PropertiesView::FullUpdate));
    }

private:
    DAVA::TimerUpdater timerUpdater;
    DAVA::QtConnections connections;
    DAVA::QtDelayedExecutor executor;
};

class PropertyPanelData : public DAVA::TArcDataNode
{
public:
    std::shared_ptr<DAVA::PropertiesView::Updater> updater;
    QPointer<DAVA::PropertiesView> view;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(PropertyPanelData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<PropertyPanelData>::Begin()
        .End();
    }
};

DAVA::Vector<DAVA::Reflection> CastSelectableGroupToReflection(const DAVA::Any& v)
{
    const DAVA::SelectableGroup& selection = v.Get<DAVA::SelectableGroup>();
    DAVA::Vector<DAVA::Reflection> result;
    result.reserve(selection.GetSize());
    for (DAVA::Selectable object : selection.GetContent())
    {
        DAVA::Any obj = object.GetContainedObject();
        result.push_back(DAVA::Reflection::Create(obj));
    }

    return result;
}
}

void PropertyPanelModule::PostInit()
{
    using namespace DAVA;
    using namespace PropertyPanelModuleDetail;

    DAVA::AnyCast<SelectableGroup, DAVA::Vector<DAVA::Reflection>>::Register(&CastSelectableGroupToReflection);
    UI* ui = GetUI();

    ContextAccessor* accessor = GetAccessor();
    DataContext* ctx = accessor->GetGlobalContext();
    ctx->CreateData(std::make_unique<PropertyPanelData>());

    PropertyPanelData* data = ctx->GetData<PropertyPanelData>();
    data->updater.reset(new PropertyPanelUpdater());

    DockPanelInfo panelInfo;
    panelInfo.title = QStringLiteral("Properties");
    panelInfo.actionPlacementInfo = ActionPlacementInfo(CreateMenuPoint(QList<QString>() << MenuItems::menuView
                                                                                         << "Dock"));
    PropertiesView::Params params(DAVA::mainWindowKey);
    params.accessor = accessor;
    params.invoker = GetInvoker();
    params.ui = ui;
    params.objectsField.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
    params.objectsField.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
    params.settingsNodeName = "PropertyPanel";
    params.updater = std::weak_ptr<PropertiesView::Updater>(data->updater);
#if !defined(DEPLOY_BUILD)
    params.isInDevMode = true;
#endif

    PropertiesView* view = new PropertiesView(params);
    data->view = view;

    view->RegisterExtension(std::make_shared<REModifyPropertyExtension>(accessor));
    view->RegisterExtension(std::make_shared<EntityChildCreator>());
    view->RegisterExtension(std::make_shared<EntityEditorCreator>());
    view->RegisterExtension(std::make_shared<QualitySettingsChildCreator>());
    view->RegisterExtension(std::make_shared<QualitySettingsEditorCreator>());
    view->RegisterExtension(std::make_shared<KeyedArchiveChildCreator>());
    view->RegisterExtension(std::make_shared<KeyedArchiveEditorCreator>(accessor));
    view->RegisterExtension(std::make_shared<ParticleForceCreator>());
    ui->AddView(DAVA::mainWindowKey, PanelKey(panelInfo.title, panelInfo), view);

    RegisterInterface(static_cast<PropertyPanelInterface*>(this));
}

void PropertyPanelModule::RegisterExtension(const std::shared_ptr<DAVA::ExtensionChain>& extension)
{
    using namespace DAVA;

    ContextAccessor* accessor = GetAccessor();
    DataContext* ctx = accessor->GetGlobalContext();
    PropertyPanelModuleDetail::PropertyPanelData* data = ctx->GetData<PropertyPanelModuleDetail::PropertyPanelData>();
    if (data->view.isNull() == false)
    {
        data->view->RegisterExtension(extension);
    }
}

void PropertyPanelModule::UnregisterExtension(const std::shared_ptr<DAVA::ExtensionChain>& extension)
{
    using namespace DAVA;

    ContextAccessor* accessor = GetAccessor();
    DataContext* ctx = accessor->GetGlobalContext();
    PropertyPanelModuleDetail::PropertyPanelData* data = ctx->GetData<PropertyPanelModuleDetail::PropertyPanelData>();
    if (data->view.isNull() == false)
    {
        data->view->UnregisterExtension(extension);
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(PropertyPanelModule)
{
    DAVA::ReflectionRegistrator<PropertyPanelModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_TARC_MODULE(PropertyPanelModule);
