#include "UI/Styles/UIStyleSheetSelectorChain.h"
#include "UI/UIControl.h"

namespace DAVA
{
namespace
{
class SelectorParser
{
public:
    enum SelectorParserState
    {
        SELECTOR_STATE_CONTROL_CLASS_NAME,
        SELECTOR_STATE_CLASS,
        SELECTOR_STATE_PSEUDO_CLASS,
        SELECTOR_STATE_NAME,
        SELECTOR_STATE_NONE,
    };

    SelectorParser(Vector<UIStyleSheetSelector>& aSelectorChain)
        :
        selectorChain(aSelectorChain)
    {
    }

    void Parse(const char* selectorStr)
    {
        currentSelector = UIStyleSheetSelector();
        currentToken = "";
        state = SELECTOR_STATE_CONTROL_CLASS_NAME;

        while (*selectorStr && *selectorStr == ' ') ++selectorStr;

        while (*selectorStr)
        {
            if ((*selectorStr) == ' ')
            {
                FinishProcessingCurrentSelector();
                while (*(selectorStr + 1) == ' ') ++selectorStr;
            }
            else if ((*selectorStr) == '?')
            {
                FinishProcessingCurrentSelector();
                while (*(selectorStr + 1) == ' ') ++selectorStr;
                selectorChain.push_back(UIStyleSheetSelector());
            }
            else if ((*selectorStr) == '.')
            {
                GoToState(SELECTOR_STATE_CLASS);
            }
            else if ((*selectorStr) == ':')
            {
                GoToState(SELECTOR_STATE_PSEUDO_CLASS);
            }
            else if ((*selectorStr) == '#')
            {
                GoToState(SELECTOR_STATE_NAME);
            }
            else
            {
                currentToken += *selectorStr;
            }

            ++selectorStr;
        }
        FinishProcessingCurrentSelector();
    }

private:
    String currentToken;
    SelectorParserState state;
    UIStyleSheetSelector currentSelector;

    Vector<UIStyleSheetSelector>& selectorChain;

    void FinishProcessingCurrentSelector()
    {
        GoToState(SELECTOR_STATE_CONTROL_CLASS_NAME);
        if (!currentSelector.className.empty() || currentSelector.name.IsValid() || !currentSelector.classes.empty())
            selectorChain.push_back(currentSelector);
        currentSelector = UIStyleSheetSelector();
    }

    void GoToState(SelectorParserState newState)
    {
        if (currentToken != "")
        {
            if (state == SELECTOR_STATE_CONTROL_CLASS_NAME)
            {
                currentSelector.className = currentToken;
            }
            else if (state == SELECTOR_STATE_NAME)
            {
                currentSelector.name = FastName(currentToken);
            }
            else if (state == SELECTOR_STATE_CLASS)
            {
                currentSelector.classes.push_back(FastName(currentToken));
            }
            else if (state == SELECTOR_STATE_PSEUDO_CLASS)
            {
                for (int32 stateIndex = 0; stateIndex < UIControl::STATE_COUNT; ++stateIndex)
                    if (currentToken == UIControl::STATE_NAMES[stateIndex])
                        currentSelector.stateMask |= 1 << stateIndex;
            }
        }
        currentToken = "";
        state = newState;
    }
};
}

UIStyleSheetSelectorChain::UIStyleSheetSelectorChain()
{
}

UIStyleSheetSelectorChain::UIStyleSheetSelectorChain(const String& string)
{
    SelectorParser parser(selectors);
    parser.Parse(string.c_str());
}

String UIStyleSheetSelectorChain::ToString() const
{
    String result = "";

    for (const UIStyleSheetSelector& selectorChainIter : selectors)
    {
        if (selectorChainIter.className.empty() &&
            !selectorChainIter.name.IsValid() &&
            selectorChainIter.classes.empty() &&
            selectorChainIter.stateMask == 0)
        {
            result += "?";
        }
        else
        {
            result += selectorChainIter.className;
            if (selectorChainIter.name.IsValid())
                result += String("#") + selectorChainIter.name.c_str();

            for (const FastName& clazz : selectorChainIter.classes)
                result += String(".") + clazz.c_str();

            for (int32 stateIndex = 0; stateIndex < UIControl::STATE_COUNT; ++stateIndex)
                if (selectorChainIter.stateMask & (1 << stateIndex))
                    result += String(":") + UIControl::STATE_NAMES[stateIndex];
        }

        result += " ";
    }

    if (!result.empty())
        result.resize(result.size() - 1);
    else
        result = "?";

    return result;
}

Vector<UIStyleSheetSelector>::const_iterator UIStyleSheetSelectorChain::begin() const
{
    return selectors.begin();
}

Vector<UIStyleSheetSelector>::const_iterator UIStyleSheetSelectorChain::end() const
{
    return selectors.end();
}

Vector<UIStyleSheetSelector>::const_reverse_iterator UIStyleSheetSelectorChain::rbegin() const
{
    return selectors.rbegin();
}

Vector<UIStyleSheetSelector>::const_reverse_iterator UIStyleSheetSelectorChain::rend() const
{
    return selectors.rend();
}

int32 UIStyleSheetSelectorChain::GetSize() const
{
    return static_cast<int32>(selectors.size());
}
}
