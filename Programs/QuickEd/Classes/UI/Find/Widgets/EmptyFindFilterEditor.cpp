#include "UI/Find/Widgets/EmptyFindFilterEditor.h"
#include "UI/Find/Filters/FindFilter.h"

using namespace DAVA;

EmptyFindFilterEditor::EmptyFindFilterEditor(QWidget* parent, const FindFilterBuilder& findFilterBuilder_)
    : FindFilterEditor(parent)
    , findFilterBuilder(findFilterBuilder_)
{
}

std::unique_ptr<FindFilter> EmptyFindFilterEditor::BuildFindFilter()
{
    return findFilterBuilder();
}
