#include "PackageControlsNode.h"

#include "ControlNode.h"
#include "PackageVisitor.h"

#include "PackageNode.h"
#include "UI/UIPackage.h"
#include "UI/UIControl.h"
#include "Utils/Utils.h"

using namespace DAVA;

namespace PackageBaseNodeDetails
{
void FindNodesAtPathLevel(ControlNode* node, const Vector<String>& strPath, size_t level, Set<PackageBaseNode*>& foundNodes)
{
    if (level == strPath.size())
    {
        foundNodes.emplace(node);
    }
    else
    {
        size_t nextLevel = level + 1;
        const String& levelName = strPath[level];
        for (ControlNode* child : *node)
        {
            if (child->GetName() == levelName)
            {
                FindNodesAtPathLevel(child, strPath, nextLevel, foundNodes);
            }
        }
    }
}
}

PackageControlsNode::PackageControlsNode(PackageNode* parent_, const String& name_)
    : ControlsContainerNode(parent_)
    , name(name_)
{
}

PackageControlsNode::~PackageControlsNode()
{
    for (ControlNode* node : nodes)
        node->Release();
    nodes.clear();
}

void PackageControlsNode::Add(ControlNode* node)
{
    DVASSERT(node->GetParent() == nullptr);
    DVASSERT(node->GetPackageContext() == nullptr);
    node->SetParent(this);
    node->SetPackageContext(GetPackage()->GetContext());
    nodes.push_back(SafeRetain(node));
}

void PackageControlsNode::InsertAtIndex(int index, ControlNode* node)
{
    DVASSERT(node->GetParent() == nullptr);
    DVASSERT(node->GetPackageContext() == nullptr);
    node->SetParent(this);
    node->SetPackageContext(GetPackage()->GetContext());
    if (index < nodes.size())
    {
        nodes.insert(nodes.begin() + index, SafeRetain(node));
    }
    else
    {
        nodes.push_back(SafeRetain(node));
    }
}

void PackageControlsNode::Remove(ControlNode* node)
{
    auto it = find(nodes.begin(), nodes.end(), node);
    if (it != nodes.end())
    {
        DVASSERT(node->GetParent() == this);
        node->SetParent(nullptr);

        DVASSERT(node->GetPackageContext() == GetPackage()->GetContext());
        node->SetPackageContext(nullptr);

        nodes.erase(it);
        SafeRelease(node);
    }
    else
    {
        DVASSERT(false);
    }
}

int PackageControlsNode::GetCount() const
{
    return static_cast<int>(nodes.size());
}

ControlNode* PackageControlsNode::Get(int index) const
{
    return nodes[index];
}

void PackageControlsNode::Accept(PackageVisitor* visitor)
{
    visitor->VisitControls(this);
}

String PackageControlsNode::GetName() const
{
    return name;
}

bool PackageControlsNode::IsEditingSupported() const
{
    return false;
}

bool PackageControlsNode::IsInsertingControlsSupported() const
{
    return !IsReadOnly();
}

bool PackageControlsNode::CanInsertControl(const ControlNode* node, DAVA::int32 pos) const
{
    return !IsReadOnly();
}

bool PackageControlsNode::CanRemove() const
{
    return false;
}

bool PackageControlsNode::CanCopy() const
{
    return false;
}

void PackageControlsNode::RefreshControlProperties()
{
    for (ControlNode* node : nodes)
        node->RefreshProperties();
}

void PackageControlsNode::FindAllNodesByPath(const DAVA::String& path, Set<PackageBaseNode*>& foundNodes) const
{
    Vector<String> strPath;
    Split(path, "/", strPath, false, true);

    if (!strPath.empty())
    {
        for (ControlNode* child : nodes)
        {
            if (child->GetName() == strPath[0])
            {
                PackageBaseNodeDetails::FindNodesAtPathLevel(child, strPath, 1, foundNodes);
            }
        }
    }
}
