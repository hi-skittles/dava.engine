#pragma once

#include <QtTools/Updaters/ContinuousUpdater.h>

#include <Base/RefPtr.h>
#include <FileSystem/VariantType.h>
#include <UI/Styles/UIStyleSheetPropertyDataBase.h>

#include <QAbstractItemModel>
#include <QSet>

namespace DAVA
{
class Any;
class UIControl;
class ContextAccessor;
class FieldBinder;
}

class AbstractProperty;
class RootProperty;
class PackageBaseNode;
class ControlNode;
class StyleSheetNode;
class StyleSheetProperty;
class StyleSheetSelectorProperty;
class StyleSheetPropertiesSection;
class StyleSheetSelectorsSection;
class ComponentPropertiesSection;

class PropertiesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum
    {
        ResetRole = Qt::UserRole + 1,
        BindingRole = Qt::UserRole + 2
    };

    PropertiesModel(QObject* parent = nullptr);
    ~PropertiesModel() override;

    void SetAccessor(DAVA::ContextAccessor* accessor);

    void Reset(PackageBaseNode* node_);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    const AbstractProperty* GetRootProperty() const;
    QModelIndex indexByProperty(const AbstractProperty* property, int column = 0);

signals:
    void ComponentAdded(const QModelIndex& index);

protected:
    void UpdateAllChangedProperties();
    // PropertyListener
    void UpdateProperty(AbstractProperty* property);

    void PropertyChanged(AbstractProperty* property);
    void ComponentPropertiesWillBeAdded(RootProperty* root, ComponentPropertiesSection* section, int index);
    void ComponentPropertiesWasAdded(RootProperty* root, ComponentPropertiesSection* section, int index);

    void ComponentPropertiesWillBeRemoved(RootProperty* root, ComponentPropertiesSection* section, int index);
    void ComponentPropertiesWasRemoved(RootProperty* root, ComponentPropertiesSection* section, int index);

    void StylePropertyWillBeAdded(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index);
    void StylePropertyWasAdded(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index);

    void StylePropertyWillBeRemoved(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index);
    void StylePropertyWasRemoved(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index);

    void StyleSelectorWillBeAdded(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index);
    void StyleSelectorWasAdded(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index);

    void StyleSelectorWillBeRemoved(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index);
    void StyleSelectorWasRemoved(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index);

    void OnStylePropertiesChanged(DAVA::UIControl* control, const DAVA::UIStyleSheetPropertySet& properties);

    virtual void ChangeProperty(AbstractProperty* property, const DAVA::Any& value);
    virtual void ChangeBindingProperty(AbstractProperty* property, const DAVA::String& value, DAVA::int32 mode);
    virtual void ResetProperty(AbstractProperty* property);

    QString makeQVariant(const AbstractProperty* property) const;
    void initAny(DAVA::Any& var, const QVariant& val) const;
    void CleanUp();

    void OnPackageChanged(const DAVA::Any& package);
    void BindFields();

    QColor GetErrorBgColor() const;
    QColor GetBoundColor() const;
    QString GetDataBindingIcon(DAVA::int32 bindingUpdateMode) const;

    ControlNode* controlNode = nullptr;
    StyleSheetNode* styleSheet = nullptr;
    AbstractProperty* rootProperty = nullptr;
    DAVA::Set<DAVA::RefPtr<AbstractProperty>> changedProperties;
    ContinuousUpdater propertiesUpdater;

    DAVA::ContextAccessor* accessor = nullptr;
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
};
