#ifndef __QUICKED_PROPERTY_VISITOR_H__
#define __QUICKED_PROPERTY_VISITOR_H__

class RootProperty;
class BackgroundPropertiesSection;
class ClassProperty;
class ComponentPropertiesSection;
class ControlPropertiesSection;
class CustomClassProperty;
class FontValueProperty;
class InternalControlPropertiesSection;
class IntrospectionProperty;
class LocalizedTextValueProperty;
class NameProperty;
class PrototypeNameProperty;
class StyleSheetRootProperty;
class StyleSheetSelectorProperty;
class StyleSheetProperty;

class PropertyVisitor
{
public:
    PropertyVisitor();
    virtual ~PropertyVisitor();

    virtual void VisitRootProperty(RootProperty* property) = 0;

    virtual void VisitControlSection(ControlPropertiesSection* property) = 0;
    virtual void VisitComponentSection(ComponentPropertiesSection* property) = 0;

    virtual void VisitNameProperty(NameProperty* property) = 0;
    virtual void VisitPrototypeNameProperty(PrototypeNameProperty* property) = 0;
    virtual void VisitClassProperty(ClassProperty* property) = 0;
    virtual void VisitCustomClassProperty(CustomClassProperty* property) = 0;

    virtual void VisitIntrospectionProperty(IntrospectionProperty* property) = 0;

    virtual void VisitStyleSheetRoot(StyleSheetRootProperty* property) = 0;
    virtual void VisitStyleSheetSelectorProperty(StyleSheetSelectorProperty* property) = 0;
    virtual void VisitStyleSheetProperty(StyleSheetProperty* property) = 0;
};

#endif // __QUICKED_PROPERTY_VISITOR_H__
