#pragma once
#include "Base/Any.h"
#include "Base/Type.h"
#include "Reflection/Private/Metas.h"

namespace DAVA
{
/**
 template T - Base of Meta
 template IndexT - Find helper type. In ReflectedMeta we store search index by Meta<IndexT, IndexT> and value as Any(Meta<T, IndexT>)
 T should be the same as IndexT, of should be derived from IndexT
*/
template <typename T, typename IndexT = T>
struct Meta : public T
{
    template <typename... Args>
    Meta(Args&&... args);
};

class Type;
class ReflectedMeta final
{
    friend class ReflectedTypeDB; // friend for stast calculation

public:
    ReflectedMeta() = default;

    ReflectedMeta(const ReflectedMeta&) = delete;
    ReflectedMeta& operator=(const ReflectedMeta&) = delete;

    DAVA_DEPRECATED(ReflectedMeta(ReflectedMeta&& rm)); // visual studio 2013 require this

    template <typename T, typename IndexT>
    ReflectedMeta(Meta<T, IndexT>&& meta);

    template <typename T>
    const T* GetMeta() const;

    const void* GetMeta(const Type* metaType) const;

    template <typename T, typename IndexT>
    void Emplace(Meta<T, IndexT>&& meta);

protected:
    UnorderedMap<const Type*, Any> metas;
};

template <typename T, typename IndexT, typename U, typename IndexU>
ReflectedMeta operator, (Meta<T, IndexT> && metaa, Meta<IndexU>&& metab);

template <typename T, typename IndexT>
ReflectedMeta&& operator, (ReflectedMeta && rmeta, Meta<T, IndexT>&& meta);

namespace M
{
/**
    \defgroup metas Metas
*/

/**
    \ingroup metas
    Add hint that indicates ReadOnly policy for some Reflected Field
*/
using ReadOnly = Meta<Metas::ReadOnly>;

/**
    \ingroup metas
    Mark field as invisible in property panel
*/
using HiddenField = Meta<Metas::HiddenField>;
/**
    \ingroup metas
    Mark field to be visible only in developer mode of property panel
*/
using DeveloperModeOnly = Meta<Metas::DeveloperModeOnly>;
/**
    \ingroup metas
    Name of property that should be shown for user instead of field name
*/
using DisplayName = Meta<Metas::DisplayName>;
/** Hint that property can be edited as slider */
using Slider = Meta<Metas::Slider>;
/**
    \ingroup metas
    Add hint that indicates valid range of value
    \arg \c minValue has value of DAVA::Any
    \arg \c maxValue has value of DAVA::Any
    Control will try to cast minValue and maxValue to control specific type T
    If some of bound couldn't be casted to T, this bound will be equal std::numeric_limits<T>::min\max.
*/
using Range = Meta<Metas::Range>;
/**
    \ingroup metas
    Specifies count of signs in fraction part of float number for editing
    \arg \c accuracy has value of DAVA::uint32
*/
using FloatNumberAccuracy = Meta<Metas::FloatNumberAccuracy>;

/**
 \ingroup metas
 Specifies maximum count of characters in text for editing
 \arg \c length has value of DAVA::uint32
 */
using MaxLength = Meta<Metas::MaxLength>;

/**
    \ingroup metas
    Add value validation function to Reflected Field.
    \arg \c validationFn should be pointer on a free function with specified signature \c TValidationFn
*/
using Validator = Meta<Metas::Validator>;
/**
    \ingroup metas
    Validation callback
*/
using TValidationFn = Metas::TValidationFn;
/**
    \ingroup metas
    Result of validation function
    To get more information check Metas::ValidationResult documentation
*/
using ValidationResult = Metas::ValidationResult;

/**
    \ingroup metas
    Add hint that indicates that value of Reflected Field is enum and value has String representation
    The following sample shows how to specify Enum meta for field:
    \anchor enum_example
    \code
    enum ErrorCode
    {
        NoErrors,
        OutOfBound,
        InvalidArgument
    }

    DAVA_REFLECTION(Error)
    {
        ReflectionRegistrator<Error>::Begin()
            .Field("error", &Error::error)[M::EnumT<ErrorCode>()]
            .End()
    }
    \endcode

    To get Enum meta from field you shoul use M::Enum:
    \code
    Reflection ref = model.GetField("error");
    const M::Enum* enumMeta = ref.GetMeta<M::Enum>();
    if (enumMeta != nullptr)
    {
        const EnumMap* enumMap = enumMeta->GetEnumMap();
        // some client code here
    }
    \endcode
*/
using Enum = Meta<Metas::Enum>;
/**
    \ingroup metas
    Template helper to specify Enum meta.
    You should use it in ReflectionRegistrator.
    To gen more information see \ref enum_example "Enum Example"
*/
template <typename T>
using EnumT = Meta<Metas::EnumT<T>, Metas::Enum>;

/**
    \ingroup metas
    Add hint that indicate Flags value of Reflected Field.
    Work the same as Enum meta.
    User should look after the values of Enum inside Flags to provide truly bitfield Enum
*/
using Flags = Meta<Metas::Flags>;
/**
    \ingroup metas
    Template helper to specify Flags meta.
    You should use it in ReflectionRegistrator.
    To gen more information see \ref enum_example "Enum Example"
*/
template <typename T>
using FlagsT = Meta<Metas::FlagsT<T>, Metas::Flags>;

/**
    \ingroup metas
    Add hint that indicate value of Reflected Field as File.
    \arg \c shouldExists has type of bool
*/
using File = Meta<Metas::File>;
/**
    \ingroup metas
    Add hint that indicate value of Reflected Field as Directory.
    \arg \c shouldExists has type of bool
*/
using Directory = Meta<Metas::Directory>;
/**
    \ingroup metas
    Specify logically group for set of Reflected Fields.
    \arg \c groupName has type of DAVA::String
*/
using Group = Meta<Metas::Group>;
/**
    \ingroup metas
    Specify function that can provide string representation of value.
    \arg \c descriptionFunction has value of String (*)(const DAVA::Any& value)

    Example:
    \code
    void VisibleFieldDescription(const Any& value)
    {
        return value.Cast<bool>() == true ? String("Visible") : String("Invisible");
    }

    DAVA_REFLECTION(Entity)
    {
        ReflectionRegistrator<Entity>::Begin()
        .Field("Visible", &Entity::visible)[M::ValueDescription(&VisibleFieldDescription)]
        .End();
    }
    \endcode
*/
using ValueDescription = Meta<Metas::ValueDescription>;

/**
     \ingroup metas
     Indicates that we can bind UIControl/UIComponent reflection field to some
     data from model. QuickEd uses this indication to build special editor in
     Property Panel.
*/
using Bindable = Meta<Metas::Bindable>;

/**
    \ingroup metas
    We think about some types like about base types: Vector2, Vector3, Vector4, Color, Rect etc
    But in real this types are complex and have fields. For example Vector3 comprises the following fields: X, Y, Z
    This meta mark field of "BaseType" as "field to edit". As a reaction there will be created separate sub-editor
    for each field that marked by this meta
*/
using SubProperty = Meta<Metas::SubProperty>;

/**
    \ingroup metas
    Says that value can be changed at some unpredictable moment and
    Reflection's client should update value as often as possible
*/
using FrequentlyChangedValue = Meta<Metas::FrequentlyChangedValue>;

/**
    \ingroup metas
    Type that derived from Component and marked by this Meta couldn't be created in PropertyPanel
*/
using CantBeCreatedManualyComponent = Meta<Metas::CantBeCreatedManualyComponent>;

/**
    \ingroup metas
    Type that derived from Component and marked by this Meta couldn't be created in PropertyPanel
*/
using CantBeDeletedManualyComponent = Meta<Metas::CantBeDeletedManualyComponent>;

/** 
    \ingroup metas
    Says that type derived from Component and marked by this Meta can't be exported 
*/
using NonExportableComponent = Meta<Metas::NonExportableComponent>;

/**
    \ingroup metas
    Says that type derived from Component and marked by this Meta can't be serialized
*/
using NonSerializableComponent = Meta<Metas::NonSerializableComponent>;

using Tooltip = Meta<Metas::Tooltip>;
using IntColor = Meta<Metas::IntColor>;

/** 
    \ingroup metas
    Marks UIComponent type as multiple instances per UIControl
*/
using Multiple = Meta<Metas::Multiple>;
}

} // namespace DAVA
