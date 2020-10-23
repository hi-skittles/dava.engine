#ifndef __DAVAENGINE_UI_STYLESHEET_PROPERTIES_TABLE_H__
#define __DAVAENGINE_UI_STYLESHEET_PROPERTIES_TABLE_H__

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Base/StaticSingleton.h"
#include "UI/Styles/UIStyleSheetStructs.h"
#include <bitset>

namespace DAVA
{
class UIStyleSheetPropertyDataBase :
public StaticSingleton<UIStyleSheetPropertyDataBase>
{
public:
    virtual ~UIStyleSheetPropertyDataBase();
    static const int32 STYLE_SHEET_PROPERTY_COUNT = 85;

    UIStyleSheetPropertyDataBase();

    uint32 GetStyleSheetPropertyIndex(const FastName& name) const;
    uint32 GetStyleSheetVisiblePropertyIndex() const
    {
        return visiblePropertyIndex;
    }

    bool IsValidStyleSheetProperty(const FastName& name) const;
    const UIStyleSheetPropertyDescriptor& GetStyleSheetPropertyByIndex(uint32 index) const;
    int32 FindStyleSheetProperty(const Type* componentType, const FastName& name) const;

private:
    UIStyleSheetPropertyGroup controlGroup;
    UIStyleSheetPropertyGroup bgGroup;
    UIStyleSheetPropertyGroup staticTextGroup;
    UIStyleSheetPropertyGroup textFieldGroup;
    UIStyleSheetPropertyGroup particleEffectGroup;

    UIStyleSheetPropertyGroup linearLayoutGroup;
    UIStyleSheetPropertyGroup flowLayoutGroup;
    UIStyleSheetPropertyGroup flowLayoutHintGroup;
    UIStyleSheetPropertyGroup ignoreLayoutGroup;
    UIStyleSheetPropertyGroup sizePolicyGroup;
    UIStyleSheetPropertyGroup anchorGroup;
    UIStyleSheetPropertyGroup anchorSafeAreaGroup;
    UIStyleSheetPropertyGroup soundGroup;

    Array<UIStyleSheetPropertyDescriptor, STYLE_SHEET_PROPERTY_COUNT> properties; // have to be after groups declaration

    UnorderedMap<FastName, uint32> propertyNameToIndexMap;

    uint32 visiblePropertyIndex;
};

typedef std::bitset<UIStyleSheetPropertyDataBase::STYLE_SHEET_PROPERTY_COUNT> UIStyleSheetPropertySet;
};


#endif
