#include "StyleSheetsNode.h"

#include "PackageVisitor.h"

using namespace DAVA;

StyleSheetsNode::StyleSheetsNode(PackageBaseNode* parent)
    : PackageBaseNode(parent)
{
}

StyleSheetsNode::~StyleSheetsNode()
{
    for (StyleSheetNode* styleSheet : styleSheets)
        styleSheet->Release();
    styleSheets.clear();
}

void StyleSheetsNode::Add(StyleSheetNode* node)
{
    DVASSERT(node->GetParent() == NULL);
    node->SetParent(this);
    styleSheets.push_back(SafeRetain(node));
}

void StyleSheetsNode::InsertAtIndex(DAVA::int32 index, StyleSheetNode* node)
{
    DVASSERT(node->GetParent() == NULL);
    node->SetParent(this);

    styleSheets.insert(styleSheets.begin() + index, SafeRetain(node));
}

void StyleSheetsNode::Remove(StyleSheetNode* node)
{
    auto it = find(styleSheets.begin(), styleSheets.end(), node);
    if (it != styleSheets.end())
    {
        DVASSERT(node->GetParent() == this);
        node->SetParent(NULL);

        styleSheets.erase(it);
        SafeRelease(node);
    }
    else
    {
        DVASSERT(false);
    }
}

int StyleSheetsNode::GetCount() const
{
    return static_cast<int>(styleSheets.size());
}

StyleSheetNode* StyleSheetsNode::Get(int index) const
{
    return styleSheets[index];
}

void StyleSheetsNode::Accept(PackageVisitor* visitor)
{
    visitor->VisitStyleSheets(this);
}

String StyleSheetsNode::GetName() const
{
    return "Style Sheets";
}

bool StyleSheetsNode::IsInsertingStylesSupported() const
{
    return true;
}

bool StyleSheetsNode::CanInsertStyle(StyleSheetNode* node, DAVA::int32 pos) const
{
    return !IsReadOnly();
}

DAVA::Vector<StyleSheetNode*>::const_iterator StyleSheetsNode::begin() const
{
    return styleSheets.begin();
}

DAVA::Vector<StyleSheetNode*>::const_iterator StyleSheetsNode::end() const
{
    return styleSheets.end();
}

DAVA::Vector<StyleSheetNode*>::iterator StyleSheetsNode::begin()
{
    return styleSheets.begin();
}

DAVA::Vector<StyleSheetNode*>::iterator StyleSheetsNode::end()
{
    return styleSheets.end();
}
