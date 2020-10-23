#include "Utils/PackageListenerProxy.h"

#include "Modules/DocumentsModule/DocumentData.h"

#include "Model/PackageHierarchy/PackageNode.h"

#include <TArc/Core/FieldBinder.h>

#include <Reflection/ReflectedTypeDB.h>

PackageListenerProxy::PackageListenerProxy() = default;

PackageListenerProxy::PackageListenerProxy(PackageListener* listener, DAVA::ContextAccessor* accessor)
{
    Init(listener, accessor);
}

PackageListenerProxy::~PackageListenerProxy() = default;

void PackageListenerProxy::Init(PackageListener* listener_, DAVA::ContextAccessor* accessor)
{
    using namespace DAVA;

    listener = listener_;

    fieldBinder.reset(new FieldBinder(accessor));
    FieldDescriptor fieldDescr;
    fieldDescr.type = ReflectedTypeDB::Get<DocumentData>();
    fieldDescr.fieldName = FastName(DocumentData::packagePropertyName);
    fieldBinder->BindField(fieldDescr, MakeFunction(this, &PackageListenerProxy::OnPackageChanged));
}

void PackageListenerProxy::OnPackageChanged(const DAVA::Any& packageValue)
{
    if (package)
    {
        package->RemoveListener(this);
    }

    if (packageValue.CanGet<PackageNode*>())
    {
        package = DAVA::RefPtr<PackageNode>::ConstructWithRetain(packageValue.Get<PackageNode*>());
        package->AddListener(this);
    }
    else
    {
        package = nullptr;
    }
    ActivePackageNodeWasChanged(package.Get());
}

void PackageListenerProxy::ActivePackageNodeWasChanged(PackageNode* node)
{
    listener->ActivePackageNodeWasChanged(node);
}

void PackageListenerProxy::ControlPropertyWasChanged(ControlNode* node, AbstractProperty* property)
{
    listener->ControlPropertyWasChanged(node, property);
}

void PackageListenerProxy::StylePropertyWasChanged(StyleSheetNode* node, AbstractProperty* property)
{
    listener->StylePropertyWasChanged(node, property);
}

void PackageListenerProxy::ControlComponentWasAdded(ControlNode* node, ComponentPropertiesSection* section)
{
    listener->ControlComponentWasAdded(node, section);
}

void PackageListenerProxy::ControlComponentWasRemoved(ControlNode* node, ComponentPropertiesSection* section)
{
    listener->ControlComponentWasRemoved(node, section);
}

void PackageListenerProxy::ControlWillBeAdded(ControlNode* node, ControlsContainerNode* destination, int index)
{
    listener->ControlWillBeAdded(node, destination, index);
}

void PackageListenerProxy::ControlWasAdded(ControlNode* node, ControlsContainerNode* destination, int index)
{
    listener->ControlWasAdded(node, destination, index);
}

void PackageListenerProxy::ControlWillBeRemoved(ControlNode* node, ControlsContainerNode* from)
{
    listener->ControlWillBeRemoved(node, from);
}

void PackageListenerProxy::ControlWasRemoved(ControlNode* node, ControlsContainerNode* from)
{
    listener->ControlWasRemoved(node, from);
}

void PackageListenerProxy::StyleWillBeAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index)
{
    listener->StyleWillBeAdded(node, destination, index);
}

void PackageListenerProxy::StyleWasAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index)
{
    listener->StyleWasAdded(node, destination, index);
}

void PackageListenerProxy::StyleWillBeRemoved(StyleSheetNode* node, StyleSheetsNode* from)
{
    listener->StyleWillBeRemoved(node, from);
}

void PackageListenerProxy::StyleWasRemoved(StyleSheetNode* node, StyleSheetsNode* from)
{
    listener->StyleWasRemoved(node, from);
}

void PackageListenerProxy::ImportedPackageWillBeAdded(PackageNode* node, ImportedPackagesNode* to, int index)
{
    listener->ImportedPackageWillBeAdded(node, to, index);
}

void PackageListenerProxy::ImportedPackageWasAdded(PackageNode* node, ImportedPackagesNode* to, int index)
{
    listener->ImportedPackageWasAdded(node, to, index);
}

void PackageListenerProxy::ImportedPackageWillBeRemoved(PackageNode* node, ImportedPackagesNode* from)
{
    listener->ImportedPackageWillBeRemoved(node, from);
}

void PackageListenerProxy::ImportedPackageWasRemoved(PackageNode* node, ImportedPackagesNode* from)
{
    listener->ImportedPackageWasRemoved(node, from);
}

void PackageListenerProxy::StyleSheetsWereRebuilt()
{
    listener->StyleSheetsWereRebuilt();
}
