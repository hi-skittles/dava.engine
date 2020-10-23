#include "UI/RichContent/UIRichContentAliasesComponent.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Utils/Utils.h"
#include "Logger/Logger.h"

#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(UIRichContentAliasesComponent)
{
    ReflectionRegistrator<UIRichContentAliasesComponent>::Begin()[M::Multiple(), M::DisplayName("Rich Content Aliases"), M::Group("Content")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIRichContentAliasesComponent* o) { o->Release(); })
    .Field("aliases", &UIRichContentAliasesComponent::GetAliasesAsString, &UIRichContentAliasesComponent::SetAliasesFromString)[M::DisplayName("Aliases")]
    .End();
}

IMPLEMENT_UI_COMPONENT(UIRichContentAliasesComponent);

UIRichContentAliasesComponent::UIRichContentAliasesComponent(const UIRichContentAliasesComponent& src)
    : UIComponent(src)
    , aliases(src.aliases)
    , modified(true)
{
}

UIRichContentAliasesComponent* UIRichContentAliasesComponent::Clone() const
{
    return new UIRichContentAliasesComponent(*this);
}

void UIRichContentAliasesComponent::SetAliases(const Aliases& _aliases)
{
    if (aliases != _aliases)
    {
        aliases = _aliases;

        aliasesAsString.clear();
        for (const auto& pair : aliases)
        {
            aliasesAsString += pair.first + "," + pair.second + ";";
        }

        modified = true;
    }
}

void UIRichContentAliasesComponent::SetModified(bool _modified)
{
    modified = _modified;
}

void UIRichContentAliasesComponent::SetAliasesFromString(const String& _aliases)
{
    aliasesAsString = _aliases;

    Aliases newAliases;
    Vector<String> tokens;
    Split(_aliases, ";", tokens);
    for (const String& token : tokens)
    {
        size_t pos = token.find(",");
        if (pos != String::npos)
        {
            String alias = token.substr(0, pos);
            String xmlSrc = token.substr(pos + 1);

            auto it = std::find_if(newAliases.begin(), newAliases.end(), [&alias](const std::pair<String, String>& a) {
                return a.first == alias;
            });
            if (it != newAliases.end())
            {
                it->second = xmlSrc;
            }
            else
            {
                newAliases.push_back({ alias, xmlSrc });
            }
        }
        else
        {
            Logger::Error("[RichAliasMap::FromString] Wrong string token '%s'!", token.c_str());
            return;
        }
    }

    SetAliases(newAliases);
}

const String& UIRichContentAliasesComponent::GetAliasesAsString() const
{
    return aliasesAsString;
}
}
