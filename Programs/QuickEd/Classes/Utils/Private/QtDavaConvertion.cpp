#include "Utils/QtDavaConvertion.h"

#include <TArc/Utils/Utils.h>
#include <TArc/Qt/QtString.h>

#include <Reflection/ReflectedMeta.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Utils/StringFormat.h>
#include <Utils/UTF8Utils.h>
#include <QColor>
#include <QVariant>
#include <QVector2D>

using namespace DAVA;

DAVA::String QStringToString(const QString& str)
{
    return DAVA::String(str.toStdString());
}

QString StringToQString(const DAVA::String& str)
{
    return QString::fromStdString(str);
}

DAVA::WideString QStringToWideString(const QString& str)
{
#ifdef __DAVAENGINE_MACOS__
    return str.toStdWString();
#else
    return DAVA::WideString((wchar_t*)str.unicode(), str.length());
#endif
}

QString WideStringToQString(const DAVA::WideString& str)
{
#ifdef __DAVAENGINE_MACOS__
    return QString::fromStdWString(str);
#else
    return QString((const QChar*)str.c_str(), static_cast<int>(str.length()));
#endif
}

DAVA::Vector2 QVector2DToVector2(const QVector2D& vector)
{
    return DAVA::Vector2(vector.x(), vector.y());
}

QVector2D Vector2ToQVector2D(const DAVA::Vector2& vector)
{
    return QVector2D(vector.x, vector.y);
}

QColor HexToQColor(const QString& str)
{
    QColor color;
    if (!str.startsWith("#"))
        return color;
    int len = str.length() - 1;
    switch (len)
    {
    case 6: //RGB
    {
        int r, g, b;
        if (3 == sscanf(str.toStdString().c_str(), "#%02x%02x%02x", &r, &g, &b))
        {
            color.setRgb(qRgb(r, g, b));
        }
    }
    break;
    case 8: //RGBA
    {
        int r, g, b, a;
        if (4 == sscanf(str.toStdString().c_str(), "#%02x%02x%02x%02x", &r, &g, &b, &a))
        {
            color.setRgba(qRgba(r, g, b, a));
        }
    }
    break;
    default:
        break;
    }

    return color;
}

QString QColorToHex(const QColor& color)
{
    QString str;
    str.sprintf("#%02x%02x%02x%02x", color.red(), color.green(), color.blue(), color.alpha());

    return str;
}

QString AnyToQString(const DAVA::Any& val, const DAVA::ReflectedStructure::Field* field)
{
    if (field->meta)
    {
        const M::Flags* flagsMeta = field->meta->GetMeta<M::Flags>();
        if (flagsMeta != nullptr)
        {
            int32 e = val.Cast<int32>();
            QString res = "";
            int p = 0;
            while (e)
            {
                if ((e & 0x01) != 0)
                {
                    if (!res.isEmpty())
                        res += " | ";

                    const int32 enumValue = 1 << p;
                    res += QString::fromStdString(flagsMeta->GetFlagsMap()->ToString(enumValue));
                }
                p++;
                e >>= 1;
            }
            return res;
        }

        const M::Enum* enumMeta = field->meta->GetMeta<M::Enum>();
        if (enumMeta != nullptr)
        {
            return QString::fromStdString(enumMeta->GetEnumMap()->ToString(val.Cast<int32>()));
        }
    }

    if (val.CanGet<int32>())
    {
        return QString::number(val.Get<int32>());
    }
    else if (val.CanGet<uint32>())
    {
        return QString::number(val.Get<uint32>());
    }
    else if (val.CanGet<uint64>())
    {
        return QString::number(val.Get<uint64>());
    }
    else if (val.CanGet<int64>())
    {
        return QString::number(val.Get<int64>());
    }
    else if (val.CanGet<uint16>())
    {
        return QString::number(val.Get<int16>());
    }
    else if (val.CanGet<int16>())
    {
        return QString::number(val.Get<int16>());
    }
    else if (val.CanGet<uint8>())
    {
        return QString::number(val.Get<uint8>());
    }
    else if (val.CanGet<int8>())
    {
        return QString::number(val.Get<int8>());
    }
    else if (val.CanGet<float32>())
    {
        return QString::number(val.Get<float32>());
    }
    else if (val.CanGet<String>())
    {
        return QString::fromStdString(val.Get<String>());
    }
    else if (val.CanGet<WideString>())
    {
        return QString::fromStdWString(val.Get<WideString>().c_str());
    }
    else if (val.CanGet<FastName>())
    {
        const FastName& fastName = val.Get<FastName>();
        if (fastName.IsValid())
        {
            return QString::fromStdString(fastName.c_str());
        }
        else
        {
            return QString();
        }
    }
    else if (val.CanGet<FilePath>())
    {
        return QString::fromStdString(val.Get<FilePath>().GetStringValue());
    }
    else if (val.CanGet<bool>())
    {
        return val.Get<bool>() ? "true" : "false";
    }

    return QString("");
}

