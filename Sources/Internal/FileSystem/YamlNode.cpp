#include "YamlNode.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/KeyedArchive.h"
#include "Utils/Utils.h"
#include "Utils/UTF8Utils.h"
#include "Utils/StringFormat.h"
#include <Base/RefPtrUtils.h>
#include "Base/Type.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
static const String EMPTY_STRING = "";
static const Vector<RefPtr<YamlNode>> EMPTY_VECTOR;
static const UnorderedMap<String, RefPtr<YamlNode>> EMPTY_MAP = UnorderedMap<String, RefPtr<YamlNode>>();

RefPtr<YamlNode> YamlNode::CreateStringNode()
{
    RefPtr<YamlNode> node = MakeRef<YamlNode>(TYPE_STRING);
    return node;
}

RefPtr<YamlNode> YamlNode::CreateArrayNode(eArrayRepresentation representation /* = AR_FLOW_REPRESENTATION*/)
{
    RefPtr<YamlNode> node = MakeRef<YamlNode>(TYPE_ARRAY);
    node->objectArray->style = representation;
    return node;
}

RefPtr<YamlNode> YamlNode::CreateMapNode(bool orderedSave /* = true*/, eMapRepresentation valRepresentation /*= MR_BLOCK_REPRESENTATION*/, eStringRepresentation keyRepresentation /*= SR_PLAIN_REPRESENTATION*/)
{
    RefPtr<YamlNode> node = MakeRef<YamlNode>(TYPE_MAP);
    node->objectMap->style = valRepresentation;
    node->objectMap->keyStyle = keyRepresentation;
    node->objectMap->orderedSave = orderedSave;
    return node;
}

YamlNode::YamlNode(eType _type)
    : type(_type)
{
    switch (GetType())
    {
    case TYPE_STRING:
        objectString = new ObjectString();
        objectString->style = SR_DOUBLE_QUOTED_REPRESENTATION;
        break;
    case TYPE_ARRAY:
        objectArray = new ObjectArray();
        objectArray->style = AR_FLOW_REPRESENTATION;
        break;
    case TYPE_MAP:
        objectMap = new ObjectMap();
        objectMap->style = MR_BLOCK_REPRESENTATION;
        objectMap->keyStyle = SR_PLAIN_REPRESENTATION;
        objectMap->orderedSave = true;
        break;
    }
}

YamlNode::~YamlNode()
{
    switch (GetType())
    {
    case TYPE_STRING:
    {
        SafeDelete(objectString);
    }
    break;
    case TYPE_ARRAY:
    {
        objectArray->array.clear();
        SafeDelete(objectArray);
    }
    break;
    case TYPE_MAP:
    {
        objectMap->ordered.clear();
        objectMap->unordered.clear();
        SafeDelete(objectMap);
    }
    break;
    }
}

uint32 YamlNode::GetCount() const
{
    switch (GetType())
    {
    case TYPE_MAP:
        return static_cast<uint32>(objectMap->unordered.size());
    case TYPE_ARRAY:
        return static_cast<uint32>(objectArray->array.size());
    default:
        break;
    }
    return 0; //string nodes does not contain content
}

int32 YamlNode::AsInt() const
{
    return AsInt32();
}

int32 YamlNode::AsInt32() const
{
    DVASSERT(GetType() == TYPE_STRING);
    int32 ret = 0;
    if (GetType() == TYPE_STRING)
    {
        sscanf(objectString->nwStringValue.c_str(), "%d", &ret);
    }
    return ret;
}

uint32 YamlNode::AsUInt32() const
{
    DVASSERT(GetType() == TYPE_STRING);
    uint32 ret = 0;
    if (GetType() == TYPE_STRING)
    {
        sscanf(objectString->nwStringValue.c_str(), "%u", &ret);
    }
    return ret;
}

int64 YamlNode::AsInt64() const
{
    DVASSERT(GetType() == TYPE_STRING);
    int64 ret = 0;
    if (GetType() == TYPE_STRING)
    {
        sscanf(objectString->nwStringValue.c_str(), "%lld", &ret);
    }
    return ret;
}

uint64 YamlNode::AsUInt64() const
{
    DVASSERT(GetType() == TYPE_STRING);
    uint64 ret = 0;
    if (GetType() == TYPE_STRING)
    {
        sscanf(objectString->nwStringValue.c_str(), "%llu", &ret);
    }
    return ret;
}

