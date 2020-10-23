#include "Classes/PropertyPanel/ComponentExtensions.h"
#include "Classes/Commands2/RemoveComponentCommand.h"
#include "Classes/PropertyPanel/Private/ActionComponentEditor.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"

#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Utils/Utils.h>

#include <Entity/Component.h>
#include <Base/TemplateHelpers.h>
#include "SoundComponentEditor/SoundComponentEditor.h"

namespace ComponentExtensionsDetail
{
class RemoveComponentProducer : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::TArc::PropertyNode>& node) const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.icon = DAVA::TArc::SharedIcon(":/QtIcons/remove.png");
        info.tooltip = "Remove Component";
        info.description = "Remove Component";

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const Params& params) const override
    {
        DAVA::Component* component = node->field.ref.GetValueObject().GetPtr<DAVA::Component>();
        DAVA::Entity* entity = component->GetEntity();
        return std::unique_ptr<DAVA::Command>(new RemoveComponentCommand(entity, component));
    }
};

class ActionsEditProducer : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::TArc::PropertyNode>& node) const override
    {
        return true;
    }

    bool OnlyForSingleSelection() const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.icon = DAVA::TArc::SharedIcon(":/QtIcons/settings.png");
        info.tooltip = "Edit action component";
        info.description = "Edit action component";

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const Params& params) const override
    {
        using namespace DAVA;
        using namespace TArc;

        Component* component = node->field.ref.GetValueObject().GetPtr<Component>();
        ActionComponent* actionComponent = DynamicTypeCheck<ActionComponent*>(component);
        Entity* entity = component->GetEntity();

        ActionComponentEditor editor(params.ui->GetWindow(DAVA::TArc::mainWindowKey));
        editor.SetComponent(actionComponent);
        editor.exec();

        if (editor.IsModified())
        {
            DataContext* activeContext = params.accessor->GetActiveContext();
            DVASSERT(activeContext != nullptr);
            RefPtr<SceneEditor2> curScene = activeContext->GetData<SceneData>()->GetScene();
            curScene->SetChanged();
        }

        return std::unique_ptr<DAVA::Command>();
    }
};

class SoundsEditProducer : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::TArc::PropertyNode>& node) const override
    {
        return true;
    }

    bool OnlyForSingleSelection() const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.icon = DAVA::TArc::SharedIcon(":/QtIcons/settings.png");
        info.tooltip = "Edit sound component";
        info.description = "Edit sound component";

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const Params& params) const override
    {
        using namespace DAVA;
        using namespace TArc;

        Component* component = node->field.ref.GetValueObject().GetPtr<Component>();
        SoundComponent* soundComponent = DynamicTypeCheck<SoundComponent*>(component);
        Entity* entity = component->GetEntity();

        DataContext* ctx = params.accessor->GetActiveContext();
        DVASSERT(ctx != nullptr);

        SoundComponentEditor editor(ctx->GetData<SceneData>()->GetScene().Get(), params.ui->GetWindow(DAVA::TArc::mainWindowKey));
        editor.SetEditableEntity(entity);
        editor.exec();

        return std::unique_ptr<DAVA::Command>();
    }
};

class TriggerWaveProducer : public DAVA::M::CommandProducer
{
public:
    bool IsApplyable(const std::shared_ptr<DAVA::TArc::PropertyNode>& node) const override
    {
        return true;
    }

    bool OnlyForSingleSelection() const override
    {
        return true;
    }

    Info GetInfo() const override
    {
        Info info;
        info.icon = DAVA::TArc::SharedIcon(":/QtIcons/clone.png");
        info.tooltip = "Trigger Wave";
        info.description = "";

        return info;
    }

    std::unique_ptr<DAVA::Command> CreateCommand(const std::shared_ptr<DAVA::TArc::PropertyNode>& node, const Params& params) const override
    {
        using namespace DAVA;
        using namespace TArc;

        Component* component = node->field.ref.GetValueObject().GetPtr<Component>();
        WaveComponent* waveComponent = DynamicTypeCheck<WaveComponent*>(component);
        waveComponent->Trigger();

        return std::unique_ptr<DAVA::Command>();
    }
};
} // namespace ComponentExtensionsDetail

std::shared_ptr<DAVA::M::CommandProducer> CreateRemoveComponentProducer()
{
    return std::shared_ptr<DAVA::M::CommandProducer>(new ComponentExtensionsDetail::RemoveComponentProducer());
}

std::shared_ptr<DAVA::M::CommandProducer> CreateActionsEditProducer()
{
    return std::shared_ptr<DAVA::M::CommandProducer>(new ComponentExtensionsDetail::ActionsEditProducer());
}

std::shared_ptr<DAVA::M::CommandProducer> CreateSoundsEditProducer()
{
    return std::shared_ptr<DAVA::M::CommandProducer>(new ComponentExtensionsDetail::SoundsEditProducer());
}

std::shared_ptr<DAVA::M::CommandProducer> CreateWaveTriggerProducer()
{
    return std::shared_ptr<DAVA::M::CommandProducer>(new ComponentExtensionsDetail::TriggerWaveProducer());
}
