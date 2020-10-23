#include "QECommands/SetGuidesCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"

using namespace DAVA;

SetGuidesCommand::SetGuidesCommand(PackageNode* package, const DAVA::String& controlName_, DAVA::Vector2::eAxis orientation_, const PackageNode::AxisGuides& guides_)
    : QEPackageCommand(package, "Set guides")
    , controlName(controlName_)
    , orientation(orientation_)
    , guides(guides_)
    , oldGuides(package->GetAxisGuides(controlName, orientation))
{
}

void SetGuidesCommand::Redo()
{
    package->SetAxisGuides(controlName, orientation, guides);
}

void SetGuidesCommand::Undo()
{
    package->SetAxisGuides(controlName, orientation, oldGuides);
}
