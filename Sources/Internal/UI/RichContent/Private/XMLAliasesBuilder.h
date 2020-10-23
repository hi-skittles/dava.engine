#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/XMLParserDelegate.h"
#include "FileSystem/XMLParserStatus.h"
#include "UI/RichContent/Private/RichStructs.h"

namespace DAVA
{
class XMLAliasesBuilder final : public XMLParserDelegate
{
public:
    XMLAliasesBuilder(const String& alias);
    const RichContentAlias& GetAlias() const;
    XMLParserStatus Build(const String& xmlSrc);

protected:
    void OnElementStarted(const String& elementName, const String& namespaceURI, const String& qualifiedName, const Map<String, String>& attributes) override;
    void OnElementEnded(const String& elementName, const String& namespaceURI, const String& qualifiedName) override;
    void OnFoundCharacters(const String& chars) override;

private:
    RichContentAlias alias;
};
}
