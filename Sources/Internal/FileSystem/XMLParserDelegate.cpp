#include "FileSystem/XMLParserDelegate.h"

namespace DAVA
{
bool XMLParserDelegate::GetAttribute(const Map<String, String>& attributesMap, const String& key, String& attributeValue)
{
    Map<String, String>::const_iterator it;
    it = attributesMap.find(key);
    if (it != attributesMap.end())
    {
        attributeValue = it->second;
        return true;
    }

    return false;
}
};
