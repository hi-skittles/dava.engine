#ifndef __DAVAENGINE_UI_STYLESHEET_H__
#define __DAVAENGINE_UI_STYLESHEET_H__

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "FileSystem/VariantType.h"
#include "UI/Styles/UIStyleSheetPropertyDataBase.h"
#include "UI/Styles/UIStyleSheetStructs.h"
#include "UI/Styles/UIStyleSheetSelectorChain.h"
#include "UI/Styles/UIStyleSheetPropertyTable.h"

namespace DAVA
{
class UIStyleSheet :
public BaseObject
{
protected:
    virtual ~UIStyleSheet();

public:
    UIStyleSheet();

    int32 GetScore() const;

    const UIStyleSheetPropertyTable* GetPropertyTable() const;
    const UIStyleSheetSelectorChain& GetSelectorChain() const;
    const UIStyleSheetSourceInfo& GetSourceInfo() const;

    void SetPropertyTable(UIStyleSheetPropertyTable* properties);
    void SetSelectorChain(const UIStyleSheetSelectorChain& selectorChain);
    void SetSourceInfo(const UIStyleSheetSourceInfo& sourceInfo);

private:
    void RecalculateScore();

    UIStyleSheetSelectorChain selectorChain;

    RefPtr<UIStyleSheetPropertyTable> properties;

    UIStyleSheetSourceInfo sourceInfo;

    uint32 score;
};
};


#endif
