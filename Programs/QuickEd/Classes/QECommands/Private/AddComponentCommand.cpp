#include "QECommands/AddComponentCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/ControlProperties/ComponentPropertiesSection.h"
#include <UI/Components/UIComponent.h>

using namespace DAVA;

AddComponentCommand::AddComponentCommand(PackageNode* package, ControlNode* node_, ComponentPropertiesSection* section_)
    : QEPackageCommand(package, "Add component")
    , node(RefPtr<ControlNode>::ConstructWithRetain(node_))
    , section(RefPtr<ComponentPropertiesSection>::ConstructWithRetain(section_))
{
}

void AddComponentCommand::Redo()
{
    package->AddComponent(node.Get(), section.Get());
}

void AddComponentCommand::Undo()
{
    package->RemoveComponent(node.Get(), section.Get());
}
