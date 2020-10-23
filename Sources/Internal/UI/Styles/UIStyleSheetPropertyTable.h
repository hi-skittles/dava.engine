#ifndef __DAVAENGINE_UI_STYLESHEET_PROPERTY_TABLE_H__
#define __DAVAENGINE_UI_STYLESHEET_PROPERTY_TABLE_H__

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "UI/Styles/UIStyleSheetStructs.h"
#include "UI/Styles/UIStyleSheetPropertyDataBase.h"

namespace DAVA
{
class UIStyleSheetPropertyTable :
public BaseObject
{
protected:
    virtual ~UIStyleSheetPropertyTable(){};

public:
    void SetProperties(const Vector<UIStyleSheetProperty>& properties);
    const Vector<UIStyleSheetProperty>& GetProperties() const;

    const UIStyleSheetPropertySet& GetPropertySet() const;

private:
    Vector<UIStyleSheetProperty> properties;
    UIStyleSheetPropertySet propertySet;
};
};


#endif
