#include "UI/RichContent/Private/RichStructs.h"
#include "UI/RichContent/UIRichContentAliasesComponent.h"
#include "UI/RichContent/UIRichContentComponent.h"
#include "UI/UIControl.h"

namespace DAVA
{
void RichContentAliasesLink::PutAlias(const RichContentAlias& alias)
{
    aliases.push_back(alias);
}

const RichContentAlias& RichContentAliasesLink::GetAlias(const String& alias) const
{
    auto it = std::find_if(aliases.begin(), aliases.end(), [&alias](const RichContentAlias& a) { return a.alias == alias; });
    if (it != aliases.end())
    {
        return *it;
    }
    static const RichContentAlias EMPTY_ALIAS = {};
    return EMPTY_ALIAS;
}

void RichContentAliasesLink::RemoveAll()
{
    aliases.clear();
}

void RichContentLink::AddItem(const RefPtr<UIControl>& item)
{
    richItems.push_back(item);
}

void RichContentLink::RemoveItems()
{
    if (component != nullptr)
    {
        UIControl* ctrl = component->GetControl();
        if (ctrl != nullptr)
        {
            for (const RefPtr<UIControl>& item : richItems)
            {
                ctrl->RemoveControl(item.Get());
            }
        }
    }
    richItems.clear();
}

void RichContentLink::AddAliases(UIRichContentAliasesComponent* component)
{
    auto it = std::find_if(aliasesLinks.begin(), aliasesLinks.end(), [component](const RichContentAliasesLink& a) {
        return a.component == component;
    });
    if (it == aliasesLinks.end())
    {
        RichContentAliasesLink a;
        a.component = component;
        aliasesLinks.push_back(a);
    }
}

void RichContentLink::RemoveAliases(UIRichContentAliasesComponent* component)
{
    auto it = std::find_if(aliasesLinks.begin(), aliasesLinks.end(), [component](const RichContentAliasesLink& a) {
        return a.component == component;
    });
    if (it != aliasesLinks.end())
    {
        aliasesLinks.erase(it);
    }
}
}