String AnyToString(const Any& val)
{
    if (val.CanGet<int32>())
    {
        return Format("%d", val.Get<int32>());
    }
    else if (val.CanGet<uint32>())
    {
        return Format("%d", val.Get<uint32>());
    }
    else if (val.CanGet<uint64>())
    {
        return Format("%ld", val.Get<uint64>());
    }
    else if (val.CanGet<int64>())
    {
        return Format("%ldL", val.Get<int64>());
    }
    else if (val.CanGet<uint16>())
    {
        return Format("%d", val.Get<int16>());
    }
    else if (val.CanGet<int16>())
    {
        return Format("%d", val.Get<int16>());
    }
    else if (val.CanGet<uint8>())
    {
        return Format("%d", val.Get<uint8>());
    }
    else if (val.CanGet<int8>())
    {
        return Format("%d", val.Get<int8>());
    }
    else if (val.CanGet<float32>())
    {
        return Format("%f", val.Get<float32>());
    }
    else if (val.CanGet<String>())
    {
        return val.Get<String>();
    }
    else if (val.CanGet<WideString>())
    {
        return UTF8Utils::EncodeToUTF8(val.Get<WideString>());
    }
    else if (val.CanGet<FastName>())
    {
        const FastName& fastName = val.Get<FastName>();
        if (fastName.IsValid())
        {
            return fastName.c_str();
        }
        else
        {
            return "";
        }
    }
    else if (val.CanGet<FilePath>())
    {
        return val.Get<FilePath>().GetStringValue();
    }
    else if (val.CanGet<bool>())
    {
        return val.Get<bool>() ? "true" : "false";
    }

    DVASSERT(false);
    return String("");
}

VariantType AnyToVariantType(const DAVA::Any& val)
{
    if (val.CanGet<int32>())
    {
        return VariantType(val.Get<int32>());
    }
    else if (val.CanGet<uint32>())
    {
        return VariantType(val.Get<uint32>());
    }
    else if (val.CanGet<uint64>())
    {
        return VariantType(val.Get<uint64>());
    }
    else if (val.CanGet<int64>())
    {
        return VariantType(val.Get<int64>());
    }
    else if (val.CanGet<uint16>())
    {
        return VariantType(val.Get<int16>());
    }
    else if (val.CanGet<int16>())
    {
        return VariantType(val.Get<int16>());
    }
    else if (val.CanGet<uint8>())
    {
        return VariantType(val.Get<uint8>());
    }
    else if (val.CanGet<int8>())
    {
        return VariantType(val.Get<int8>());
    }
    else if (val.CanGet<float32>())
    {
        return VariantType(val.Get<float32>());
    }
    else if (val.CanGet<String>())
    {
        return VariantType(val.Get<String>());
    }
    else if (val.CanGet<WideString>())
    {
        return VariantType(val.Get<WideString>());
    }
    if (val.CanGet<FastName>())
    {
        return VariantType(val.Get<FastName>());
    }
    else if (val.CanGet<FilePath>())
    {
        return VariantType(val.Get<FilePath>());
    }
    else if (val.CanGet<bool>())
    {
        return VariantType(val.Get<bool>());
    }
    else if (val.CanGet<Color>())
    {
        return VariantType(val.Get<Color>());
    }
    else if (val.CanGet<Vector2>())
    {
        return VariantType(val.Get<Vector2>());
    }
    else if (val.CanGet<Vector3>())
    {
        return VariantType(val.Get<Vector3>());
    }
    else if (val.CanGet<Vector4>())
    {
        return VariantType(val.Get<Vector4>());
    }

    DVASSERT(false); // TODO: Implement all cases
    return VariantType();
}

