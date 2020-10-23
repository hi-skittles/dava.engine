#pragma once

#include "Base/Vector.h"
#include "Base/RefPtr.h"

namespace DAVA
{
class UIControl;
class UIRichContentAliasesComponent;
class UIRichContentComponent;

struct RichContentAlias final
{
    String alias;
    String tag;
    Map<String, String> attributes;
};

struct RichContentAliasesLink final
{
    UIRichContentAliasesComponent* component = nullptr;
    Vector<RichContentAlias> aliases;

    void PutAlias(const RichContentAlias& alias);
    const RichContentAlias& GetAlias(const String& alias) const;
    void RemoveAll();
};

struct RichContentLink final
{
    UIControl* control = nullptr;
    UIRichContentComponent* component = nullptr;
    Vector<RichContentAliasesLink> aliasesLinks;
    Vector<RefPtr<UIControl>> richItems;

    void AddItem(const RefPtr<UIControl>& item);
    void RemoveItems();
    void AddAliases(UIRichContentAliasesComponent* component);
    void RemoveAliases(UIRichContentAliasesComponent* component);
};
}
