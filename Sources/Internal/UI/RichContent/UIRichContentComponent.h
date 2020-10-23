#pragma once

#include "Functional/Signal.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIControl;

/**
    Describe rich content inside control.
    Format of input text source it xml (or xhtml).

    For default all text content in any tags will be added as UIStaticText controls.
    Also all tags support attribute `class` witch set specified class to created
    UIStaticText controls.

    Special rules available for next tags:
    - <p> - paragraph.
        Text content inside this tag will be wrap by new lines.
    - <ul> and <li> - unordered list and list item.
        Text content inside these tags will be wrap by new lines and before all
        lines in `li` tag will be added bullet symbol.
    - <img src="..."> - image.
        Insert UIControl with UIControlBackground component and set intro it
        sprite by path specified in `src` attribute.
    - <object path="..." control="..." prototype="..." name="..."> - custom control.
        Insert clone of specified control by root control name or prototype name
        witch contain in package by specialized path. Also set name to this clone
        from name attribute. Attribute `path` and one of attributes `control` or
        `prototype` are required. Attribute `name` is optional.
*/
class UIRichContentComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIRichContentComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIRichContentComponent);

public:
    /** Default constructor. */
    UIRichContentComponent() = default;
    /** Copy constructor. */
    UIRichContentComponent(const UIRichContentComponent& src);
    /** Removed operator overloading. */
    UIRichContentComponent& operator=(const UIRichContentComponent&) = delete;

    UIRichContentComponent* Clone() const override;

    /** Set rich content text. */
    void SetText(const String& text);
    /** Return rich content text. */
    const String& GetText() const;

    /** Set top level classes for rich content. */
    void SetBaseClasses(const String& classes);
    /** Return top level classes for rich content. */
    const String& GetBaseClasses() const;

    /** Set classes inheritance flag for nested elements. */
    void SetClassesInheritance(bool inheritance);
    /** Return classes inheritance flag for nested elements. */
    bool GetClassesInheritance() const;

    /** Set modification flag. */
    void SetModified(bool modified);
    /** Return value of modification flag. */
    bool IsModified() const;

    /** Emit signal on create control by `<object />` tag */
    Signal<UIControl* /* createdControl */> onCreateObject;

protected:
    ~UIRichContentComponent() override = default;

private:
    String text;
    String baseClasses;
    bool classesInheritance = false;
    bool modified = false;
};

inline const String& UIRichContentComponent::GetText() const
{
    return text;
}

inline const String& UIRichContentComponent::GetBaseClasses() const
{
    return baseClasses;
}

inline bool UIRichContentComponent::GetClassesInheritance() const
{
    return classesInheritance;
}

inline bool UIRichContentComponent::IsModified() const
{
    return modified;
}
}
