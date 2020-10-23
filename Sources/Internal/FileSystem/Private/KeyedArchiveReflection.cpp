#include "FileSystem/Private/KeyedArchiveReflection.h"
#include "FileSystem/KeyedArchive.h"
#include "FileSystem/VariantType.h"
#include "FileSystem/FilePath.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Math/AABBox3.h"
#include "Math/Vector.h"
#include "Math/Matrix2.h"
#include "Math/Matrix4.h"
#include "Math/Matrix3.h"
#include "Math/Color.h"
#include "Base/FastName.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
const Type* GetTypeByVariantType(VariantType* value)
{
    if (value == nullptr)
    {
        return nullptr;
    };

    const Type* result = nullptr;
    switch (value->GetType())
    {
    case VariantType::TYPE_BOOLEAN:
        result = Type::Instance<bool>();
        break;
    case VariantType::TYPE_FLOAT:
        result = Type::Instance<float32>();
        break;
    case VariantType::TYPE_STRING:
        result = Type::Instance<String>();
        break;
    case VariantType::TYPE_WIDE_STRING:
        result = Type::Instance<WideString>();
        break;
    case VariantType::TYPE_BYTE_ARRAY:
        result = nullptr;
        break;
    case VariantType::TYPE_KEYED_ARCHIVE:
        result = Type::Instance<KeyedArchive*>();
        break;
    case VariantType::TYPE_INT64:
        result = Type::Instance<int64>();
        break;
    case VariantType::TYPE_UINT64:
        result = Type::Instance<uint64>();
        break;
    case VariantType::TYPE_VECTOR2:
        result = Type::Instance<Vector2>();
        break;
    case VariantType::TYPE_VECTOR3:
        result = Type::Instance<Vector3>();
        break;
    case VariantType::TYPE_VECTOR4:
        result = Type::Instance<Vector4>();
        break;
    case VariantType::TYPE_MATRIX2:
        result = Type::Instance<Matrix2>();
        break;
    case VariantType::TYPE_MATRIX3:
        result = Type::Instance<Matrix3>();
        break;
    case VariantType::TYPE_MATRIX4:
        result = Type::Instance<Matrix4>();
        break;
    case VariantType::TYPE_COLOR:
        result = Type::Instance<Color>();
        break;
    case VariantType::TYPE_FASTNAME:
        result = Type::Instance<FastName>();
        break;
    case VariantType::TYPE_AABBOX3:
        result = Type::Instance<AABBox3>();
        break;
    case VariantType::TYPE_FILEPATH:
        result = Type::Instance<FilePath>();
        break;
    case VariantType::TYPE_FLOAT64:
        result = Type::Instance<float64>();
        break;
    case VariantType::TYPE_INT8:
    case VariantType::TYPE_INT16:
    case VariantType::TYPE_INT32:
        result = Type::Instance<int32>();
        break;
    case VariantType::TYPE_UINT8:
    case VariantType::TYPE_UINT16:
    case VariantType::TYPE_UINT32:
        result = Type::Instance<uint32>();
        break;
    default:
        break;
    }

    return result;
}

