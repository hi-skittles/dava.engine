#ifndef __UI_EDITOR_IMPORTED_PACKAGES_NODE_H__
#define __UI_EDITOR_IMPORTED_PACKAGES_NODE_H__

#include "PackageBaseNode.h"

class PackageNode;
class PackageControlsNode;

class ImportedPackagesNode : public PackageBaseNode
{
public:
    ImportedPackagesNode(PackageBaseNode* parent);
    virtual ~ImportedPackagesNode();

    void Add(PackageNode* node);
    void InsertAtIndex(DAVA::int32 index, PackageNode* node);
    void Remove(PackageNode* node);
    PackageNode* GetImportedPackage(DAVA::int32 index) const;
    int GetCount() const override;
    PackageBaseNode* Get(DAVA::int32 index) const override;
    void Accept(PackageVisitor* visitor) override;

    virtual DAVA::String GetName() const override;

    bool IsInsertingPackagesSupported() const override;
    bool CanInsertImportedPackage(PackageNode* package) const override;

    PackageNode* FindPackageByName(const DAVA::String& name) const;

    DAVA::Vector<PackageNode*>::const_iterator begin() const;
    DAVA::Vector<PackageNode*>::const_iterator end() const;

    DAVA::Vector<PackageNode*>::iterator begin();
    DAVA::Vector<PackageNode*>::iterator end();

protected:
    virtual bool IsReadOnly() const override;

private:
    DAVA::Vector<PackageNode*> packages;
};

#endif //__UI_EDITOR_IMPORTED_PACKAGES_NODE_H__
