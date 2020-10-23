#pragma once

#include "Base/BaseTypes.h"
#include "Base/Vector.h"
#include "Base/RefPtr.h"
#include "FileSystem/XMLParserDelegate.h"
#include "FileSystem/XMLParserStatus.h"
#include "Utils/BiDiHelper.h"

namespace DAVA
{
struct RichContentLink;
class UIControl;

class XMLRichContentBuilder final : public XMLParserDelegate
{
public:
    /** Constructor with specified RichContentLink pointer and editor mode and debug draw flags. */
    XMLRichContentBuilder(RichContentLink* link_, bool editorMode = false, bool debugDraw = false);

    /** Parse specified text and build list of controls. */
    XMLParserStatus Build(const String& text);

    /** Return generated controls. */
    const Vector<RefPtr<UIControl>>& GetControls() const;

protected:
    // XMLParserDelegate interface implementation
    void OnElementStarted(const String& elementName, const String& namespaceURI, const String& qualifiedName, const Map<String, String>& attributes) override;
    void OnElementEnded(const String& elementName, const String& namespaceURI, const String& qualifiedName) override;
    void OnFoundCharacters(const String& chars) override;

private:
    /** Store class in stack. */
    void PutClass(const String& clazz);
    /** Remove top class from stack. */
    void PopClass();
    /** Return top class from stack. */
    const String& GetClass() const;

    /** Setup base parameters in specified control. */
    void PrepareControl(UIControl* ctrl, bool autosize);
    /** Append control to the controls list. */
    void AppendControl(UIControl* ctrl);
    /** Process open tag. */
    void ProcessTagBegin(const String& tag, const Map<String, String>& attributes);
    /** Process close tag. */
    void ProcessTagEnd(const String& tag);
    /** Process text content. */
    void ProcessText(const String& text);
    /** Process concatenated text. */
    void FlushText();

private:
    bool needLineBreak = false;
    bool needSpace = false;
    bool needSoftStick = false;
    bool isEditorMode = false;
    bool isDebugDraw = false;
    bool classesInheritance = false;
    BiDiHelper::Direction direction = BiDiHelper::Direction::NEUTRAL;
    String fullText;
    String defaultClasses;
    Vector<String> classesStack;
    Vector<RefPtr<UIControl>> controls;
    RichContentLink* link = nullptr;
    BiDiHelper bidiHelper;
};
}
