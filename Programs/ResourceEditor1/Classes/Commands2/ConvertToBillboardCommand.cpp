#include "Commands2/RECommandIDs.h"
#include "Commands2/ConvertToBillboardCommand.h"
#include "Render/Highlevel/BillboardRenderObject.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Base/ScopedPtr.h"

ConvertToBillboardCommand::ConvertToBillboardCommand(DAVA::RenderObject* ro, DAVA::Entity* entity_)
    : RECommand(CMDID_CONVERT_TO_BILLBOARD, "Convert to billboard")
    , entity(entity_)
    , oldRenderComponent(DAVA::GetRenderComponent(entity))
    , newRenderComponent(new DAVA::RenderComponent())
{
    DAVA::ScopedPtr<DAVA::RenderObject> newRenderObject(new DAVA::BillboardRenderObject());
    oldRenderComponent->GetRenderObject()->Clone(newRenderObject);
    newRenderObject->AddFlag(DAVA::RenderObject::eFlags::CUSTOM_PREPARE_TO_RENDER);
    newRenderObject->RecalcBoundingBox();

    newRenderComponent->SetRenderObject(newRenderObject);

    detachedComponent = newRenderComponent;
}

ConvertToBillboardCommand::~ConvertToBillboardCommand()
{
    DVASSERT(detachedComponent->GetEntity() == nullptr);
    DAVA::SafeDelete(detachedComponent);
}

void ConvertToBillboardCommand::Redo()
{
    entity->DetachComponent(oldRenderComponent);
    entity->AddComponent(newRenderComponent);

    entity->GetScene()->GetRenderSystem()->MarkForUpdate(newRenderComponent->GetRenderObject());

    detachedComponent = oldRenderComponent;
}

void ConvertToBillboardCommand::Undo()
{
    entity->DetachComponent(newRenderComponent);
    entity->AddComponent(oldRenderComponent);

    detachedComponent = newRenderComponent;
}