float32 YamlNode::AsFloat() const
{
    DVASSERT(GetType() == TYPE_STRING);
    float32 ret = 0.0f;
    if (GetType() == TYPE_STRING)
    {
        sscanf(objectString->nwStringValue.c_str(), "%f", &ret);
    }
    return ret;
}

const String& YamlNode::AsString() const
{
    DVASSERT(GetType() == TYPE_STRING);
    if (GetType() == TYPE_STRING)
        return objectString->nwStringValue;

    return EMPTY_STRING;
}

FastName YamlNode::AsFastName() const
{
    return FastName(AsString());
}

bool YamlNode::AsBool() const
{
    return ("true" == AsString() || "yes" == AsString());
}

WideString YamlNode::AsWString() const
{
    DVASSERT(GetType() == TYPE_STRING);
    if (GetType() == TYPE_STRING)
    {
        return UTF8Utils::EncodeToWideString(objectString->nwStringValue);
    }

    return L"";
}

Vector2 YamlNode::AsPoint() const
{
    Vector2 result;
    if (GetType() == TYPE_ARRAY)
    {
        const YamlNode* x = Get(0);
        if (x)
            result.x = x->AsFloat();
        const YamlNode* y = Get(1);
        if (y)
            result.y = y->AsFloat();
    }
    return result;
}

Vector3 YamlNode::AsVector3() const
{
    Vector3 result(0, 0, 0);
    if (GetType() == TYPE_ARRAY)
    {
        const YamlNode* x = Get(0);
        if (x)
            result.x = x->AsFloat();

        const YamlNode* y = Get(1);
        if (y)
            result.y = y->AsFloat();

        const YamlNode* z = Get(2);
        if (z)
            result.z = z->AsFloat();
    }
    return result;
}

Vector4 YamlNode::AsVector4() const
{
    Vector4 result(0, 0, 0, 0);
    if (GetType() == TYPE_ARRAY)
    {
        const YamlNode* x = Get(0);
        if (x)
            result.x = x->AsFloat();

        const YamlNode* y = Get(1);
        if (y)
            result.y = y->AsFloat();

        const YamlNode* z = Get(2);
        if (z)
            result.z = z->AsFloat();

        const YamlNode* w = Get(3);
        if (w)
            result.w = w->AsFloat();
    }
    return result;
}

Color YamlNode::AsColor() const
{
    Color result = Color::White;
    if (GetType() == TYPE_ARRAY)
    {
        const YamlNode* r = Get(0);
        if (r)
            result.r = r->AsFloat();

        const YamlNode* g = Get(1);
        if (g)
            result.g = g->AsFloat();

        const YamlNode* b = Get(2);
        if (b)
            result.b = b->AsFloat();

        const YamlNode* a = Get(3);
        if (a)
            result.a = a->AsFloat();
    }
    return result;
}

Vector2 YamlNode::AsVector2() const
{
    return AsPoint();
}

Rect YamlNode::AsRect() const
{
    Rect result;
    if (GetType() == TYPE_ARRAY)
    {
        const YamlNode* x = Get(0);
        if (x)
            result.x = x->AsFloat();
        const YamlNode* y = Get(1);
        if (y)
            result.y = y->AsFloat();
        const YamlNode* dx = Get(2);
        if (dx)
            result.dx = dx->AsFloat();
        const YamlNode* dy = Get(3);
        if (dy)
            result.dy = dy->AsFloat();
    }
    return result;
}

