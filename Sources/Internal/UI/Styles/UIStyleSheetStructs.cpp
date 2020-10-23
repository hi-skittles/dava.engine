#include "UI/Styles/UIStyleSheetStructs.h"

#include "Utils/Utils.h"

namespace DAVA
{
bool UIStyleSheetClassSet::AddClass(const FastName& clazz)
{
    auto it = std::find_if(classes.begin(), classes.end(), [&clazz](const UIStyleSheetClass& cl) {
        return cl.clazz == clazz && !cl.tag.IsValid();
    });

    if (it == classes.end())
    {
        classes.push_back(UIStyleSheetClass(FastName(), clazz));
        return true;
    }
    return false;
}

bool UIStyleSheetClassSet::RemoveClass(const FastName& clazz)
{
    auto it = std::find_if(classes.begin(), classes.end(), [&clazz](const UIStyleSheetClass& cl) {
        return cl.clazz == clazz && !cl.tag.IsValid();
    });

    if (it != classes.end())
    {
        *it = classes.back();
        classes.pop_back();
        return true;
    }
    return false;
}

bool UIStyleSheetClassSet::HasClass(const FastName& clazz) const
{
    auto it = std::find_if(classes.begin(), classes.end(), [&clazz](const UIStyleSheetClass& cl) {
        return cl.clazz == clazz;
    });

    return it != classes.end();
}

bool UIStyleSheetClassSet::SetTaggedClass(const FastName& tag, const FastName& clazz)
{
    auto it = std::find_if(classes.begin(), classes.end(), [&tag](const UIStyleSheetClass& cl) {
        return cl.tag == tag;
    });

    if (it != classes.end())
    {
        if (it->clazz == clazz)
        {
            return false;
        }
        else
        {
            it->clazz = clazz;
            return true;
        }
    }
    else
    {
        classes.push_back(UIStyleSheetClass(tag, clazz));
        return true;
    }
}

FastName UIStyleSheetClassSet::GetTaggedClass(const FastName& tag) const
{
    if (!tag.IsValid())
    {
        return FastName();
    }

    auto it = std::find_if(classes.begin(), classes.end(), [&tag](const UIStyleSheetClass& cl) {
        return cl.tag == tag;
    });

    if (it != classes.end())
    {
        return it->clazz;
    }

    return FastName();
}

bool UIStyleSheetClassSet::ResetTaggedClass(const FastName& tag)
{
    auto it = std::find_if(classes.begin(), classes.end(), [&tag](const UIStyleSheetClass& cl) {
        return cl.tag == tag;
    });

    if (it != classes.end())
    {
        *it = classes.back();
        classes.pop_back();
        return true;
    }
    return false;
}

bool UIStyleSheetClassSet::RemoveAllClasses()
{
    if (!classes.empty())
    {
        classes.clear();
        return true;
    }
    return false;
}

String UIStyleSheetClassSet::GetClassesAsString() const
{
    String result;
    for (size_t i = 0; i < classes.size(); i++)
    {
        if (i != 0)
        {
            result += " ";
        }
        if (classes[i].tag.IsValid())
        {
            result += classes[i].tag.c_str();
            result += "=";
        }

        result += classes[i].clazz.c_str();
    }
    return result;
}

void UIStyleSheetClassSet::SetClassesFromString(const String& classesStr)
{
    Vector<String> tokens;
    Split(classesStr, " ", tokens);

    classes.clear();
    for (String& token : tokens)
    {
        Vector<String> pair;
        Split(token, "=", pair);
        if (pair.size() > 1)
        {
            classes.push_back(UIStyleSheetClass(FastName(pair[0]), FastName(pair[1])));
        }
        else if (pair.size() > 0)
        {
            classes.push_back(UIStyleSheetClass(FastName(), FastName(pair[0])));
        }
    }
}
}
