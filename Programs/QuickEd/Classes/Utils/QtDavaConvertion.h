#pragma once

#include <Base/BaseTypes.h>
#include <Base/BaseMath.h>
#include <FileSystem/VariantType.h>
#include <Base/Any.h>
#include <Base/IntrospectionBase.h>
#include <Reflection/ReflectedStructure.h>

#include <QMetaType>

class QString;
class QColor;
class QVector2D;

Q_DECLARE_METATYPE(DAVA::VariantType);
Q_DECLARE_METATYPE(DAVA::Any);

DAVA::String QStringToString(const QString& str);
QString StringToQString(const DAVA::String& str);

DAVA::WideString QStringToWideString(const QString& str);
QString WideStringToQString(const DAVA::WideString& str);

QColor HexToQColor(const QString& str);
QString QColorToHex(const QColor& color);

DAVA::Vector2 QVector2DToVector2(const QVector2D& vector);
QVector2D Vector2ToQVector2D(const DAVA::Vector2& vector);

QString AnyToQString(const DAVA::Any& val, const DAVA::ReflectedStructure::Field* field);
DAVA::String AnyToString(const DAVA::Any& any);

DAVA::VariantType AnyToVariantType(const DAVA::Any& any);
DAVA::Any VariantTypeToAny(const DAVA::VariantType& val);
const DAVA::Type* VariantTypeToType(DAVA::VariantType::eVariantType type);