VariantType YamlNode::AsVariantType() const
{
    VariantType retValue;

    const auto& mapFromNode = AsMap();

    for (auto it = mapFromNode.begin(); it != mapFromNode.end(); ++it)
    {
        const String& innerTypeName = it->first;

        if (innerTypeName == DAVA::VariantType::TYPENAME_BOOLEAN)
        {
            retValue.SetBool(it->second->AsBool());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_INT8)
        {
            retValue.SetInt8(static_cast<int8>(it->second->AsInt32()));
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_UINT8)
        {
            retValue.SetUInt8(static_cast<uint8>(it->second->AsUInt32()));
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_INT16)
        {
            retValue.SetInt16(static_cast<int16>(it->second->AsInt32()));
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_UINT16)
        {
            retValue.SetUInt16(static_cast<uint16>(it->second->AsUInt32()));
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_INT32)
        {
            retValue.SetInt32(it->second->AsInt32());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_UINT32)
        {
            retValue.SetUInt32(it->second->AsUInt32());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_INT64)
        {
            retValue.SetInt64(it->second->AsInt64());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_UINT64)
        {
            retValue.SetUInt64(it->second->AsUInt64());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_FLOAT)
        {
            retValue.SetFloat(it->second->AsFloat());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_STRING)
        {
            retValue.SetString(it->second->AsString());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_FASTNAME)
        {
            retValue.SetFastName(it->second->AsFastName());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_WIDESTRING)
        {
            retValue.SetWideString(it->second->AsWString());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_BYTE_ARRAY)
        {
            const auto& byteArrayNoodes = it->second->AsVector();
            int32 size = static_cast<int32>(byteArrayNoodes.size());
            uint8* innerArray = new uint8[size];
            for (int32 i = 0; i < size; ++i)
            {
                int32 val = 0;
                int32 retCode = sscanf(byteArrayNoodes[i]->AsString().c_str(), "%x", &val);
                if ((val < 0) || (val > UCHAR_MAX) || (retCode == 0))
                {
                    delete[] innerArray;
                    return retValue;
                }
                innerArray[i] = static_cast<uint8>(val);
            }
            retValue.SetByteArray(innerArray, size);
            delete[] innerArray;
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_KEYED_ARCHIVE)
        {
            ScopedPtr<KeyedArchive> innerArch(new KeyedArchive());
            innerArch->LoadFromYamlNode(this);
            retValue.SetKeyedArchive(innerArch);
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_VECTOR2)
        {
            retValue.SetVector2(it->second->AsVector2());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_VECTOR3)
        {
            retValue.SetVector3(it->second->AsVector3());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_VECTOR4)
        {
            retValue.SetVector4(it->second->AsVector4());
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_MATRIX2)
        {
            const YamlNode* firstRowNode = it->second->Get(0);
            const YamlNode* secondRowNode = it->second->Get(1);
            if (NULL == firstRowNode || NULL == secondRowNode)
            {
                return retValue;
            }
            Vector2 fRowVect = firstRowNode->AsVector2();
            Vector2 sRowVect = secondRowNode->AsVector2();
            retValue.SetMatrix2(Matrix2(fRowVect.x, fRowVect.y, sRowVect.x, sRowVect.y));
        }
        else if (innerTypeName == VariantType::TYPENAME_MATRIX3)
        {
            const YamlNode* firstRowNode = it->second->Get(0);
            const YamlNode* secondRowNode = it->second->Get(1);
            const YamlNode* thirdRowNode = it->second->Get(2);

            if (NULL == firstRowNode ||
                NULL == secondRowNode ||
                NULL == thirdRowNode)
            {
                return retValue;
            }
            Vector3 fRowVect = firstRowNode->AsVector3();
            Vector3 sRowVect = secondRowNode->AsVector3();
            Vector3 tRowVect = thirdRowNode->AsVector3();

            retValue.SetMatrix3(Matrix3(fRowVect.x, fRowVect.y, fRowVect.z,
                                        sRowVect.x, sRowVect.y, sRowVect.z,
                                        tRowVect.x, tRowVect.y, tRowVect.z));
        }
        else if (innerTypeName == VariantType::TYPENAME_MATRIX4)
        {
            const YamlNode* firstRowNode = it->second->Get(0);
            const YamlNode* secondRowNode = it->second->Get(1);
            const YamlNode* thirdRowNode = it->second->Get(2);
            const YamlNode* fourthRowNode = it->second->Get(3);

            if (NULL == firstRowNode || NULL == secondRowNode ||
                NULL == thirdRowNode || NULL == fourthRowNode)
            {
                return retValue;
            }
            Vector4 fRowVect = firstRowNode->AsVector4();
            Vector4 sRowVect = secondRowNode->AsVector4();
            Vector4 tRowVect = thirdRowNode->AsVector4();
            Vector4 foRowVect = fourthRowNode->AsVector4();

            retValue.SetMatrix4(Matrix4(fRowVect.x, fRowVect.y, fRowVect.z, fRowVect.w,
                                        sRowVect.x, sRowVect.y, sRowVect.z, sRowVect.w,
                                        tRowVect.x, tRowVect.y, tRowVect.z, tRowVect.w,
                                        foRowVect.x, foRowVect.y, foRowVect.z, foRowVect.w));
        }
        else if (innerTypeName == DAVA::VariantType::TYPENAME_COLOR)
        {
            retValue.SetColor(it->second->AsColor());
        }
    }

    return retValue;
}

