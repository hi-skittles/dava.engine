#include "ImportedPackagesNode.h"

#include "PackageControlsNode.h"
#include "PackageNode.h"
#include "PackageVisitor.h"

using namespace DAVA;

ImportedPackagesNode::ImportedPackagesNode(PackageBaseNode* parent)
    : PackageBaseNode(parent)
{
}

ImportedPackagesNode::~ImportedPackagesNode()
{
    for (PackageNode* package : packages)
        package->Release();
    packages.clear();
}

void ImportedPackagesNode::Add(PackageNode* node)
{
    DVASSERT(node->GetParent() == NULL);
    node->SetParent(this);
    packages.push_back(SafeRetain(node));
}

void ImportedPackagesNode::InsertAtIndex(DAVA::int32 index, PackageNode* node)
{
    DVASSERT(node->GetParent() == NULL);
    node->SetParent(this);

    packages.insert(packages.begin() + index, SafeRetain(node));
}

void ImportedPackagesNode::Remove(PackageNode* node)
{
    auto it = find(packages.begin(), packages.end(), node);
    if (it != packages.end())
    {
        DVASSERT(node->GetParent() == this);
        node->SetParent(NULL);

        packages.erase(it);
        SafeRelease(node);
    }
    else
    {
        DVASSERT(false);
    }
}

PackageNode* ImportedPackagesNode::GetImportedPackage(DAVA::int32 index) const
{
    return packages[index];
}

int ImportedPackagesNode::GetCount() const
{
    return static_cast<int>(packages.size());
}

PackageBaseNode* ImportedPackagesNode::Get(int index) const
{
    return packages[index];
}

void ImportedPackagesNode::Accept(PackageVisitor* visitor)
{
    visitor->VisitImportedPackages(this);
}

String ImportedPackagesNode::GetName() const
{
    return "Imported Packages";
}

bool ImportedPackagesNode::IsInsertingPackagesSupported() const
{
    return !IsReadOnly();
}

bool ImportedPackagesNode::CanInsertImportedPackage(PackageNode* package) const
{
    if (package->FindPackageInImportedPackagesRecursively(GetPackage()))
        return false;
    return true;
}

PackageNode* ImportedPackagesNode::FindPackageByName(const DAVA::String& name) const
{
    for (PackageNode* node : packages)
    {
        if (node->GetName() == name)
            return node;
    }
    return nullptr;
}

bool ImportedPackagesNode::IsReadOnly() const
{
    return GetParent() != nullptr ? GetParent()->IsReadOnly() : true;
}

DAVA::Vector<PackageNode*>::const_iterator ImportedPackagesNode::begin() const
{
    return packages.begin();
}

DAVA::Vector<PackageNode*>::const_iterator ImportedPackagesNode::end() const
{
    return packages.end();
}

DAVA::Vector<PackageNode*>::iterator ImportedPackagesNode::begin()
{
    return packages.begin();
}

DAVA::Vector<PackageNode*>::iterator ImportedPackagesNode::end()
{
    return packages.end();
}