Any VariantTypeToAny(const VariantType& val)
{
    switch (val.GetType())
    {
    case VariantType::TYPE_NONE:
        return Any();
    case VariantType::TYPE_BOOLEAN:
        return Any(val.AsBool());
    case VariantType::TYPE_INT8:
        return Any(val.AsInt8());
    case VariantType::TYPE_UINT8:
        return Any(val.AsUInt8());
    case VariantType::TYPE_INT16:
        return Any(val.AsInt16());
    case VariantType::TYPE_UINT16:
        return Any(static_cast<uint16>(val.AsUInt16()));
    case VariantType::TYPE_INT32:
        return Any(val.AsInt32());
    case VariantType::TYPE_UINT32:
        return Any(val.AsUInt32());
    case VariantType::TYPE_INT64:
        return Any(val.AsInt64());
    case VariantType::TYPE_UINT64:
        return Any(val.AsUInt64());
    case VariantType::TYPE_FLOAT:
        return Any(val.AsFloat());
    case VariantType::TYPE_FLOAT64:
        return Any(val.AsFloat64());
    case VariantType::TYPE_STRING:
        return Any(val.AsString());
    case VariantType::TYPE_WIDE_STRING:
        return Any(val.AsWideString());
    case VariantType::TYPE_FASTNAME:
        return Any(val.AsFastName());
    case VariantType::TYPE_VECTOR2:
        return Any(val.AsVector2());
    case VariantType::TYPE_COLOR:
        return Any(val.AsColor());
    case VariantType::TYPE_VECTOR4:
        return Any(val.AsVector4());
    case VariantType::TYPE_FILEPATH:
        return Any(val.AsFilePath());

    case VariantType::TYPE_BYTE_ARRAY:
    case VariantType::TYPE_KEYED_ARCHIVE:
    case VariantType::TYPE_VECTOR3:

    case VariantType::TYPE_MATRIX2:
    case VariantType::TYPE_MATRIX3:
    case VariantType::TYPE_MATRIX4:
    case VariantType::TYPE_AABBOX3:
    default:
        // DVASSERT
        break;
    }
    DVASSERT(false);

    return Any();
}

const Type* VariantTypeToType(DAVA::VariantType::eVariantType type)
{
    switch (type)
    {
    case VariantType::TYPE_NONE:
        return nullptr;
    case VariantType::TYPE_BOOLEAN:
        return Type::Instance<bool>();
    case VariantType::TYPE_INT8:
        return Type::Instance<int8>();
    case VariantType::TYPE_UINT8:
        return Type::Instance<uint8>();
    case VariantType::TYPE_INT16:
        return Type::Instance<int16>();
    case VariantType::TYPE_UINT16:
        return Type::Instance<uint16>();
    case VariantType::TYPE_INT32:
        return Type::Instance<int32>();
    case VariantType::TYPE_UINT32:
        return Type::Instance<uint32>();
    case VariantType::TYPE_INT64:
        return Type::Instance<int64>();
    case VariantType::TYPE_UINT64:
        return Type::Instance<uint64>();
    case VariantType::TYPE_FLOAT:
        return Type::Instance<float32>();
    case VariantType::TYPE_FLOAT64:
        return Type::Instance<float64>();
    case VariantType::TYPE_STRING:
        return Type::Instance<String>();
    case VariantType::TYPE_WIDE_STRING:
        return Type::Instance<WideString>();
    case VariantType::TYPE_FASTNAME:
        return Type::Instance<FastName>();
    case VariantType::TYPE_VECTOR2:
        return Type::Instance<Vector2>();
    case VariantType::TYPE_COLOR:
        return Type::Instance<Color>();
    case VariantType::TYPE_VECTOR4:
        return Type::Instance<Vector4>();
    case VariantType::TYPE_FILEPATH:
        return Type::Instance<FilePath>();

    case VariantType::TYPE_BYTE_ARRAY:
    case VariantType::TYPE_KEYED_ARCHIVE:
    case VariantType::TYPE_VECTOR3:

    case VariantType::TYPE_MATRIX2:
    case VariantType::TYPE_MATRIX3:
    case VariantType::TYPE_MATRIX4:
    case VariantType::TYPE_AABBOX3:
    default:
        // DVASSERT
        break;
    }
    DVASSERT(false);
    return nullptr;
}
