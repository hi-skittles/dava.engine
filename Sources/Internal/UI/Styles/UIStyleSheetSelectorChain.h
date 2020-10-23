#ifndef __DAVAENGINE_UI_STYLESHEET_SELECTOR_CHAIN_H__
#define __DAVAENGINE_UI_STYLESHEET_SELECTOR_CHAIN_H__

#include "Base/BaseTypes.h"
#include "UI/Styles/UIStyleSheetStructs.h"

namespace DAVA
{
class UIStyleSheetSelectorChain
{
public:
    UIStyleSheetSelectorChain();
    UIStyleSheetSelectorChain(const String& string);
    String ToString() const;

    Vector<UIStyleSheetSelector>::const_iterator begin() const;
    Vector<UIStyleSheetSelector>::const_iterator end() const;

    Vector<UIStyleSheetSelector>::const_reverse_iterator rbegin() const;
    Vector<UIStyleSheetSelector>::const_reverse_iterator rend() const;

    int32 GetSize() const;

private:
    Vector<UIStyleSheetSelector> selectors;
};
};


#endif