VariantType PrepareValueForKeyedArchiveImpl(const Any& value, VariantType::eVariantType resultType)
{
    VariantType result;
    switch (resultType)
    {
    case VariantType::TYPE_BOOLEAN:
        if (value.CanCast<bool>())
        {
            result.SetBool(value.Cast<bool>());
        }
        break;
    case VariantType::TYPE_INT32:
        if (value.CanCast<int32>())
        {
            result.SetInt32(value.Cast<int32>());
        }
        break;
    case VariantType::TYPE_FLOAT:
        if (value.CanCast<float32>())
        {
            result.SetFloat(value.Cast<float32>());
        }
        break;
    case VariantType::TYPE_STRING:
        if (value.CanCast<String>())
        {
            result.SetString(value.Cast<String>());
        }
        break;
    case VariantType::TYPE_WIDE_STRING:
        if (value.CanCast<WideString>())
        {
            result.SetWideString(value.Cast<WideString>());
        }
        break;
    case VariantType::TYPE_UINT32:
        if (value.CanCast<uint32>())
        {
            result.SetUInt32(value.Cast<uint32>());
        }
        break;
    case VariantType::TYPE_INT64:
        if (value.CanCast<int64>())
        {
            result.SetInt64(value.Cast<int64>());
        }
        break;
    case VariantType::TYPE_UINT64:
        if (value.CanCast<uint64>())
        {
            result.SetUInt64(value.Cast<uint64>());
        }
        break;
    case VariantType::TYPE_VECTOR2:
        if (value.CanCast<Vector2>())
        {
            result.SetVector2(value.Cast<Vector2>());
        }
        break;
    case VariantType::TYPE_VECTOR3:
        if (value.CanCast<Vector3>())
        {
            result.SetVector3(value.Cast<Vector3>());
        }
        break;
    case VariantType::TYPE_VECTOR4:
        if (value.CanCast<Vector4>())
        {
            result.SetVector4(value.Cast<Vector4>());
        }
        break;
    case VariantType::TYPE_MATRIX2:
        if (value.CanCast<Matrix2>())
        {
            result.SetMatrix2(value.Cast<Matrix2>());
        }
        break;
    case VariantType::TYPE_MATRIX3:
        if (value.CanCast<Matrix3>())
        {
            result.SetMatrix3(value.Cast<Matrix3>());
        }
        break;
    case VariantType::TYPE_MATRIX4:
        if (value.CanCast<Matrix4>())
        {
            result.SetMatrix4(value.Cast<Matrix4>());
        }
        break;
    case VariantType::TYPE_COLOR:
        if (value.CanCast<Color>())
        {
            result.SetColor(value.Cast<Color>());
        }
        break;
    case VariantType::TYPE_FASTNAME:
        if (value.CanCast<FastName>())
        {
            result.SetFastName(value.Cast<FastName>());
        }
        break;
    case VariantType::TYPE_FILEPATH:
        if (value.CanCast<FilePath>())
        {
            result.SetFilePath(value.Cast<FilePath>());
        }
        break;
    case VariantType::TYPE_FLOAT64:
        if (value.CanCast<float64>())
        {
            result.SetFloat64(value.Cast<float64>());
        }
        break;
    case VariantType::TYPE_INT8:
        if (value.CanCast<int8>())
        {
            result.SetInt32(value.Cast<int8>());
        }
        break;
    case VariantType::TYPE_UINT8:
        if (value.CanCast<uint8>())
        {
            result.SetUInt32(value.Cast<uint8>());
        }
        break;
    case VariantType::TYPE_INT16:
        if (value.CanCast<int16>())
        {
            result.SetInt32(value.Cast<int16>());
        }
        break;
    case VariantType::TYPE_UINT16:
        if (value.CanCast<uint16>())
        {
            result.SetUInt32(value.Cast<uint16>());
        }
        break;
    case VariantType::TYPE_AABBOX3:
        if (value.CanCast<AABBox3>())
        {
            result.SetAABBox3(value.Cast<AABBox3>());
        }
        break;
    case VariantType::TYPE_KEYED_ARCHIVE:
        if (value.CanGet<KeyedArchive*>())
        {
            result.SetKeyedArchive(value.Get<KeyedArchive*>());
        }
        else if (value.CanGet<RefPtr<KeyedArchive>>())
        {
            result.SetKeyedArchive(value.Get<RefPtr<KeyedArchive>>().Get());
        }
        else
        {
            DVASSERT(false);
        }
        break;
    case VariantType::TYPE_BYTE_ARRAY:
    default:
        DVASSERT(false);
        break;
    }

    return result;
}

const Type* KeyedArchiveElementValueWrapper::GetType(const ReflectedObject& object) const
{
    VariantType* value = object.GetPtr<VariantType>();
    return GetTypeByVariantType(value);
}

bool KeyedArchiveElementValueWrapper::IsReadonly(const ReflectedObject& object) const
{
    return object.IsConst();
}