VariantType YamlNode::AsVariantType(const InspMember* insp) const
{
    if (insp->Desc().type == InspDesc::T_ENUM)
    {
        int32 val = 0;
        if (insp->Desc().enumMap->ToValue(AsString().c_str(), val))
        {
            return VariantType(val);
        }
        else
        {
            DVASSERT(false);
        }
    }
    else if (insp->Desc().type == InspDesc::T_FLAGS)
    {
        int32 val = 0;
        for (uint32 i = 0; i < GetCount(); i++)
        {
            const YamlNode* flagNode = Get(i);
            int32 flag = 0;
            if (insp->Desc().enumMap->ToValue(flagNode->AsString().c_str(), flag))
            {
                val |= flag;
            }
            else
            {
                DVASSERT(false);
            }
        }
        return VariantType(val);
    }
    else if (insp->Type() == MetaInfo::Instance<bool>())
        return VariantType(AsBool());
    else if (insp->Type() == MetaInfo::Instance<int32>())
        return VariantType(AsInt32());
    else if (insp->Type() == MetaInfo::Instance<uint32>())
        return VariantType(AsUInt32());
    else if (insp->Type() == MetaInfo::Instance<String>())
        return VariantType(AsString());
    else if (insp->Type() == MetaInfo::Instance<WideString>())
        return VariantType(AsWString());
    else if (insp->Type() == MetaInfo::Instance<float32>())
        return VariantType(AsFloat());
    else if (insp->Type() == MetaInfo::Instance<Vector2>())
        return VariantType(AsVector2());
    else if (insp->Type() == MetaInfo::Instance<Color>())
        return VariantType(AsColor());
    else if (insp->Type() == MetaInfo::Instance<Vector4>())
        return VariantType(AsVector4());
    else if (insp->Type() == MetaInfo::Instance<FilePath>())
        return VariantType(FilePath(AsString()));
    else if (insp->Type() == MetaInfo::Instance<FastName>())
        return VariantType(AsFastName());

    DVASSERT(false);
    return VariantType();
}

Any YamlNode::AsAny(const ReflectedStructure::Field* field) const
{
    // TODO: Make better
    const Type* type = field->valueWrapper->GetType(ReflectedObject())->Decay();
    const M::Enum* emeta = field->meta != nullptr ? field->meta->GetMeta<M::Enum>() : nullptr;
    const M::Flags* fmeta = field->meta != nullptr ? field->meta->GetMeta<M::Flags>() : nullptr;

    if (nullptr != emeta)
    {
        int32 val = 0;
        if (GetType() == TYPE_STRING)
        {
            if (emeta->GetEnumMap()->ToValue(AsString().c_str(), val))
            {
                return Any(val).ReinterpretCast(type);
            }
        }
        DVASSERT(false);
    }
    else if (nullptr != fmeta)
    {
        int32 val = 0;
        const uint32 count = GetCount();
        for (uint32 i = 0; i < count; i++)
        {
            const YamlNode* flagNode = Get(i);
            int32 flag = 0;
            if (fmeta->GetFlagsMap()->ToValue(flagNode->AsString().c_str(), flag))
            {
                val |= flag;
            }
            else
            {
                DVASSERT(false);
            }
        }
        return Any(val).ReinterpretCast(type);
    }

    if (type == Type::Instance<bool>())
        return Any(AsBool());
    else if (type == Type::Instance<int32>())
        return Any(AsInt32());
    else if (type == Type::Instance<uint32>())
        return Any(AsUInt32());
    else if (type == Type::Instance<int64>())
        return Any(AsInt64());
    else if (type == Type::Instance<uint64>())
        return Any(AsUInt64());
    else if (type == Type::Instance<float32>())
        return Any(AsFloat());
    else if (type == Type::Instance<FastName>())
        return Any(AsFastName());
    else if (type == Type::Instance<String>())
        return Any(AsString());
    else if (type == Type::Instance<WideString>())
        return Any(AsWString());
    else if (type == Type::Instance<Vector2>())
        return Any(AsVector2());
    else if (type == Type::Instance<Vector3>())
        return Any(AsVector3());
    else if (type == Type::Instance<Vector4>())
        return Any(AsVector4());
    else if (type == Type::Instance<Color>())
        return Any(AsColor());
    else if (type == Type::Instance<Rect>())
        return Any(AsRect());
    else if (type == Type::Instance<FilePath>())
        return Any(FilePath(AsString()));

    DVASSERT(false);
    return Any();
}

