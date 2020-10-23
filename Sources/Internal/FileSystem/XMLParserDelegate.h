#pragma once

#include "Base/BaseTypes.h"

using xmlChar = unsigned char;

namespace DAVA
{
class XMLParserDelegate
{
public:
    virtual ~XMLParserDelegate() = default;

    virtual void OnElementStarted(const String& elementName, const String& namespaceURI
                                  ,
                                  const String& qualifedName, const Map<String, String>& attributes) = 0;
    virtual void OnElementEnded(const String& elementName, const String& namespaceURI
                                ,
                                const String& qualifedName) = 0;

    virtual void OnFoundCharacters(const String& chars) = 0;

    /**
	 \brief Returns attribute value if this value is presents in the attributesMap.
	 \param[in] attributes map you want to search for.
	 \param[in] key you fant to found in the map.
	 \param[out] writes to this string value for the key if attribute is present.
	 \returns true if attribute for key is present.
	 */
    bool GetAttribute(const Map<String, String>& attributesMap, const String& key, String& attributeValue);
};
};