Any KeyedArchiveElementValueWrapper::GetValue(const ReflectedObject& object) const
{
    VariantType* value = object.GetPtr<VariantType>();
    if (value == nullptr)
    {
        return Any();
    }

    Any result;
    switch (value->GetType())
    {
    case VariantType::TYPE_BOOLEAN:
        result = value->AsBool();
        break;
    case VariantType::TYPE_INT32:
        result = value->AsInt32();
        break;
    case VariantType::TYPE_FLOAT:
        result = value->AsFloat();
        break;
    case VariantType::TYPE_STRING:
        result = value->AsString();
        break;
    case VariantType::TYPE_WIDE_STRING:
        result = value->AsWideString();
        break;
    case VariantType::TYPE_BYTE_ARRAY:
        result = Any();
        break;
    case VariantType::TYPE_UINT32:
        result = value->AsUInt32();
        break;
    case VariantType::TYPE_KEYED_ARCHIVE:
        result = value->AsKeyedArchive();
        break;
    case VariantType::TYPE_INT64:
        result = value->AsInt64();
        break;
    case VariantType::TYPE_UINT64:
        result = value->AsUInt64();
        break;
    case VariantType::TYPE_VECTOR2:
        result = value->AsVector2();
        break;
    case VariantType::TYPE_VECTOR3:
        result = value->AsVector3();
        break;
    case VariantType::TYPE_VECTOR4:
        result = value->AsVector4();
        break;
    case VariantType::TYPE_MATRIX2:
        result = value->AsMatrix2();
        break;
    case VariantType::TYPE_MATRIX3:
        result = value->AsMatrix3();
        break;
    case VariantType::TYPE_MATRIX4:
        result = value->AsMatrix4();
        break;
    case VariantType::TYPE_COLOR:
        result = value->AsColor();
        break;
    case VariantType::TYPE_FASTNAME:
        result = value->AsFastName();
        break;
    case VariantType::TYPE_AABBOX3:
        result = value->AsAABBox3();
        break;
    case VariantType::TYPE_FILEPATH:
        result = value->AsFilePath();
        break;
    case VariantType::TYPE_FLOAT64:
        result = value->AsFloat64();
        break;
    case VariantType::TYPE_INT8:
        result = static_cast<int32>(value->AsInt8());
        break;
    case VariantType::TYPE_UINT8:
        result = static_cast<uint32>(value->AsUInt8());
        break;
    case VariantType::TYPE_INT16:
        result = static_cast<int32>(value->AsInt16());
        break;
    case VariantType::TYPE_UINT16:
        result = static_cast<uint32>(value->AsUInt16());
        break;
    default:
        break;
    }

    return result;
}

bool KeyedArchiveElementValueWrapper::SetValue(const ReflectedObject& object, const Any& value) const
{
    VariantType* v = object.GetPtr<VariantType>();
    const Type* srcType = GetTypeByVariantType(v);

    bool exception = srcType == Type::Instance<KeyedArchive*>() && value.GetType() == Type::Instance<RefPtr<KeyedArchive>>();

    if (srcType != value.GetType() && !exception)
    {
        return false;
    }

    switch (v->GetType())
    {
    case VariantType::TYPE_BOOLEAN:
        v->SetBool(value.Get<bool>());
        break;
    case VariantType::TYPE_INT32:
        v->SetInt32(value.Get<int32>());
        break;
    case VariantType::TYPE_FLOAT:
        v->SetFloat(value.Get<float32>());
        break;
    case VariantType::TYPE_STRING:
        v->SetString(value.Get<String>());
        break;
    case VariantType::TYPE_WIDE_STRING:
        v->SetWideString(value.Get<WideString>());
        break;
    case VariantType::TYPE_UINT32:
        v->SetUInt32(value.Get<uint32>());
        break;
    case VariantType::TYPE_INT64:
        v->SetInt64(value.Get<int64>());
        break;
    case VariantType::TYPE_UINT64:
        v->SetUInt64(value.Get<uint64>());
        break;
    case VariantType::TYPE_VECTOR2:
        v->SetVector2(value.Get<Vector2>());
        break;
    case VariantType::TYPE_VECTOR3:
        v->SetVector3(value.Get<Vector3>());
        break;
    case VariantType::TYPE_VECTOR4:
        v->SetVector4(value.Get<Vector4>());
        break;
    case VariantType::TYPE_MATRIX2:
        v->SetMatrix2(value.Get<Matrix2>());
        break;
    case VariantType::TYPE_MATRIX3:
        v->SetMatrix3(value.Get<Matrix3>());
        break;
    case VariantType::TYPE_MATRIX4:
        v->SetMatrix4(value.Get<Matrix4>());
        break;
    case VariantType::TYPE_COLOR:
        v->SetColor(value.Get<Color>());
        break;
    case VariantType::TYPE_FASTNAME:
        v->SetFastName(value.Get<FastName>());
        break;
    case VariantType::TYPE_FILEPATH:
        v->SetFilePath(value.Get<FilePath>());
        break;
    case VariantType::TYPE_FLOAT64:
        v->SetFloat64(value.Get<float64>());
        break;
    case VariantType::TYPE_INT8:
        v->SetInt32(value.Get<int8>());
        break;
    case VariantType::TYPE_UINT8:
        v->SetUInt32(value.Get<uint8>());
        break;
    case VariantType::TYPE_INT16:
        v->SetInt32(value.Get<int16>());
        break;
    case VariantType::TYPE_UINT16:
        v->SetUInt32(value.Get<uint16>());
        break;
    case VariantType::TYPE_AABBOX3:
        v->SetAABBox3(value.Get<AABBox3>());
        break;
    case VariantType::TYPE_KEYED_ARCHIVE:
        if (value.CanGet<KeyedArchive*>())
        {
            v->SetKeyedArchive(value.Get<KeyedArchive*>());
        }
        else if (value.CanGet<RefPtr<KeyedArchive>>())
        {
            v->SetKeyedArchive(value.Get<RefPtr<KeyedArchive>>().Get());
        }
        else
        {
            DVASSERT(false);
        }
        break;
    case VariantType::TYPE_BYTE_ARRAY:
    default:
        DVASSERT(false);
        return false;
    }

    return true;
}