Any YamlNode::AsAny(const Reflection& ref) const
{
    // TODO: Make better
    const Type* type = ref.GetValueType()->Decay();
    const M::Enum* emeta = ref.GetMeta<M::Enum>();
    const M::Flags* fmeta = ref.GetMeta<M::Flags>();

    if (nullptr != emeta)
    {
        int32 val = 0;
        const M::Enum* emeta = ref.GetMeta<M::Enum>();
        if (GetType() == TYPE_STRING)
        {
            if (emeta->GetEnumMap()->ToValue(AsString().c_str(), val))
            {
                return Any(val);
            }
        }
        DVASSERT(false);
    }
    else if (nullptr != fmeta)
    {
        int32 val = 0;
        const uint32 count = GetCount();
        for (uint32 i = 0; i < count; i++)
        {
            const YamlNode* flagNode = Get(i);
            int32 flag = 0;
            if (fmeta->GetFlagsMap()->ToValue(flagNode->AsString().c_str(), flag))
            {
                val |= flag;
            }
            else
            {
                DVASSERT(false);
            }
        }
        return Any(val);
    }
    else if (type == Type::Instance<bool>())
        return Any(AsBool());
    else if (type == Type::Instance<int32>())
        return Any(AsInt32());
    else if (type == Type::Instance<uint32>())
        return Any(AsUInt32());
    else if (type == Type::Instance<int64>())
        return Any(AsInt64());
    else if (type == Type::Instance<uint64>())
        return Any(AsUInt64());
    else if (type == Type::Instance<float32>())
        return Any(AsFloat());
    else if (type == Type::Instance<FastName>())
        return Any(AsFastName());
    else if (type == Type::Instance<String>())
        return Any(AsString());
    else if (type == Type::Instance<WideString>())
        return Any(AsWString());
    else if (type == Type::Instance<Vector2>())
        return Any(AsVector2());
    else if (type == Type::Instance<Vector3>())
        return Any(AsVector3());
    else if (type == Type::Instance<Vector4>())
        return Any(AsVector4());
    else if (type == Type::Instance<Color>())
        return Any(AsColor());
    else if (type == Type::Instance<Rect>())
        return Any(AsRect());
    else if (type == Type::Instance<FilePath>())
        return Any(FilePath(AsString()));

    DVASSERT(false);
    return Any();
}

const Vector<RefPtr<YamlNode>>& YamlNode::AsVector() const
{
    DVASSERT(GetType() == TYPE_ARRAY);
    if (GetType() == TYPE_ARRAY)
        return objectArray->array;

    return EMPTY_VECTOR;
}

const UnorderedMap<String, RefPtr<YamlNode>>& YamlNode::AsMap() const
{
    DVASSERT(GetType() == TYPE_MAP);
    if (GetType() == TYPE_MAP)
        return objectMap->ordered;

    return EMPTY_MAP;
}

const YamlNode* YamlNode::Get(uint32 index) const
{
    if (GetType() == TYPE_ARRAY)
    {
        return objectArray->array[index].Get();
    }
    else if (GetType() == TYPE_MAP)
    {
        return objectMap->unordered[index].second.Get();
    }
    return nullptr;
}

const String& YamlNode::GetItemKeyName(uint32 index) const
{
    DVASSERT(GetType() == TYPE_MAP);
    if (GetType() == TYPE_MAP)
    {
        return objectMap->unordered[index].first;
    }
    return EMPTY_STRING;
}

const YamlNode* YamlNode::Get(const String& name) const
{
    //DVASSERT(GetType() == TYPE_MAP);
    if (GetType() == TYPE_MAP)
    {
        auto iter = objectMap->ordered.find(name);
        if (iter != objectMap->ordered.end())
        {
            return iter->second.Get();
        }
    }
    return NULL;
}

struct EqualToFirst
{
    EqualToFirst(const String& strValue)
        : value(strValue)
    {
    }
    bool operator()(const std::pair<String, RefPtr<YamlNode>>& val)
    {
        return val.first == value;
    }
    const String& value;
};

