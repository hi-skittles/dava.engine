#include "Classes/Model/PackageHierarchy/ControlNode.h"

#include "Classes/Model/ControlProperties/NameProperty.h"
#include "Classes/Model/ControlProperties/RootProperty.h"
#include "Classes/Model/PackageHierarchy/PackageNode.h"
#include "Classes/Model/PackageHierarchy/PackageVisitor.h"

#include <UI/UIControl.h>
#include <UI/UIControlPackageContext.h>
#include <Base/ObjectFactory.h>
#include <Base/RefPtr.h>

using namespace DAVA;

static const Set<String> ControlClassesWithoutChildren = { "UI3DView" };

ControlNode* GetRootControlNode(ControlNode* node)
{
    PackageBaseNode* root = node;
    while (nullptr != root && nullptr != root->GetParent() && nullptr != root->GetParent()->GetControl())
    {
        root = root->GetParent();
    }

    return dynamic_cast<ControlNode*>(root);
}

ControlNode::ControlNode(UIControl* control, bool recursively)
    : ControlsContainerNode(nullptr)
    , control(SafeRetain(control))
    , rootProperty(nullptr)
    , prototype(nullptr)
    , creationType(CREATED_FROM_CLASS)
{
    rootProperty = new RootProperty(this, nullptr);

    if (recursively)
    {
        const auto& children = control->GetChildren();
        for (const auto& child : children)
        {
            ControlNode* childNode(new ControlNode(child.Get(), recursively));
            childNode->SetParent(this);
            childNode->SetPackageContext(GetPackageContext());
            nodes.push_back(childNode);
        }
    }
}

ControlNode::ControlNode(ControlNode* node, eCreationType _creationType)
    : ControlsContainerNode(nullptr)
    , control(nullptr)
    , rootProperty(nullptr)
    , prototype(nullptr)
    , creationType(_creationType)
{
    control = ObjectFactory::Instance()->New<UIControl>(node->GetControl()->GetClassName());
    control->SetLocalPropertySet(node->GetControl()->GetLocalPropertySet());

    eCreationType childCreationType;
    if (creationType == CREATED_FROM_CLASS)
    {
        DVASSERT(false);
    }
    else
    {
        prototype = SafeRetain(node);
        prototype->AddControlToInstances(this);
        rootProperty = new RootProperty(this, node->GetRootProperty());
        childCreationType = CREATED_FROM_PROTOTYPE_CHILD;
    }

    for (ControlNode* sourceChild : node->nodes)
    {
        ScopedPtr<ControlNode> childNode(new ControlNode(sourceChild, childCreationType));
        Add(childNode);
    }

    rootProperty->Refresh(AbstractProperty::REFRESH_ALL);
}

ControlNode::~ControlNode()
{
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
        (*it)->Release();
    nodes.clear();

    SafeRelease(control);
    SafeRelease(rootProperty);

    if (prototype)
        prototype->RemoveControlFromInstances(this);
    SafeRelease(prototype);

    DVASSERT(instances.empty());
}

ControlNode* ControlNode::CreateFromControl(DAVA::UIControl* control)
{
    return new ControlNode(control, false);
}

ControlNode* ControlNode::CreateFromControlWithChildren(UIControl* control)
{
    return new ControlNode(control, true);
}

ControlNode* ControlNode::CreateFromPrototype(ControlNode* sourceNode)
{
    return new ControlNode(sourceNode, CREATED_FROM_PROTOTYPE);
}

ControlNode* ControlNode::CreateFromPrototypeChild(ControlNode* sourceNode)
{
    return new ControlNode(sourceNode, CREATED_FROM_PROTOTYPE_CHILD);
}

void ControlNode::Add(ControlNode* node)
{
    DVASSERT(node->GetParent() == nullptr);
    node->SetParent(this);
    nodes.push_back(SafeRetain(node));
    control->AddControl(node->GetControl());
    node->SetPackageContext(GetPackageContext());
}

void ControlNode::InsertAtIndex(int index, ControlNode* node)
{
    if (index >= static_cast<int>(nodes.size()))
    {
        Add(node);
    }
    else
    {
        DVASSERT(node->GetParent() == nullptr);
        node->SetParent(this);

        UIControl* belowThis = nodes[index]->GetControl();

        nodes.insert(nodes.begin() + index, SafeRetain(node));
        control->InsertChildBelow(node->GetControl(), belowThis);
        node->SetPackageContext(GetPackageContext());
    }
}

void ControlNode::Remove(ControlNode* node)
{
    auto it = find(nodes.begin(), nodes.end(), node);
    if (it != nodes.end())
    {
        DVASSERT(node->GetParent() == this);
        node->SetParent(nullptr);

        node->GetControl()->RemoveFromParent();
        node->SetPackageContext(nullptr);
        nodes.erase(it);
        SafeRelease(node);
    }
    else
    {
        DVASSERT(false);
    }
}

int ControlNode::GetCount() const
{
    return static_cast<int>(nodes.size());
}

ControlNode* ControlNode::Get(int index) const
{
    return nodes[index];
}

void ControlNode::Accept(PackageVisitor* visitor)
{
    visitor->VisitControl(this);
}

