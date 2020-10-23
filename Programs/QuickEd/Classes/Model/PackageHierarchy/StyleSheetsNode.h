#ifndef __QUICKED_STYLE_SHEETS_NODE_H__
#define __QUICKED_STYLE_SHEETS_NODE_H__

#include "StyleSheetNode.h"

class StyleSheetsNode : public PackageBaseNode
{
public:
    StyleSheetsNode(PackageBaseNode* parent);
    virtual ~StyleSheetsNode();

    void Add(StyleSheetNode* node);
    void InsertAtIndex(DAVA::int32 index, StyleSheetNode* node);
    void Remove(StyleSheetNode* node);
    int GetCount() const override;
    StyleSheetNode* Get(DAVA::int32 index) const override;
    void Accept(PackageVisitor* visitor) override;

    DAVA::String GetName() const override;

    bool IsInsertingStylesSupported() const override;
    bool CanInsertStyle(StyleSheetNode* node, DAVA::int32 pos) const override;

    DAVA::Vector<StyleSheetNode*>::const_iterator begin() const;
    DAVA::Vector<StyleSheetNode*>::const_iterator end() const;

    DAVA::Vector<StyleSheetNode*>::iterator begin();
    DAVA::Vector<StyleSheetNode*>::iterator end();

private:
    DAVA::Vector<StyleSheetNode*> styleSheets;
};

#endif //__QUICKED_STYLE_SHEETS_NODE_H__