void YamlNode::RemoveNodeFromMap(const String& name)
{
    DVASSERT(GetType() == TYPE_MAP);
    auto iter = objectMap->ordered.find(name);
    if (iter == objectMap->ordered.end())
        return;

    objectMap->ordered.erase(iter);

    auto& array = objectMap->unordered;
    array.erase(std::remove_if(array.begin(), array.end(), EqualToFirst(name)), array.end());
}
YamlNode::eStringRepresentation YamlNode::GetStringRepresentation() const
{
    DVASSERT(GetType() == TYPE_STRING);
    return objectString->style;
}

void YamlNode::SetStringRepresentation(eStringRepresentation rep)
{
    DVASSERT(GetType() == TYPE_STRING);
    objectString->style = rep;
}

YamlNode::eArrayRepresentation YamlNode::GetArrayRepresentation() const
{
    DVASSERT(GetType() == TYPE_ARRAY);
    return objectArray->style;
}

YamlNode::eMapRepresentation YamlNode::GetMapRepresentation() const
{
    DVASSERT(GetType() == TYPE_MAP);
    return objectMap->style;
}

YamlNode::eStringRepresentation YamlNode::GetMapKeyRepresentation() const
{
    DVASSERT(GetType() == TYPE_MAP);
    return objectMap->keyStyle;
}

bool YamlNode::GetMapOrderRepresentation() const
{
    DVASSERT(GetType() == TYPE_MAP);
    return objectMap->orderedSave;
}

void YamlNode::InternalSetToString(const VariantType& varType)
{
    const bool initResult = InitStringFromVariantType(varType);
    DVASSERT(initResult);
}

void YamlNode::InternalSetToString(const String& value)
{
    InternalSetString(value, SR_DOUBLE_QUOTED_REPRESENTATION);
}

void YamlNode::InternalAddToMap(const String& name, const VariantType& varType, bool rewritePreviousValue)
{
    RefPtr<YamlNode> node = CreateNodeFromVariantType(varType);
    InternalAddNodeToMap(name, node, rewritePreviousValue);
}

void YamlNode::InternalAddToMap(const String& name, const String& value, bool rewritePreviousValue)
{
    RefPtr<YamlNode> node = CreateStringNode();
    node->InternalSetString(value, SR_DOUBLE_QUOTED_REPRESENTATION);
    InternalAddNodeToMap(name, node, rewritePreviousValue);
}

void YamlNode::InternalAddToArray(const VariantType& varType)
{
    RefPtr<YamlNode> node = CreateNodeFromVariantType(varType);
    InternalAddNodeToArray(node);
}

void YamlNode::InternalAddToArray(const String& value)
{
    RefPtr<YamlNode> node = CreateStringNode();
    node->InternalSetString(value, SR_DOUBLE_QUOTED_REPRESENTATION);
    InternalAddNodeToArray(node);
}

void YamlNode::InternalAddNodeToArray(const RefPtr<YamlNode>& node)
{
    DVASSERT(GetType() == TYPE_ARRAY);
    objectArray->array.push_back(node);
}

void YamlNode::InternalAddNodeToMap(const String& name, const RefPtr<YamlNode>& node, bool rewritePreviousValue)
{
    DVASSERT(GetType() == TYPE_MAP);
    if (rewritePreviousValue)
    {
        RemoveNodeFromMap(name);
    }

    DVASSERT(objectMap->ordered.find(name) == objectMap->ordered.end(), Format("YamlNode::InternalAddNodeToMap: map must have the unique key, \"%s\" is already there!", name.c_str()).c_str());
    objectMap->ordered.insert(std::make_pair(name, node));
    objectMap->unordered.push_back(std::make_pair(name, node));
}

void YamlNode::InternalSetString(const String& value, eStringRepresentation style /* = SR_DOUBLE_QUOTED_REPRESENTATION*/)
{
    DVASSERT(GetType() == TYPE_STRING);
    objectString->nwStringValue = value;
    objectString->style = style;
}

void YamlNode::InternalSetMatrix(const float32* array, uint32 dimension)
{
    for (uint32 i = 0; i < dimension; ++i)
    {
        RefPtr<YamlNode> rowNode = CreateArrayNode();
        rowNode->InternalSetVector(&array[i * dimension], dimension);
        InternalAddNodeToArray(rowNode);
    }
    objectArray->style = AR_FLOW_REPRESENTATION;
}

