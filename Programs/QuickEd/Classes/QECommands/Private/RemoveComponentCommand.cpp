#include "QECommands/RemoveComponentCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/ControlProperties/ComponentPropertiesSection.h"
#include "UI/Components/UIComponent.h"

using namespace DAVA;

RemoveComponentCommand::RemoveComponentCommand(PackageNode* package, ControlNode* node_, ComponentPropertiesSection* section_)
    : QEPackageCommand(package, "Remove Component")
    , node(RefPtr<ControlNode>::ConstructWithRetain(node_))
    , componentSection(RefPtr<ComponentPropertiesSection>::ConstructWithRetain(section_))
{
}

void RemoveComponentCommand::Redo()
{
    package->RemoveComponent(node.Get(), componentSection.Get());
}

void RemoveComponentCommand::Undo()
{
    package->AddComponent(node.Get(), componentSection.Get());
}
