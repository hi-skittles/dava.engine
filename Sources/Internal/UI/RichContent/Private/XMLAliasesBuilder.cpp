#include "UI/RichContent/Private/XMLAliasesBuilder.h"
#include "FileSystem/XMLParser.h"

namespace DAVA
{
XMLAliasesBuilder::XMLAliasesBuilder(const String& alias_)
    : alias(RichContentAlias{ alias_ })
{
}

const RichContentAlias& XMLAliasesBuilder::GetAlias() const
{
    return alias;
}

XMLParserStatus XMLAliasesBuilder::Build(const String& xmlSrc)
{
    return XMLParser::ParseStringEx(xmlSrc, this);
}

void XMLAliasesBuilder::OnElementStarted(const String& elementName, const String& namespaceURI, const String& qualifiedName, const Map<String, String>& attributes)
{
    // Store only first tag and its attributes
    if (alias.tag.empty())
    {
        alias.tag = elementName;
        alias.attributes = attributes;
    }
}

void XMLAliasesBuilder::OnElementEnded(const String& elementName, const String& namespaceURI, const String& qualifiedName)
{
}

void XMLAliasesBuilder::OnFoundCharacters(const String& chars)
{
}
}