void YamlNode::InternalSetVector(const float32* array, uint32 dimension)
{
    objectArray->array.reserve(dimension);
    for (uint32 i = 0; i < dimension; ++i)
    {
        RefPtr<YamlNode> innerNode = CreateNodeFromVariantType(VariantType(array[i]));
        InternalAddNodeToArray(innerNode);
    }
    objectArray->style = AR_FLOW_REPRESENTATION;
}

void YamlNode::InternalSetByteArray(const uint8* byteArray, int32 byteArraySize)
{
    objectArray->array.reserve(byteArraySize);
    for (int32 i = 0; i < byteArraySize; ++i)
    {
        RefPtr<YamlNode> innerNode = CreateStringNode();
        innerNode->InternalSetString(Format("%x", byteArray[i]), SR_PLAIN_REPRESENTATION);
        InternalAddNodeToArray(innerNode);
    }
    objectArray->style = AR_FLOW_REPRESENTATION;
}

void YamlNode::InternalSetKeyedArchive(KeyedArchive* archive)
{
    //creation array with variables
    const KeyedArchive::UnderlyingMap& innerArchiveMap = archive->GetArchieveData();
    for (auto it = innerArchiveMap.begin(); it != innerArchiveMap.end(); ++it)
    {
        RefPtr<YamlNode> arrayElementNodeValue = CreateMapNode(true, MR_BLOCK_REPRESENTATION);
        arrayElementNodeValue->InternalAddNodeToMap(it->second->GetTypeName(), CreateNodeFromVariantType(*it->second), false);

        InternalAddNodeToMap(it->first, arrayElementNodeValue, false);
    }
    objectMap->style = MR_BLOCK_REPRESENTATION;
}

