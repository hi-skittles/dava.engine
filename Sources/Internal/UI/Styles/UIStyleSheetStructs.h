#ifndef __DAVAENGINE_UI_STYLESHEET_STRUCTS_H__
#define __DAVAENGINE_UI_STYLESHEET_STRUCTS_H__

#include "Base/Introspection.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/VariantType.h"
#include "Animation/Interpolation.h"
#include "Reflection/Reflection.h"
#include "Reflection/ReflectedStructure.h"
#include "Reflection/ReflectedType.h"

namespace DAVA
{
struct UIStyleSheetPropertyGroup
{
    String prefix;
    const Type* componentType; // nullptr for UIControl
    const ReflectedType* refType;

    UIStyleSheetPropertyGroup(const String& prefix_, const Type* componentType_, const ReflectedType* refType_)
        : prefix(prefix_)
        , componentType(componentType_)
        , refType(refType_)
    {
    }
};

struct UIStyleSheetPropertyDescriptor
{
    UIStyleSheetPropertyGroup* group;

    FastName name;
    Any defaultValue;
    const ReflectedStructure::Field* field = nullptr;

    UIStyleSheetPropertyDescriptor(UIStyleSheetPropertyGroup* group_, const char* name_, const Any& defaultValue_)
        : group(group_)
        , name(name_)
        , defaultValue(defaultValue_)
    {
        const ReflectedStructure* s = group->refType->GetStructure();
        auto it = std::find_if(s->fields.begin(), s->fields.end(), [&name_](const std::unique_ptr<ReflectedStructure::Field>& field) {
            return field->name == FastName(name_);
        });
        if (it != s->fields.end())
        {
            field = it->get();
        }
    }

    inline String GetFullName() const
    {
        if (group->prefix.empty())
            return String(name.c_str());

        return group->prefix + String("-") + String(name.c_str());
    }
};

struct UIStyleSheetSelector
{
    UIStyleSheetSelector()
        : className("")
        , name()
        , stateMask(0)
    {
    }

    String className;
    FastName name;
    int32 stateMask;
    Vector<FastName> classes;
};

struct UIStyleSheetProperty
{
    UIStyleSheetProperty(uint32 aPropertyIndex,
                         const Any& aValue,
                         bool aTransition = false,
                         Interpolation::FuncType aTransitionFunction = Interpolation::LINEAR,
                         float32 aTransitionTime = 0.0f)
        : propertyIndex(aPropertyIndex)
        , value(aValue)
        , transitionFunction(aTransitionFunction)
        , transitionTime(aTransitionTime)
        , transition(aTransition)
    {
    }

    uint32 propertyIndex;
    Any value;
    Interpolation::FuncType transitionFunction;
    float32 transitionTime;
    bool transition;
};

struct UIStyleSheetClass
{
    UIStyleSheetClass(const FastName& tag_, const FastName& clazz_)
        : tag(tag_)
        , clazz(clazz_)
    {
    }

    FastName tag;
    FastName clazz;
};

class UIStyleSheetClassSet
{
public:
    bool AddClass(const FastName& clazz);
    bool RemoveClass(const FastName& clazz);
    bool HasClass(const FastName& clazz) const;
    bool SetTaggedClass(const FastName& tag, const FastName& clazz);
    FastName GetTaggedClass(const FastName& tag) const;
    bool ResetTaggedClass(const FastName& tag);

    bool RemoveAllClasses();

    String GetClassesAsString() const;
    void SetClassesFromString(const String& classes);

private:
    Vector<UIStyleSheetClass> classes;
};

struct UIStyleSheetSourceInfo
{
    UIStyleSheetSourceInfo() = default;

    UIStyleSheetSourceInfo(const FilePath& file_)
        : file(file_)
    {
    }

    FilePath file;
};
};

#endif
