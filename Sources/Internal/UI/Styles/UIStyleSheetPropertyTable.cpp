#include "UI/Styles/UIStyleSheetPropertyTable.h"

namespace DAVA
{
void UIStyleSheetPropertyTable::SetProperties(const Vector<UIStyleSheetProperty>& newProperties)
{
    properties = newProperties;

    std::sort(properties.begin(), properties.end(),
              [](const UIStyleSheetProperty& first, const UIStyleSheetProperty& second) {
                  return first.propertyIndex < second.propertyIndex;
              });

    propertySet.reset();

    for (const UIStyleSheetProperty& prop : properties)
    {
        propertySet[prop.propertyIndex] = true;
    }
}

const Vector<UIStyleSheetProperty>& UIStyleSheetPropertyTable::GetProperties() const
{
    return properties;
}

const UIStyleSheetPropertySet& UIStyleSheetPropertyTable::GetPropertySet() const
{
    return propertySet;
}
}
