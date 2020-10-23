#ifndef __QT_PROPERTY_DATA_DAVA_VARIANT_H__
#define __QT_PROPERTY_DATA_DAVA_VARIANT_H__

#include "Base/Introspection.h"
#include "Base/EnumMap.h"

#include "TArc/Utils/QtConnections.h"

#include "../QtPropertyData.h"

namespace DAVA
{
class ColorPickerDialog;
}

class QtPropertyDataDavaVariant
: public QtPropertyData
{
    friend class QtPropertyDataDavaVariantSubValue;

public:
    enum AllowedValueType
    {
        Default,
        TypeFlags,
    };

public:
    QtPropertyDataDavaVariant(const DAVA::FastName& name, const DAVA::VariantType& value);
    virtual ~QtPropertyDataDavaVariant();

    const DAVA::VariantType& GetVariantValue() const;
    void SetVariantValue(const DAVA::VariantType& value);

    void AddAllowedValue(const DAVA::VariantType& realValue, const QVariant& visibleValue = QVariant());
    void ClearAllowedValues();
    void SetAllowedValueType(AllowedValueType type);
    AllowedValueType GetAllowedValueType() const;

    void SetInspDescription(const DAVA::InspDesc& desc);

    QVariant FromDavaVariant(const DAVA::VariantType& variant) const;

    void SetOpenDialogFilter(const QString&);
    QString GetOpenDialogFilter();

    void SetDefaultOpenDialogPath(const QString&);
    QString GetDefaultOpenDialogPath();

    QVariant GetToolTip() const;

protected:
    DAVA::VariantType curVariantValue;

    virtual QVariant GetValueInternal() const;
    virtual QVariant GetValueAlias() const;
    virtual void SetValueInternal(const QVariant& value);

    virtual QWidget* CreateEditorInternal(QWidget* parent, const QStyleOptionViewItem& option) const;
    virtual bool SetEditorDataInternal(QWidget* editor);
    virtual bool EditorDoneInternal(QWidget* editor);

protected slots:
    void MultilineEditClicked();
    void ColorOWPressed();
    void FilePathOWPressed();
    void OnColorChanging(DAVA::ColorPickerDialog* colorPicker);

protected:
    struct AllowedValue
    {
        DAVA::VariantType realValue;
        QVariant visibleValue;
    };

    void InitFlags();
    void ChildsCreate();
    void ChildsSetFromMe();
    void MeSetFromChilds();

    void UpdateColorButtonIcon();

    QVariant FromKeyedArchive(DAVA::KeyedArchive* archive) const;
    QVariant FromFloat(DAVA::float32 value) const;
    QVariant FromVector4(const DAVA::Vector4& vector) const;
    QVariant FromVector3(const DAVA::Vector3& vector) const;
    QVariant FromVector2(const DAVA::Vector2& vector) const;
    QVariant FromMatrix4(const DAVA::Matrix4& matrix) const;
    QVariant FromMatrix3(const DAVA::Matrix3& matrix) const;
    QVariant FromMatrix2(const DAVA::Matrix2& matrix) const;
    QVariant FromColor(const DAVA::Color& color) const;
    QVariant FromAABBox3(const DAVA::AABBox3& aabbox) const;

    void ToKeyedArchive(const QVariant& value);
    void ToFloat(const QVariant& value);
    void ToVector4(const QVariant& value);
    void ToVector3(const QVariant& value);
    void ToVector2(const QVariant& value);
    void ToMatrix4(const QVariant& value);
    void ToMatrix3(const QVariant& value);
    void ToMatrix2(const QVariant& value);
    void ToColor(const QVariant& value);
    void ToAABBox3(const QVariant& value);
    int ParseFloatList(const QString& str, int maxCount, DAVA::float32* dest);

    void SubValueAdd(const DAVA::FastName& key, const DAVA::VariantType& subvalue);
    void SubValueSetToMe(const DAVA::FastName& key, const QVariant& subvalue);
    void SubValueSetFromMe(const DAVA::FastName& key, const QVariant& subvalue);
    QVariant SubValueGet(const DAVA::FastName& key);

    QWidget* CreateAllowedValuesEditor(QWidget* parent) const;
    QWidget* CreateAllowedFlagsEditor(QWidget* parent) const;
    void SetAllowedValueEditorData(QWidget* editorWidget);
    void ApplyAllowedValueFromEditor(QWidget* editorWidget);

    QStringList GetFlagsList() const;

    QVector<AllowedValue> allowedValues;
    mutable bool allowedValuesLocked;
    QtPropertyToolButton* allowedButton;
    AllowedValueType allowedValueType;

    QString openDialogFilter;
    QString defaultOpenDialogPath;
    bool isSettingMeFromChilds;

    mutable DAVA::QtConnections connections;
};

class QtPropertyDataDavaVariantSubValue
: public QtPropertyDataDavaVariant
{
public:
    QtPropertyDataDavaVariantSubValue(const DAVA::FastName& name, QtPropertyDataDavaVariant* parent, const DAVA::VariantType& subvalue);

    QtPropertyDataDavaVariant* parentVariant;
    bool trackParent;

    virtual void SetValueInternal(const QVariant& value);
    virtual bool IsMergable() const;
};

#endif // __QT_PROPERTY_DATA_DAVA_VARIANT_H__
