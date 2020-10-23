#ifndef __UI_EDITOR_STYLE_SHEET_NODE_H__
#define __UI_EDITOR_STYLE_SHEET_NODE_H__

#include "PackageBaseNode.h"
#include "UI/Styles/UIStyleSheet.h"
#include "UI/Styles/UIStyleSheetSelectorChain.h"

namespace DAVA
{
class UIStyleSheet;
}

class StyleSheetRootProperty;

class StyleSheetNode : public PackageBaseNode
{
public:
    StyleSheetNode(const DAVA::UIStyleSheetSourceInfo& sourceInfo, const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties);

protected:
    virtual ~StyleSheetNode();

public:
    StyleSheetNode* Clone() const;

    int GetCount() const override;
    PackageBaseNode* Get(DAVA::int32 index) const override;
    void Accept(PackageVisitor* visitor) override;

    DAVA::String GetName() const override;
    void UpdateName();

    bool CanRemove() const override;
    bool CanCopy() const override;

    StyleSheetRootProperty* GetRootProperty() const;

private:
    DAVA::UIStyleSheetSourceInfo sourceInfo;
    StyleSheetRootProperty* rootProperty;
    DAVA::String name;
};

#endif //__UI_EDITOR_STYLE_SHEET_NODE_H__
