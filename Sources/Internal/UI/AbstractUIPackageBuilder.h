#ifndef __DAVAENGINE_UI_ABSTRACT_PACKAGE_LOADER_H__
#define __DAVAENGINE_UI_ABSTRACT_PACKAGE_LOADER_H__

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "UI/Styles/UIStyleSheetSelectorChain.h"

namespace DAVA
{
class UIPackage;
class UIControl;
class UIStyleSheet;
class UIComponent;
class UIControlBackground;
class YamlNode;
class AbstractUIPackageBuilder;

class AbstractUIPackageLoader
{
public:
    virtual ~AbstractUIPackageLoader();
    virtual bool LoadPackage(const FilePath& packagePath, AbstractUIPackageBuilder* builder) = 0;
    virtual bool LoadControlByName(const FastName& name, AbstractUIPackageBuilder* builder) = 0;
};

class AbstractUIPackageBuilder
{
public:
    enum eControlPlace
    {
        TO_PROTOTYPES,
        TO_CONTROLS,
        TO_PREVIOUS_CONTROL
    };

    AbstractUIPackageBuilder();
    virtual ~AbstractUIPackageBuilder();

    virtual void BeginPackage(const FilePath& packagePath, int32 version) = 0;
    virtual void EndPackage() = 0;

    virtual bool ProcessImportedPackage(const String& packagePath, AbstractUIPackageLoader* loader) = 0;
    virtual void ProcessStyleSheet(const Vector<UIStyleSheetSelectorChain>& selectorChains, const Vector<UIStyleSheetProperty>& properties) = 0;

    virtual const ReflectedType* BeginControlWithClass(const FastName& controlName, const String& className) = 0;
    virtual const ReflectedType* BeginControlWithCustomClass(const FastName& controlName, const String& customClassName, const String& className) = 0;
    virtual const ReflectedType* BeginControlWithPrototype(const FastName& controlName, const String& packageName, const FastName& prototypeName, const String* customClassName, AbstractUIPackageLoader* loader) = 0;
    virtual const ReflectedType* BeginControlWithPath(const String& pathName) = 0;
    virtual const ReflectedType* BeginUnknownControl(const FastName& controlName, const YamlNode* node) = 0;
    virtual void EndControl(eControlPlace controlPlace) = 0;

    virtual void BeginControlPropertiesSection(const String& name) = 0;
    virtual void EndControlPropertiesSection() = 0;

    virtual const ReflectedType* BeginComponentPropertiesSection(const Type* componentType, uint32 componentIndex) = 0;
    virtual void EndComponentPropertiesSection() = 0;

    virtual void ProcessProperty(const DAVA::ReflectedStructure::Field& field, const Any& value) = 0;
    virtual void ProcessDataBinding(const DAVA::String& fieldName, const DAVA::String& expression, DAVA::int32 bindingMode) = 0;

    virtual void ProcessCustomData(const YamlNode* customDataNode);
};
}

#endif // __DAVAENGINE_UI_ABSTRACT_PACKAGE_LOADER_H__