String ControlNode::GetName() const
{
    const FastName& name = GetRootProperty()->GetNameProperty()->GetValue().Cast<FastName>();
    return name.IsValid() ? name.c_str() : "";
}

UIControl* ControlNode::GetControl() const
{
    return control;
}

UIControlPackageContext* ControlNode::GetPackageContext() const
{
    return control->GetPackageContext().Get();
}

void ControlNode::SetPackageContext(UIControlPackageContext* context)
{
    control->SetPackageContext(RefPtr<UIControlPackageContext>::ConstructWithRetain(context));
    for (ControlNode* child : nodes)
        child->SetPackageContext(context);
}

ControlNode* ControlNode::GetPrototype() const
{
    return prototype;
}

const Vector<ControlNode*>& ControlNode::GetInstances() const
{
    return instances;
}

bool ControlNode::IsDependsOnPackage(PackageNode* package) const
{
    if (prototype && prototype->GetPackage() == package)
        return true;

    for (ControlNode* child : nodes)
    {
        if (child->IsDependsOnPackage(package))
            return true;
    }

    return false;
}

bool ControlNode::IsEditingSupported() const
{
    return !IsReadOnly();
}

bool ControlNode::IsInsertingControlsSupported() const
{
    return !IsReadOnly() && !ControlClassesWithoutChildren.count(control->GetClassName());
}

bool ControlNode::CanInsertControl(const ControlNode* node, DAVA::int32 pos) const
{
    if (node == nullptr || node == this || node->IsParentOf(this))
    {
        return false;
    }

    if (!IsInsertingControlsSupported())
    {
        return false;
    }

    if (pos < static_cast<int32>(nodes.size()) && nodes[pos]->GetCreationType() == CREATED_FROM_PROTOTYPE_CHILD)
    {
        return false;
    }

    const ControlNode* destNode = dynamic_cast<const ControlNode*>(this);
    for (; destNode != nullptr; destNode = dynamic_cast<const ControlNode*>(destNode->GetParent()))
    {
        if (destNode->IsInstancedFrom(node))
        {
            return false;
        }
    }

    if (node->IsDependsOnPrototype(this))
    {
        return false;
    }

    return true;
}

bool ControlNode::CanRemove() const
{
    return !IsReadOnly() && creationType != CREATED_FROM_PROTOTYPE_CHILD;
}

bool ControlNode::CanCopy() const
{
    return creationType != CREATED_FROM_PROTOTYPE_CHILD;
}

bool ControlNode::CanMoveTo(const ControlsContainerNode* container, DAVA::int32 pos) const
{
    if (!CanRemove())
    {
        return false;
    }

    if (!container->CanInsertControl(this, pos))
    {
        return false;
    }

    return true;
}

void ControlNode::RefreshProperties()
{
    rootProperty->Refresh(AbstractProperty::REFRESH_FONT | AbstractProperty::REFRESH_LOCALIZATION);
    for (ControlNode* node : nodes)
        node->RefreshProperties();
}

void ControlNode::MarkAsRemoved()
{
    if (prototype)
        prototype->RemoveControlFromInstances(this);
    for (ControlNode* node : nodes)
        node->MarkAsRemoved();
}

void ControlNode::MarkAsAlive()
{
    if (prototype)
        prototype->AddControlToInstances(this);
    for (ControlNode* node : nodes)
        node->MarkAsAlive();
}

String ControlNode::GetPathToPrototypeChild() const
{
    if (creationType == CREATED_FROM_PROTOTYPE_CHILD)
    {
        String path = GetName();
        PackageBaseNode* p = GetParent();
        while (p != nullptr && p->GetControl() != nullptr && static_cast<ControlNode*>(p)->GetCreationType() != CREATED_FROM_PROTOTYPE)
        {
            path = p->GetName() + "/" + path;
            p = p->GetParent();
        }

        return path;
    }
    return "";
}

bool ControlNode::IsInstancedFrom(const ControlNode* prototype) const
{
    for (const ControlNode* test = this; test != nullptr; test = test->GetPrototype())
    {
        if (test->GetPrototype() == prototype)
        {
            return true;
        }
    }

    return false;
}

bool ControlNode::IsDependsOnPrototype(const ControlNode* prototype) const
{
    if (IsInstancedFrom(prototype))
    {
        return true;
    }

    for (const ControlNode* child : nodes)
    {
        if (child->IsDependsOnPrototype(prototype))
            return true;
    }

    return false;
}

bool ControlNode::IsParentOf(const ControlNode* node) const
{
    for (const PackageBaseNode* parent = node->GetParent(); parent != nullptr; parent = parent->GetParent())
    {
        if (parent == this)
        {
            return true;
        }
    }

    return false;
}

void ControlNode::AddControlToInstances(ControlNode* control)
{
    auto it = std::find(instances.begin(), instances.end(), control);
    if (it == instances.end())
        instances.push_back(control);
}

void ControlNode::RemoveControlFromInstances(ControlNode* control)
{
    auto it = std::find(instances.begin(), instances.end(), control);
    if (it != instances.end())
        instances.erase(it);
}
