#include "UI/UIListDelegate.h"

namespace DAVA
{
float32 UIListDelegate::CellWidth(UIList* /*list*/, int32 /*index*/)
{
    return 20.0f;
};

float32 UIListDelegate::CellHeight(UIList* /*list*/, int32 /*index*/)
{
    return 20.0f;
};

void UIListDelegate::OnCellSelected(UIList* /*forList*/, UIListCell* /*selectedCell*/)
{
};

void UIListDelegate::SaveToYaml(UIList* /*forList*/, YamlNode* /*node*/)
{
};
}
