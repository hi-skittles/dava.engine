#include "UI/UIListCell.h"
#include "Base/ObjectFactory.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIListCell)
{
    ReflectionRegistrator<UIListCell>::Begin()[M::DisplayName("List Cell")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIListCell* o) { o->Release(); })
    .Field("identifier", &UIListCell::GetIdentifier, &UIListCell::SetIdentifier)[M::DisplayName("Identifier")]
    .End();
}

UIListCell::UIListCell(const Rect& rect, const String& cellIdentifier)
    : UIControl(rect)
    , currentIndex(-1)
    , identifier(cellIdentifier)
    , cellStore(NULL)
{
}

UIListCell::~UIListCell()
{
}

const String& UIListCell::GetIdentifier() const
{
    return identifier;
}

void UIListCell::SetIdentifier(const String& newIdentifier)
{
    identifier = newIdentifier;
}

int32 UIListCell::GetIndex() const
{
    return currentIndex;
}

UIListCell* UIListCell::Clone()
{
    UIListCell* c = new UIListCell(GetRect(), identifier);
    c->CopyDataFrom(this);
    return c;
}

void UIListCell::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);
    UIListCell* srcListCell = static_cast<UIListCell*>(srcControl);
    identifier = srcListCell->identifier;
}
};
