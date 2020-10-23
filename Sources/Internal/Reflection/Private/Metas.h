#pragma once

#include "Base/Any.h"
#include "Base/BaseTypes.h"
#include "Base/EnumMap.h"
#include "Base/GlobalEnum.h"

namespace DAVA
{
namespace Metas
{
/** Defines that Reflected Field can't be changed */
class ReadOnly
{
};

/** Mark field as invisible in property panel */
class HiddenField
{
};

class DeveloperModeOnly
{
};

class DisplayName
{
public:
    DisplayName(const String& displayName);
    const String displayName;
};

/** Hint that property can be edited as slider */
class Slider
{
};

/**
    Defines valid range of value
    Control will try to cast minValue and maxValue to control specific type T
    If some of bound couldn't be casted to T, this bound will be equal std::numeric_limits<T>::min\max.
*/
class Range
{
public:
    Range(const Any& minValue, const Any& maxValue, const Any& step);
    const Any minValue;
    const Any maxValue;
    const Any step;
};

/**
    Specifies count of signs in fraction part of float number for editing
*/
class FloatNumberAccuracy
{
public:
    FloatNumberAccuracy(uint32 accuracy);
    const uint32 accuracy;
};

/**
 Specifies maximum count of characters in text for editing
 */
class MaxLength
{
public:
    MaxLength(uint32 length);
    const uint32 length;
};

/** Validation result */
struct ValidationResult
{
    /** This enum type defines the state in which a validated value can exist */
    enum class eState
    {
        /** Inputted value isn't valid and should be discarded */
        Invalid,
        /**
            Inputted value can be valid. For example value 4 invalid for Range(10, 99)
            but we should allow user continue editing because value 40 is valid
        */
        Intermediate,
        /** Inputted value is completely valid */
        Valid
    };

    /**
        \anchor validator_state
        Current state of validator
    */
    eState state = eState::Invalid;
    /**
        Validator can change value that user inputted
        Control will use this value only if \ref validator_state "state" equal Valid or Intermediate
    */
    Any fixedValue;
    /**
        Hint text for user, that describe why inputted value isn't valid
        Control use this only if \ref validator_state "state" equal Invalid
    */
    String message;
};

using TValidationFn = ValidationResult (*)(const Any& value, const Any& prevValue);

/** Validator for Reflected Field's value */
class Validator
{
public:
    Validator(const TValidationFn& fn);
    virtual ~Validator() = default;

    /**
        Validate value
        \arg \c value Inputted value by user
        \arg \c current value of Reflected Field
    */
    virtual ValidationResult Validate(const Any& value, const Any& prevValue) const;

private:
    TValidationFn fn;
};

/** Base class for all mate Enum types */
class Enum
{
public:
    virtual ~Enum() = default;
    /** Returns EnumMap that describe values of Enum */
    virtual const EnumMap* GetEnumMap() const = 0;
};

template <typename T>
class EnumT : public Enum
{
public:
    const EnumMap* GetEnumMap() const override;
};

template <typename T>
inline const EnumMap* EnumT<T>::GetEnumMap() const
{
    return GlobalEnumMap<T>::Instance();
}

/** Base class for all mate Enum types */
class Flags
{
public:
    virtual ~Flags() = default;
    /** Returns EnumMap that describe values of bitfield Enum */
    virtual const EnumMap* GetFlagsMap() const = 0;
};

template <typename T>
class FlagsT : public Flags
{
public:
    const EnumMap* GetFlagsMap() const override;
};

template <typename T>
inline const EnumMap* FlagsT<T>::GetFlagsMap() const
{
    return GlobalEnumMap<T>::Instance();
}

/** Defines that value of Reflected Field should be File */
class File
{
public:
    File(const String& filters, const String& dlgTitle = String("Open File"));
    virtual ~File() = default;

    virtual String GetDefaultPath() const;
    virtual String GetRootDirectory() const;

    const String filters;
    const String dlgTitle;
};

/** Defines that value of Reflected Field should be Directory */
class Directory
{
};

/** Defines logical group of set of Reflected Fields under the same name */
class Group
{
public:
    /** \arg \c groupName name of logical group */
    Group(const char* groupName);
    const char* groupName;
};

using TValueDescriptorFn = String (*)(const Any&);

/** Defines function that can provide string representation of value. */
class ValueDescription
{
public:
    ValueDescription(const TValueDescriptorFn& fn);

    String GetDescription(const Any& v) const;

private:
    TValueDescriptorFn fn;
};

/**
    Indicates that we can bind UIControl/UIComponent reflection field to some
    data from model. QuickEd uses this indication to build special editor in 
    Property Panel.
*/
class Bindable
{
public:
    Bindable();
};

/**
    We think about some types like about base types: Vector2, Vector3, Vector4, Color, Rect etc
    But in real this types are complex and have fields. For example Vector3 comprises the following fields: X, Y, Z
    This meta mark field of "BaseType" as "field to edit". As a reaction there will be created separate sub-editor
    for each field that marked by this meta
*/
class SubProperty
{
};

/**
    Says that value can be changed at some unpredictable moment and 
    Reflection's client should update value as often as possible
*/
class FrequentlyChangedValue
{
};

/** Type that derived from Component and marked by this Meta can't be created in PropertyPanel */
class CantBeCreatedManualyComponent
{
};

/** Type that derived from Component and marked by this Meta can't be deleted in PropertyPanel */
class CantBeDeletedManualyComponent
{
};

/** Says that type derived from Component and marked by this Meta can't be exported */
class NonExportableComponent
{
};

/** Says that type derived from Component and marked by this Meta can't be serialized */
class NonSerializableComponent
{
};

/** Indicate field in current type, that will return tooltip */
class Tooltip
{
public:
    Tooltip(const String& tooltipFieldName);
    String tooltipFieldName;
};

/** Indicate that color's components should be edited as int*/
class IntColor
{
};

/** Marks UIComponent type as multiples instances per UIControl */
class Multiple
{
};

} // namespace Mates
} // namespace DAVA
