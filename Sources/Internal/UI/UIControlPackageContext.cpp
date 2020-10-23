#include "UI/UIControlPackageContext.h"
#include "UI/Styles/UIStyleSheet.h"

namespace DAVA
{
UIControlPackageContext::~UIControlPackageContext()
{
}

void UIControlPackageContext::AddStyleSheet(const UIPriorityStyleSheet& styleSheet)
{
    styleSheetsSorted = false;

    auto it = std::find_if(styleSheets.begin(), styleSheets.end(), [&styleSheet](UIPriorityStyleSheet& ss) {
        return ss.GetStyleSheet() == styleSheet.GetStyleSheet();
    });

    if (it == styleSheets.end())
    {
        styleSheets.push_back(styleSheet);

        maxStyleSheetHierarchyDepth = Max(maxStyleSheetHierarchyDepth, styleSheet.GetStyleSheet()->GetSelectorChain().GetSize());
    }
    else
    {
        if (styleSheet.GetPriority() < it->GetPriority())
            *it = styleSheet;
    }
}

void UIControlPackageContext::RemoveAllStyleSheets()
{
    styleSheets.clear();
    maxStyleSheetHierarchyDepth = 0;
}

const Vector<UIPriorityStyleSheet>& UIControlPackageContext::GetSortedStyleSheets()
{
    if (!styleSheetsSorted)
    {
        std::sort(styleSheets.begin(), styleSheets.end());
        styleSheetsSorted = true;
    }

    return styleSheets;
}

int32 UIControlPackageContext::GetMaxStyleSheetHierarchyDepth() const
{
    return maxStyleSheetHierarchyDepth;
}
}