bool KeyedArchiveElementValueWrapper::SetValueWithCast(const ReflectedObject& object, const Any& value) const
{
    VariantType* v = object.GetPtr<VariantType>();

    VariantType result = PrepareValueForKeyedArchive(value, v->GetType());
    if (result.GetType() != VariantType::TYPE_NONE)
    {
        DVASSERT(v->GetType() == result.GetType());
        *v = result;
        return true;
    }

    return false;
}

ReflectedObject KeyedArchiveElementValueWrapper::GetValueObject(const ReflectedObject& object) const
{
    VariantType* value = object.GetPtr<VariantType>();
    ReflectedObject result = object;
    switch (value->GetType())
    {
    case VariantType::TYPE_KEYED_ARCHIVE:
        result = ReflectedObject(value->AsKeyedArchive());
        break;
    case VariantType::TYPE_VECTOR2:
        result = ReflectedObject(value->vector2Value);
        break;
    case VariantType::TYPE_VECTOR3:
        result = ReflectedObject(value->vector3Value);
        break;
    case VariantType::TYPE_VECTOR4:
        result = ReflectedObject(value->vector4Value);
        break;
    case VariantType::TYPE_MATRIX2:
        result = ReflectedObject(value->matrix2Value);
        break;
    case VariantType::TYPE_MATRIX3:
        result = ReflectedObject(value->matrix3Value);
        break;
    case VariantType::TYPE_MATRIX4:
        result = ReflectedObject(value->matrix4Value);
        break;
    case VariantType::TYPE_COLOR:
        result = ReflectedObject(value->colorValue);
        break;
    case VariantType::TYPE_AABBOX3:
        result = ReflectedObject(value->aabbox3);
        break;
    default:
        break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      KeyedArchiveStructureWrapper                                          //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

KeyedArchiveStructureWrapper::KeyedArchiveStructureWrapper()
{
    caps.canAddField = true;
    caps.canRemoveField = true;
    caps.hasDynamicStruct = true;
}

bool KeyedArchiveStructureWrapper::HasFields(const ReflectedObject& object, const ValueWrapper* vw) const
{
    KeyedArchive* archive = vw->GetValueObject(object).GetPtr<KeyedArchive>();
    return archive->objectMap.empty() == false;
}

Reflection KeyedArchiveStructureWrapper::CreateReflection(VariantType* v, bool isArchiveConst) const
{
    ReflectedObject elementObj = ReflectedObject(v);
    if (isArchiveConst)
    {
        elementObj = ReflectedObject(const_cast<const VariantType*>(v));
    }

    const StructureWrapper* structureWrapper = nullptr;
    if (v->GetType() == VariantType::TYPE_KEYED_ARCHIVE)
    {
        structureWrapper = this;
    }

    return Reflection(elementObj, &valueWrapper, structureWrapper, nullptr);
}

Reflection KeyedArchiveStructureWrapper::GetField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const
{
    if (key.CanCast<String>())
    {
        String stringKey = key.Cast<String>();

        ReflectedObject archiveObject = vw->GetValueObject(object);
        KeyedArchive* archive = archiveObject.GetPtr<KeyedArchive>();
        if (archive->objectMap.count(stringKey) > 0)
        {
            VariantType* v = archive->objectMap[stringKey];
            return CreateReflection(v, archiveObject.IsConst());
        }
    }

    return Reflection();
}

Vector<Reflection::Field> KeyedArchiveStructureWrapper::GetFields(const ReflectedObject& object, const ValueWrapper* vw) const
{
    Vector<Reflection::Field> fields;

    ReflectedObject archiveObject = vw->GetValueObject(object);
    KeyedArchive* archive = archiveObject.GetPtr<KeyedArchive>();
    fields.reserve(archive->objectMap.size());

    bool isConst = archiveObject.IsConst();

    for (auto& node : archive->objectMap)
    {
        fields.emplace_back();
        Reflection::Field& f = fields.back();
        f.key = node.first;
        f.ref = CreateReflection(node.second, isConst);
    }

    return fields;
}

bool KeyedArchiveStructureWrapper::AddField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key, const Any& value) const
{
    if (key.CanCast<String>())
    {
        String k = key.Cast<String>();
        KeyedArchive* archive = vw->GetValueObject(object).GetPtr<KeyedArchive>();
        const Type* t = value.GetType();
        if (t == Type::Instance<int8>())
            archive->SetInt32(k, value.Get<int8>());
        else if (t == Type::Instance<uint8>())
            archive->SetUInt32(k, value.Get<uint8>());
        else if (t == Type::Instance<int16>())
            archive->SetInt32(k, value.Get<int16>());
        else if (t == Type::Instance<uint16>())
            archive->SetUInt32(k, value.Get<uint16>());
        else if (t == Type::Instance<int32>())
            archive->SetInt32(k, value.Get<int32>());
        else if (t == Type::Instance<uint32>())
            archive->SetUInt32(k, value.Get<uint32>());
        else if (t == Type::Instance<int64>())
            archive->SetInt64(k, value.Get<int64>());
        else if (t == Type::Instance<uint64>())
            archive->SetUInt64(k, value.Get<uint64>());
        else if (t == Type::Instance<float32>())
            archive->SetFloat(k, value.Get<float32>());
        else if (t == Type::Instance<float64>())
            archive->SetFloat64(k, value.Get<float64>());
        else if (t == Type::Instance<String>())
            archive->SetString(k, value.Get<String>());
        else if (t == Type::Instance<WideString>())
            archive->SetWideString(k, value.Get<WideString>());
        else if (t == Type::Instance<Vector2>())
            archive->SetVector2(k, value.Get<Vector2>());
        else if (t == Type::Instance<Vector3>())
            archive->SetVector3(k, value.Get<Vector3>());
        else if (t == Type::Instance<Vector4>())
            archive->SetVector4(k, value.Get<Vector4>());
        else if (t == Type::Instance<Matrix2>())
            archive->SetMatrix2(k, value.Get<Matrix2>());
        else if (t == Type::Instance<Matrix3>())
            archive->SetMatrix3(k, value.Get<Matrix3>());
        else if (t == Type::Instance<Matrix4>())
            archive->SetMatrix4(k, value.Get<Matrix4>());
        else if (t == Type::Instance<Color>())
            archive->SetColor(k, value.Get<Color>());
        else if (t == Type::Instance<bool>())
            archive->SetBool(k, value.Get<bool>());
        else if (t == Type::Instance<KeyedArchive*>())
            archive->SetArchive(k, value.Get<KeyedArchive*>());
        else if (t == Type::Instance<RefPtr<KeyedArchive>>())
            archive->SetArchive(k, value.Get<RefPtr<KeyedArchive>>().Get());

        return true;
    }

    return false;
}

bool KeyedArchiveStructureWrapper::RemoveField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const
{
    if (key.CanCast<String>())
    {
        KeyedArchive* archive = vw->GetValueObject(object).GetPtr<KeyedArchive>();
        archive->DeleteKey(key.Cast<String>());
        return true;
    }

    return false;
}

} // namespace DAVA