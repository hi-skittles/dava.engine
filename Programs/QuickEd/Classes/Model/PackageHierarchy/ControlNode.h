#ifndef __UI_EDITOR_CONTROL_NODE__
#define __UI_EDITOR_CONTROL_NODE__

#include "PackageBaseNode.h"
#include "ControlsContainerNode.h"

#include <Base/Result.h>

namespace DAVA
{
class UIControlPackageContext;
}

class PackageNode;
class RootProperty;

class ControlNode : public ControlsContainerNode
{
public:
    enum eCreationType
    {
        CREATED_FROM_CLASS,
        CREATED_FROM_PROTOTYPE,
        CREATED_FROM_PROTOTYPE_CHILD
    };

private:
    ControlNode(DAVA::UIControl* control, bool recursively);
    ControlNode(ControlNode* node, eCreationType creationType);
    virtual ~ControlNode();

public:
    static ControlNode* CreateFromControl(DAVA::UIControl* control);
    static ControlNode* CreateFromControlWithChildren(DAVA::UIControl* control);
    static ControlNode* CreateFromPrototype(ControlNode* sourceNode);
    static ControlNode* CreateFromPrototypeChild(ControlNode* sourceNode);

public:
    // ControlsContainerNode
    void Add(ControlNode* node) override;
    void InsertAtIndex(int index, ControlNode* node) override;
    void Remove(ControlNode* node) override;

    int GetCount() const override;
    ControlNode* Get(int index) const override;
    void Accept(PackageVisitor* visitor) override;

    virtual DAVA::String GetName() const override;
    DAVA::UIControl* GetControl() const override;
    DAVA::UIControlPackageContext* GetPackageContext() const;
    void SetPackageContext(DAVA::UIControlPackageContext* context);

    ControlNode* GetPrototype() const;
    const DAVA::Vector<ControlNode*>& GetInstances() const;
    bool IsDependsOnPackage(PackageNode* package) const;

    virtual bool IsEditingSupported() const override;
    virtual bool IsInsertingControlsSupported() const override;
    virtual bool CanInsertControl(const ControlNode* node, DAVA::int32 pos) const override;
    virtual bool CanRemove() const override;
    virtual bool CanCopy() const override;
    bool CanMoveTo(const ControlsContainerNode* node, DAVA::int32 pos) const;

    bool IsParentOf(const ControlNode* node) const;

    eCreationType GetCreationType() const
    {
        return creationType;
    }

    RootProperty* GetRootProperty() const
    {
        return rootProperty;
    }
    void RefreshProperties();

    void MarkAsRemoved();
    void MarkAsAlive();

    DAVA::String GetPathToPrototypeChild() const;

private:
    bool IsInstancedFrom(const ControlNode* prototype) const;
    bool IsDependsOnPrototype(const ControlNode* prototype) const;
    void AddControlToInstances(ControlNode* control);
    void RemoveControlFromInstances(ControlNode* control);

private:
    DAVA::UIControl* control;
    RootProperty* rootProperty;

    ControlNode* prototype;
    DAVA::Vector<ControlNode*> instances; // weak

    eCreationType creationType;
};

extern ControlNode* GetRootControlNode(ControlNode* node);

#endif // __UI_EDITOR_CONTROL_NODE__