bool YamlNode::InitStringFromVariantType(const VariantType& varType)
{
    DVASSERT(GetType() == TYPE_STRING);
    bool result = true;
    switch (varType.GetType())
    {
    case VariantType::TYPE_BOOLEAN:
    {
        InternalSetString(varType.AsBool() ? "true" : "false", SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_INT8:
    {
        InternalSetString(Format("%hhd", varType.AsInt8()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_UINT8:
    {
        InternalSetString(Format("%hhu", varType.AsUInt8()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_INT16:
    {
        InternalSetString(Format("%hd", varType.AsInt16()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_UINT16:
    {
        InternalSetString(Format("%hu", varType.AsUInt16()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_INT32:
    {
        InternalSetString(Format("%d", varType.AsInt32()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_UINT32:
    {
        InternalSetString(Format("%u", varType.AsUInt32()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_FLOAT:
    {
        InternalSetString(Format("%f", varType.AsFloat()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_STRING:
    {
        InternalSetString(varType.AsString(), SR_DOUBLE_QUOTED_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_WIDE_STRING:
    {
        InternalSetString(UTF8Utils::EncodeToUTF8(varType.AsWideString()), SR_DOUBLE_QUOTED_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_FILEPATH:
    {
        InternalSetString(varType.AsFilePath().GetStringValue(), SR_DOUBLE_QUOTED_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_INT64:
    {
        InternalSetString(Format("%lld", varType.AsInt64()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_UINT64:
    {
        InternalSetString(Format("%llu", varType.AsUInt64()), SR_PLAIN_REPRESENTATION);
    }
    break;
    case VariantType::TYPE_FASTNAME:
    {
        if (varType.AsFastName().IsValid())
        {
            InternalSetString(varType.AsFastName().c_str(), SR_DOUBLE_QUOTED_REPRESENTATION);
        }
        else
        {
            InternalSetString("", SR_DOUBLE_QUOTED_REPRESENTATION);
        }
    }
    break;

    default:
        result = false;
        break;
    }
    return result;
}

bool YamlNode::InitArrayFromVariantType(const VariantType& varType)
{
    DVASSERT(GetType() == TYPE_ARRAY);
    bool result = true;
    switch (varType.GetType())
    {
    case VariantType::TYPE_BYTE_ARRAY:
    {
        const uint8* byteArray = varType.AsByteArray();
        int32 byteArraySize = varType.AsByteArraySize();
        InternalSetByteArray(byteArray, byteArraySize);
    }
    break;
    case VariantType::TYPE_VECTOR2:
    {
        const Vector2& vector = varType.AsVector2();
        InternalSetVector(vector.data, Vector2::AXIS_COUNT);
    }
    break;
    case VariantType::TYPE_VECTOR3:
    {
        const Vector3& vector = varType.AsVector3();
        InternalSetVector(vector.data, Vector3::AXIS_COUNT);
    }
    break;
    case VariantType::TYPE_VECTOR4:
    {
        const Vector4& vector = varType.AsVector4();
        InternalSetVector(vector.data, Vector4::AXIS_COUNT);
    }
    break;
    case VariantType::TYPE_MATRIX2:
    {
        uint32 dimension = 2;
        const Matrix2& matrix = varType.AsMatrix2();
        const float32* array = &matrix._data[0][0];
        InternalSetMatrix(array, dimension);
    }
    break;
    case VariantType::TYPE_MATRIX3:
    {
        uint32 dimension = 3;
        const Matrix3& matrix = varType.AsMatrix3();
        const float32* array = &matrix._data[0][0];
        InternalSetMatrix(array, dimension);
    }
    break;
    case VariantType::TYPE_MATRIX4:
    {
        uint32 dimension = 4;
        const Matrix4& matrix = varType.AsMatrix4();
        const float32* array = &matrix._data[0][0];
        InternalSetMatrix(array, dimension);
    }
    break;
    case VariantType::TYPE_COLOR:
    {
        const Color& color = varType.AsColor();
        InternalSetVector(color.color, Color::CHANNEL_COUNT);
    }
    break;
    default:
        result = false;
        break;
    }

    return result;
}

bool YamlNode::InitMapFromVariantType(const VariantType& varType)
{
    DVASSERT(GetType() == TYPE_MAP);
    bool result = true;
    switch (varType.GetType())
    {
    case VariantType::TYPE_KEYED_ARCHIVE:
    {
        KeyedArchive* archive = varType.AsKeyedArchive();
        InternalSetKeyedArchive(archive);
    }
    break;
    default:
        result = false;
        break;
    }

    return result;
}

RefPtr<YamlNode> YamlNode::CreateNodeFromVariantType(const VariantType& varType)
{
    eType nodeType = VariantTypeToYamlNodeType(varType.GetType());
    RefPtr<YamlNode> node;
    switch (nodeType)
    {
    case TYPE_STRING:
    {
        node = CreateStringNode();
        const bool initResult = node->InitStringFromVariantType(varType);
        DVASSERT(initResult);
    }
    break;
    case TYPE_ARRAY:
    {
        node = CreateArrayNode();
        const bool initResult = node->InitArrayFromVariantType(varType);
        DVASSERT(initResult);
    }
    break;
    case TYPE_MAP:
    {
        node = CreateMapNode();
        const bool initResult = node->InitMapFromVariantType(varType);
        DVASSERT(initResult);
    }
    break;
    }

    return node;
}

DAVA::YamlNode::eType YamlNode::VariantTypeToYamlNodeType(VariantType::eVariantType variantType)
{
    switch (variantType)
    {
    case VariantType::TYPE_BOOLEAN:
    case VariantType::TYPE_INT8:
    case VariantType::TYPE_UINT8:
    case VariantType::TYPE_INT16:
    case VariantType::TYPE_UINT16:
    case VariantType::TYPE_INT32:
    case VariantType::TYPE_UINT32:
    case VariantType::TYPE_FLOAT:
    case VariantType::TYPE_STRING:
    case VariantType::TYPE_WIDE_STRING:
    case VariantType::TYPE_INT64:
    case VariantType::TYPE_UINT64:
    case VariantType::TYPE_FILEPATH:
    case VariantType::TYPE_FASTNAME:
        return TYPE_STRING;

    case VariantType::TYPE_BYTE_ARRAY:
    case VariantType::TYPE_VECTOR2:
    case VariantType::TYPE_VECTOR3:
    case VariantType::TYPE_VECTOR4:
    case VariantType::TYPE_MATRIX2:
    case VariantType::TYPE_MATRIX3:
    case VariantType::TYPE_MATRIX4:
    case VariantType::TYPE_COLOR:
        return TYPE_ARRAY;

    case VariantType::TYPE_KEYED_ARCHIVE:
        return TYPE_MAP;
    default:
        break;
    }
    DVASSERT(false);
    return TYPE_MAP;
}
}
