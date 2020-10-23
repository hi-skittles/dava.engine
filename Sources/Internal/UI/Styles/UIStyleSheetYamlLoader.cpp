#include "UI/Styles/UIStyleSheetYamlLoader.h"
#include "UI/Styles/UIStyleSheet.h"
#include "UI/UIControl.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlNode.h"
#include "Utils/Utils.h"

namespace DAVA
{
UIStyleSheetYamlLoader::UIStyleSheetYamlLoader()
{
}

void UIStyleSheetYamlLoader::LoadFromYaml(const YamlNode* rootNode, Vector<UIStyleSheet*>* styleSheets)
{
    DVASSERT(styleSheets);

    const auto& styleSheetMap = rootNode->AsVector();
    const UIStyleSheetPropertyDataBase* propertyDB = UIStyleSheetPropertyDataBase::Instance();

    for (auto styleSheetIter = styleSheetMap.begin(); styleSheetIter != styleSheetMap.end(); ++styleSheetIter)
    {
        const auto& styleSheet = (*styleSheetIter)->AsMap();

        auto propertiesSectionIter = styleSheet.find("properties");

        if (propertiesSectionIter != styleSheet.end())
        {
            Vector<UIStyleSheetProperty> propertiesToSet;
            ScopedPtr<UIStyleSheetPropertyTable> propertyTable(new UIStyleSheetPropertyTable());

            for (const auto& propertyIter : propertiesSectionIter->second->AsMap())
            {
                uint32 index = propertyDB->GetStyleSheetPropertyIndex(FastName(propertyIter.first));
                const UIStyleSheetPropertyDescriptor& propertyDescr = propertyDB->GetStyleSheetPropertyByIndex(index);
                if (propertyDescr.field != nullptr)
                {
                    propertiesToSet.push_back(UIStyleSheetProperty{ index, propertyIter.second->AsAny(propertyDescr.field) });
                }
            }

            auto transitionSectionIter = styleSheet.find("transition");

            if (transitionSectionIter != styleSheet.end())
            {
                for (const auto& propertyTransitionIter : transitionSectionIter->second->AsMap())
                {
                    uint32 index = propertyDB->GetStyleSheetPropertyIndex(FastName(propertyTransitionIter.first));
                    for (UIStyleSheetProperty& prop : propertiesToSet)
                    {
                        if (prop.propertyIndex == index)
                        {
                            const auto& transitionProps = propertyTransitionIter.second->AsVector();
                            int32 transitionFunctionType = Interpolation::LINEAR;
                            if (transitionProps.size() > 1)
                                GlobalEnumMap<Interpolation::FuncType>::Instance()->ToValue(transitionProps[1]->AsString().c_str(), transitionFunctionType);

                            prop.transition = true;
                            prop.transitionFunction = static_cast<Interpolation::FuncType>(transitionFunctionType);
                            prop.transitionTime = transitionProps[0]->AsFloat();

                            break;
                        }
                    }
                }
            }
            propertyTable->SetProperties(propertiesToSet);

            Vector<String> selectorList;
            Split(styleSheet.find("selector")->second->AsString(), ",", selectorList);

            for (const String& selectorString : selectorList)
            {
                UIStyleSheet* styleSheet = new UIStyleSheet();

                styleSheet->SetSelectorChain(UIStyleSheetSelectorChain(selectorString));
                styleSheet->SetPropertyTable(propertyTable);

                styleSheets->push_back(styleSheet);
            }
        }
    }
}
}
