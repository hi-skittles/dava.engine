#ifndef __UI_EDITOR_UI_PACKAGE_MODEL_NODE__
#define __UI_EDITOR_UI_PACKAGE_MODEL_NODE__

#include "Base/BaseObject.h"

#include <functional>
#include <Base/Result.h>

namespace DAVA
{
class UIControl;
}

class ControlNode;
class StyleSheetNode;
class PackageNode;
class PackageVisitor;

class PackageBaseNode : public DAVA::BaseObject
{
public:
    PackageBaseNode(PackageBaseNode* parent);

protected:
    virtual ~PackageBaseNode();

public:
    virtual int GetCount() const = 0;
    virtual PackageBaseNode* Get(int index) const = 0;
    int GetIndex(const PackageBaseNode* node) const;

    PackageBaseNode* GetParent() const;
    void SetParent(PackageBaseNode* parent);

    virtual void Accept(PackageVisitor* visitor) = 0;

    virtual DAVA::String GetName() const;
    virtual PackageNode* GetPackage();
    virtual const PackageNode* GetPackage() const;

    virtual DAVA::UIControl* GetControl() const;

    virtual void debugDump(int depth);

    virtual bool IsEditingSupported() const;
    virtual bool IsInsertingControlsSupported() const;
    virtual bool IsInsertingPackagesSupported() const;
    virtual bool IsInsertingStylesSupported() const;
    virtual bool CanInsertControl(const ControlNode* node, DAVA::int32 pos) const;
    virtual bool CanInsertStyle(StyleSheetNode* node, DAVA::int32 pos) const;
    virtual bool CanInsertImportedPackage(PackageNode* package) const;
    virtual bool CanRemove() const;
    virtual bool CanCopy() const;
    virtual bool IsReadOnly() const;

    void AddResult(const DAVA::Result& r);
    const DAVA::ResultList& GetResults() const;
    void AddIssue(DAVA::int32 issueId);
    void RemoveIssue(DAVA::int32 issueId);
    void RemoveAllIssues();
    bool HasErrors() const;

private:
    PackageBaseNode* parent;

    DAVA::ResultList results;
    DAVA::UnorderedSet<DAVA::int32> issues;
};

bool IsControlNode(PackageBaseNode* node);
bool IsRootControl(PackageBaseNode* node);

//comparator to sort nodes by their hierarchy
bool CompareByLCA(const PackageBaseNode* left, const PackageBaseNode* right);
using LCAComparator = std::function<bool(const PackageBaseNode*, const PackageBaseNode*)>;

//automatically sort all nodes by their hierarchy using CompareByLCA function
using SortedPackageBaseNodeSet = DAVA::Set<PackageBaseNode*, std::function<bool(const PackageBaseNode*, const PackageBaseNode*)>>;
using SortedControlNodeSet = DAVA::Set<ControlNode*, std::function<bool(const PackageBaseNode*, const PackageBaseNode*)>>;

#endif // __UI_EDITOR_UI_PACKAGE_MODEL_NODE__
