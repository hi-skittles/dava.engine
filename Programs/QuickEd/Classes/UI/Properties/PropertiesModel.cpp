#include "PropertiesModel.h"

#include "Modules/DocumentsModule/DocumentData.h"

#include "Model/ControlProperties/AbstractProperty.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/ControlProperties/SectionProperty.h"
#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "Model/ControlProperties/StyleSheetProperty.h"
#include "Model/ControlProperties/SubValueProperty.h"
#include "Model/ControlProperties/ValueProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Utils/QtDavaConvertion.h"
#include "QECommands/ChangePropertyValueCommand.h"
#include "QECommands/ChangePropertyForceOverrideCommand.h"
#include "QECommands/ChangeStylePropertyCommand.h"
#include "QECommands/ChangeBindingCommand.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/SharedModules/ThemesModule/ThemesModule.h>
#include <TArc/Utils/Utils.h>

#include <Engine/Engine.h>
#include <Reflection/ReflectedTypeDB.h>
#include <UI/UIControl.h>
#include <UI/DataBinding/UIDataBindingComponent.h>
#include <Utils/StringFormat.h>
#include <UI/UIControlSystem.h>
#include <UI/Styles/UIStyleSheetSystem.h>


#include <QFont>
#include <QIcon>
#include <QVector2D>
#include <QVector4D>
#include <QMimeData>

using namespace DAVA;

PropertiesModel::PropertiesModel(QObject* parent)
    : QAbstractItemModel(parent)
    , propertiesUpdater(500)
{
    propertiesUpdater.SetUpdater(MakeFunction(this, &PropertiesModel::UpdateAllChangedProperties));

    GetEngineContext()->uiControlSystem->GetStyleSheetSystem()->stylePropertiesChanged.Connect(this, &PropertiesModel::OnStylePropertiesChanged);
}

PropertiesModel::~PropertiesModel()
{
    GetEngineContext()->uiControlSystem->GetStyleSheetSystem()->stylePropertiesChanged.Disconnect(this);

    CleanUp();
    propertiesUpdater.Abort();
}

void PropertiesModel::SetAccessor(DAVA::ContextAccessor* accessor_)
{
    accessor = accessor_;
    BindFields();
}

void PropertiesModel::Reset(PackageBaseNode* nodeToReset)
{
    propertiesUpdater.Abort();
    beginResetModel();
    CleanUp();
    controlNode = dynamic_cast<ControlNode*>(nodeToReset);
    if (nullptr != controlNode)
    {
        controlNode->GetRootProperty()->propertyChanged.Connect(this, &PropertiesModel::PropertyChanged);
        controlNode->GetRootProperty()->componentPropertiesWillBeAdded.Connect(this, &PropertiesModel::ComponentPropertiesWillBeAdded);
        controlNode->GetRootProperty()->componentPropertiesWasAdded.Connect(this, &PropertiesModel::ComponentPropertiesWasAdded);

        controlNode->GetRootProperty()->componentPropertiesWillBeRemoved.Connect(this, &PropertiesModel::ComponentPropertiesWillBeRemoved);
        controlNode->GetRootProperty()->componentPropertiesWasRemoved.Connect(this, &PropertiesModel::ComponentPropertiesWasRemoved);

        rootProperty = controlNode->GetRootProperty();
    }

    styleSheet = dynamic_cast<StyleSheetNode*>(nodeToReset);
    if (nullptr != styleSheet)
    {
        styleSheet->GetRootProperty()->propertyChanged.Connect(this, &PropertiesModel::PropertyChanged);

        styleSheet->GetRootProperty()->stylePropertyWillBeAdded.Connect(this, &PropertiesModel::StylePropertyWillBeAdded);
        styleSheet->GetRootProperty()->stylePropertyWasAdded.Connect(this, &PropertiesModel::StylePropertyWasAdded);

        styleSheet->GetRootProperty()->stylePropertyWillBeRemoved.Connect(this, &PropertiesModel::StylePropertyWillBeRemoved);
        styleSheet->GetRootProperty()->stylePropertyWasRemoved.Connect(this, &PropertiesModel::StylePropertyWasRemoved);

        styleSheet->GetRootProperty()->styleSelectorWillBeAdded.Connect(this, &PropertiesModel::StyleSelectorWillBeAdded);
        styleSheet->GetRootProperty()->styleSelectorWasAdded.Connect(this, &PropertiesModel::StyleSelectorWasAdded);

        styleSheet->GetRootProperty()->styleSelectorWillBeRemoved.Connect(this, &PropertiesModel::StyleSelectorWillBeRemoved);
        styleSheet->GetRootProperty()->styleSelectorWasRemoved.Connect(this, &PropertiesModel::StyleSelectorWasRemoved);

        rootProperty = styleSheet->GetRootProperty();
    }
    endResetModel();
}

QModelIndex PropertiesModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid())
        return createIndex(row, column, rootProperty->GetProperty(row));

    AbstractProperty* property = static_cast<AbstractProperty*>(parent.internalPointer());
    return createIndex(row, column, property->GetProperty(row));
}

QModelIndex PropertiesModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();

    AbstractProperty* property = static_cast<AbstractProperty*>(child.internalPointer());
    AbstractProperty* parent = property->GetParent();

    if (parent == nullptr || parent == rootProperty)
        return QModelIndex();

    if (parent->GetParent())
        return createIndex(parent->GetParent()->GetIndex(parent), 0, parent);
    else
        return createIndex(0, 0, parent);
}

int PropertiesModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return rootProperty ? rootProperty->GetCount() : 0;

    return static_cast<AbstractProperty*>(parent.internalPointer())->GetCount();
}

int PropertiesModel::columnCount(const QModelIndex&) const
{
    return 2;
}

QVariant PropertiesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    AbstractProperty* property = static_cast<AbstractProperty*>(index.internalPointer());
    DAVA::Any value = property->GetValue();
    uint32 flags = property->GetFlags();
    switch (role)
    {
    case Qt::CheckStateRole:
    {
        if (value.CanGet<bool>() && index.column() == 1 && !property->IsBound())
            return value.Get<bool>() ? Qt::Checked : Qt::Unchecked;
    }
    break;

    case Qt::DisplayRole:
    {
        if (index.column() == 0)
            return QVariant(property->GetDisplayName().c_str());
        else if (index.column() == 1)
        {
            QString res = makeQVariant(property);

            if (property->IsBound())
            {
                return QString::fromStdString(property->GetBindingExpression());
            }

            StyleSheetProperty* p = dynamic_cast<StyleSheetProperty*>(property);
            if (p && p->HasTransition())
            {
                const char* interp = GlobalEnumMap<Interpolation::FuncType>::Instance()->ToString(p->GetTransitionFunction());
                res += QString(" (") + QVariant(p->GetTransitionTime()).toString() + " sec., " + interp + ")";
            }

            return res;
        }
    }
    break;

    case Qt::DecorationRole:
        if (property->IsBound() && index.column() == 1)
        {
            return QIcon(GetDataBindingIcon(property->GetBindingUpdateMode()));
        }

        break;

    case BindingRole:
    {
        QMap<QString, QVariant> map;
        map.insert("mode", property->GetBindingUpdateMode());
        map.insert("value", QString::fromStdString(property->GetBindingExpression()));
        return QVariant(map);
    }

    case Qt::ToolTipRole:
    {
        if (property->HasError())
        {
            return QVariant(property->GetErrorString().c_str());
        }
        if (index.column() == 0)
        {
            return QVariant(property->GetName().c_str());
        }

        return makeQVariant(property);
    }

    case Qt::EditRole:
    {
        QVariant var;
        if (index.column() != 0)
        {
            if (property->IsBound())
            {
                var.setValue<Any>(Any(String("=") + property->GetBindingExpression()));
            }
            else
            {
                var.setValue<Any>(value);
            }
        }
        return var;
    }

    case Qt::BackgroundRole:
        if (property->GetType() == AbstractProperty::TYPE_HEADER)
        {
            return accessor->GetGlobalContext()->GetData<DAVA::ThemesSettings>()->GetViewLineAlternateColor();
        }
        break;

    case Qt::FontRole:
    {
        if (property->IsOverriddenLocally() || property->IsReadOnly())
        {
            QFont myFont;
            // We should set font family manually, to set familyResolved flag in font.
            // If we don't do this, Qt will get resolve family almost randomly
            myFont.setFamily(myFont.family());
            myFont.setBold(property->IsOverriddenLocally());
            myFont.setItalic(property->IsReadOnly());
            return myFont;
        }
    }
    break;

    case Qt::TextColorRole:
    {
        if (property->IsBound())
        {
            return GetBoundColor();
        }
        if (property->IsOverriddenLocally() || property->IsReadOnly())
        {
            return accessor->GetGlobalContext()->GetData<DAVA::ThemesSettings>()->GetChangedPropertyColor();
        }
        if (controlNode)
        {
            int32 propertyIndex = property->GetStylePropertyIndex();
            if (propertyIndex != -1)
            {
                bool setByStyle = controlNode->GetControl()->GetStyledPropertySet().test(propertyIndex);
                if (setByStyle)
                {
                    return accessor->GetGlobalContext()->GetData<DAVA::ThemesSettings>()->GetStyleSheetNodeColor();
                }
            }
        }
        if (flags & AbstractProperty::EF_INHERITED)
        {
            return accessor->GetGlobalContext()->GetData<DAVA::ThemesSettings>()->GetPrototypeColor();
        }
    }
    }

    return QVariant();
}

bool PropertiesModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    AbstractProperty* property = static_cast<AbstractProperty*>(index.internalPointer());
    if (property->IsReadOnly())
        return false;

    switch (role)
    {
    case Qt::CheckStateRole:
    {
        if (property->GetValueType() == Type::Instance<bool>() && !property->IsBound())
        {
            Any newVal(value != Qt::Unchecked);
            ChangeProperty(property, newVal);
            UpdateProperty(property);
            return true;
        }
    }
    break;
    case Qt::EditRole:
    {
        Any newVal;

        if (value.canConvert<Any>())
        {
            newVal = value.value<Any>();
        }
        else
        {
            newVal = property->GetValue();
            initAny(newVal, value);
        }

        ChangeProperty(property, newVal);
        UpdateProperty(property);
        return true;
    }

    case BindingRole:
    {
        if (property->IsBindable())
        {
            QMap<QString, QVariant> map = value.toMap();
            auto modeIt = map.find("mode");
            auto valueIt = map.find("value");
            if (modeIt == map.end() || valueIt == map.end())
            {
                ResetProperty(property);
                UpdateProperty(property);
            }
            else
            {
                String expr = (*valueIt).toString().toStdString();
                int32 mode = (*modeIt).toInt();

                ChangeBindingProperty(property, expr, mode);
                UpdateProperty(property);
            }
            return true;
        }
    }
    break;

    case ResetRole:
    {
        ResetProperty(property);
        UpdateProperty(property);
        return true;
    }
    }
    return false;
}

Qt::ItemFlags PropertiesModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    AbstractProperty* prop = static_cast<AbstractProperty*>(index.internalPointer());
    AbstractProperty::ePropertyType propType = prop->GetType();
    bool editable = !prop->IsReadOnly() && (propType == AbstractProperty::TYPE_ENUM || propType == AbstractProperty::TYPE_FLAGS || propType == AbstractProperty::TYPE_VARIANT);
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (index.column() == 1)
    {
        flags |= Qt::ItemIsUserCheckable;

        if (editable)
            flags |= Qt::ItemIsEditable;
    }

    return flags;
}

QVariant PropertiesModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == 0)
            return "Property";
        else
            return "Value";
    }
    return QVariant();
}

const AbstractProperty* PropertiesModel::GetRootProperty() const
{
    return rootProperty;
}

void PropertiesModel::UpdateAllChangedProperties()
{
    for (auto property : changedProperties)
    {
        UpdateProperty(property.Get());
    }
    changedProperties.clear();
}

void PropertiesModel::PropertyChanged(AbstractProperty* property)
{
    changedProperties.insert(RefPtr<AbstractProperty>::ConstructWithRetain(property));
    propertiesUpdater.Update();
}

void PropertiesModel::UpdateProperty(AbstractProperty* property)
{
    QPersistentModelIndex nameIndex = indexByProperty(property, 0);
    QPersistentModelIndex valueIndex = nameIndex.sibling(nameIndex.row(), 1);
    if (nameIndex.isValid() && valueIndex.isValid())
    {
        if (property->IsBound())
        {
            emit dataChanged(nameIndex, valueIndex, QVector<int>() << BindingRole);
        }
        else
        {
            emit dataChanged(nameIndex, valueIndex, QVector<int>() << Qt::DisplayRole);
        }
    }
}

void PropertiesModel::ComponentPropertiesWillBeAdded(RootProperty* root, ComponentPropertiesSection* section, int index)
{
    QModelIndex parentIndex = indexByProperty(root, 0);
    beginInsertRows(parentIndex, index, index);
}

void PropertiesModel::ComponentPropertiesWasAdded(RootProperty* root, ComponentPropertiesSection* section, int index)
{
    endInsertRows();
    QModelIndex modelIndex = indexByProperty(root->GetProperty(index), 0);
    emit ComponentAdded(modelIndex);
}

void PropertiesModel::ComponentPropertiesWillBeRemoved(RootProperty* root, ComponentPropertiesSection* section, int index)
{
    QModelIndex parentIndex = indexByProperty(root, 0);
    beginRemoveRows(parentIndex, index, index);
}

void PropertiesModel::ComponentPropertiesWasRemoved(RootProperty* root, ComponentPropertiesSection* section, int index)
{
    endRemoveRows();
}

void PropertiesModel::StylePropertyWillBeAdded(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index)
{
    QModelIndex parentIndex = indexByProperty(section, 0);
    beginInsertRows(parentIndex, index, index);
}

void PropertiesModel::StylePropertyWasAdded(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index)
{
    endInsertRows();
}

void PropertiesModel::StylePropertyWillBeRemoved(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index)
{
    QModelIndex parentIndex = indexByProperty(section, 0);
    beginRemoveRows(parentIndex, index, index);
}

void PropertiesModel::StylePropertyWasRemoved(StyleSheetPropertiesSection* section, StyleSheetProperty* property, int index)
{
    endRemoveRows();
}

void PropertiesModel::StyleSelectorWillBeAdded(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index)
{
    QModelIndex parentIndex = indexByProperty(section, 0);
    beginInsertRows(parentIndex, index, index);
}

void PropertiesModel::StyleSelectorWasAdded(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index)
{
    endInsertRows();
}

void PropertiesModel::StyleSelectorWillBeRemoved(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index)
{
    QModelIndex parentIndex = indexByProperty(section, 0);
    beginRemoveRows(parentIndex, index, index);
}

void PropertiesModel::StyleSelectorWasRemoved(StyleSheetSelectorsSection* section, StyleSheetSelectorProperty* property, int index)
{
    endRemoveRows();
}

void PropertiesModel::OnStylePropertiesChanged(DAVA::UIControl* control, const DAVA::UIStyleSheetPropertySet& properties)
{
    if (controlNode != nullptr && rootProperty != nullptr && controlNode->GetControl() == control)
    {
        for (int32 index = 0; index < UIStyleSheetPropertyDataBase::STYLE_SHEET_PROPERTY_COUNT; index++)
        {
            if (properties.test(index))
            {
                AbstractProperty* changedProperty = rootProperty->FindPropertyByStyleIndex(index);
                if (changedProperty != nullptr)
                {
                    PropertyChanged(changedProperty);
                }
            }
        }
    }
}

void PropertiesModel::ChangeProperty(AbstractProperty* property, const Any& value)
{
    DAVA::DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* documentData = activeContext->GetData<DocumentData>();
    DVASSERT(documentData != nullptr);

    if (nullptr != controlNode)
    {
        SubValueProperty* subValueProperty = dynamic_cast<SubValueProperty*>(property);
        documentData->BeginBatch(Format("Change Property"));

        ValueProperty* vp = dynamic_cast<ValueProperty*>(property);
        if (vp)
        {
            documentData->ExecCommand<ChangePropertyForceOverrideCommand>(controlNode, vp);
        }

        if (subValueProperty)
        {
            ValueProperty* valueProperty = subValueProperty->GetValueProperty();
            Any newValue = valueProperty->ChangeValueComponent(valueProperty->GetValue(), value, subValueProperty->GetIndex());
            documentData->ExecCommand<ChangePropertyValueCommand>(controlNode, valueProperty, newValue);
        }
        else
        {
            documentData->ExecCommand<ChangePropertyValueCommand>(controlNode, property, value);
        }
        documentData->EndBatch();
    }
    else if (styleSheet)
    {
        documentData->ExecCommand<ChangeStylePropertyCommand>(styleSheet, property, value);
    }
    else
    {
        DVASSERT(false);
    }
}

void PropertiesModel::ChangeBindingProperty(AbstractProperty* property, const DAVA::String& value, int32 mode)
{
    DAVA::DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* documentData = activeContext->GetData<DocumentData>();
    DVASSERT(documentData != nullptr);

    if (controlNode != nullptr)
    {
        documentData->ExecCommand<ChangeBindingCommand>(controlNode, property, value, mode);
    }
    else
    {
        DVASSERT(false);
    }
}

void PropertiesModel::ResetProperty(AbstractProperty* property)
{
    DAVA::DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* documentData = activeContext->GetData<DocumentData>();
    DVASSERT(documentData != nullptr);

    if (nullptr != controlNode)
    {
        documentData->ExecCommand<ChangePropertyValueCommand>(controlNode, property, Any());
    }
    else
    {
        DVASSERT(false);
    }
}

QModelIndex PropertiesModel::indexByProperty(const AbstractProperty* property, int column)
{
    AbstractProperty* parent = property->GetParent();
    if (parent == nullptr)
        return QModelIndex();

    return createIndex(parent->GetIndex(property), column, static_cast<void*>(const_cast<AbstractProperty*>(property)));
}

QString PropertiesModel::makeQVariant(const AbstractProperty* property) const
{
    Any val = property->GetValue();

    if (property->GetType() == AbstractProperty::TYPE_ENUM)
    {
        return QString::fromStdString(property->GetEnumMap()->ToString(val.Cast<int32>()));
    }

    if (property->GetType() == AbstractProperty::TYPE_FLAGS)
    {
        int32 e = val.Get<int32>();
        QString res = "";
        int p = 0;
        while (e)
        {
            if ((e & 0x01) != 0)
            {
                if (!res.isEmpty())
                    res += " | ";
                res += QString::fromStdString(property->GetEnumMap()->ToString(1 << p));
            }
            p++;
            e >>= 1;
        }
        return res;
    }

    if (val.IsEmpty())
    {
        return QString();
    }

    if (val.CanGet<bool>())
    {
        return QString();
    }

    if (val.CanGet<int8>())
    {
        return QVariant(val.Get<int8>()).toString();
    }

    if (val.CanGet<uint8>())
    {
        return QVariant(val.Get<uint8>()).toString();
    }

    if (val.CanGet<int16>())
    {
        return QVariant(val.Get<int16>()).toString();
    }

    if (val.CanGet<uint16>())
    {
        return QVariant(val.Get<uint16>()).toString();
    }

    if (val.CanGet<int32>())
    {
        return QVariant(val.Get<int32>()).toString();
    }

    if (val.CanGet<uint32>())
    {
        return QVariant(val.Get<uint32>()).toString();
    }

    if (val.CanGet<int64>())
    {
        return QVariant(val.Get<int64>()).toString();
    }

    if (val.CanGet<uint64>())
    {
        return QVariant(val.Get<uint64>()).toString();
    }

    if (val.CanGet<float32>())
    {
        return QVariant(val.Get<float32>()).toString();
    }

    if (val.CanGet<float64>())
    {
        return QVariant(val.Get<float64>()).toString();
    }

    if (val.CanGet<String>())
    {
        return DAVA::UnescapeString(StringToQString(val.Get<String>()));
    }

    if (val.CanGet<WideString>())
    {
        DVASSERT(false);
        return DAVA::UnescapeString(WideStringToQString(val.Get<WideString>()));
    }

    if (val.CanGet<FastName>())
    {
        const FastName& fastName = val.Get<FastName>();
        if (fastName.IsValid())
        {
            return StringToQString(fastName.c_str());
        }
        else
        {
            return QString();
        }
    }

    if (val.CanGet<Vector2>())
    {
        Vector2 vec = val.Get<Vector2>();
        return StringToQString(Format("%g; %g", vec.x, vec.y));
    }

    if (val.CanGet<Color>())
    {
        return QColorToHex(DAVA::ColorToQColor(val.Get<Color>()));
    }

    if (val.CanGet<Vector4>())
    {
        Vector4 vec = val.Get<Vector4>();
        return StringToQString(Format("%g; %g; %g; %g", vec.x, vec.y, vec.z, vec.w));
    }

    if (val.CanGet<FilePath>())
    {
        return StringToQString(val.Get<FilePath>().GetStringValue());
    }

    if (val.CanGet<RefPtr<UIControl>>())
    {
        RefPtr<UIControl> c = val.Get<RefPtr<UIControl>>();
        return c.Valid() ? (QString("<") + QString::fromStdString(c->GetName().c_str()) + QString(">")) : "<null>";
    }

    DVASSERT(false);
    return QString();
}

void PropertiesModel::initAny(Any& var, const QVariant& val) const
{
    if (var.IsEmpty())
    {
        // do nothing;
    }
    else if (var.CanGet<bool>())
    {
        var = val.toBool();
    }
    else if (var.CanGet<int32>())
    {
        var = val.toInt();
    }
    else if (var.CanGet<float32>())
    {
        var = val.toFloat();
    }
    else if (var.CanGet<String>())
    {
        var = val.toString().toStdString();
    }
    else if (var.CanGet<WideString>())
    {
        DVASSERT(false);
        var = QStringToWideString(val.toString());
    }
    else if (var.CanGet<FastName>())
    {
        var = FastName(val.toString().toStdString());
    }
    else if (var.CanGet<Vector2>())
    {
        QVector2D vector = val.value<QVector2D>();
        var = Vector2(vector.x(), vector.y());
    }
    else if (var.CanGet<Vector4>())
    {
        QVector4D vector = val.value<QVector4D>();
        var = Vector4(vector.x(), vector.y(), vector.z(), vector.w());
    }
    else
    {
        DVASSERT(false);
    }
}

void PropertiesModel::CleanUp()
{
    if (nullptr != controlNode)
    {
        controlNode->GetRootProperty()->propertyChanged.Disconnect(this);
        controlNode->GetRootProperty()->componentPropertiesWillBeAdded.Disconnect(this);
        controlNode->GetRootProperty()->componentPropertiesWasAdded.Disconnect(this);

        controlNode->GetRootProperty()->componentPropertiesWillBeRemoved.Disconnect(this);
        controlNode->GetRootProperty()->componentPropertiesWasRemoved.Disconnect(this);
    }
    if (nullptr != styleSheet)
    {
        styleSheet->GetRootProperty()->propertyChanged.Disconnect(this);

        styleSheet->GetRootProperty()->stylePropertyWillBeAdded.Disconnect(this);
        styleSheet->GetRootProperty()->stylePropertyWasAdded.Disconnect(this);

        styleSheet->GetRootProperty()->stylePropertyWillBeRemoved.Disconnect(this);
        styleSheet->GetRootProperty()->stylePropertyWasRemoved.Disconnect(this);

        styleSheet->GetRootProperty()->styleSelectorWillBeAdded.Disconnect(this);
        styleSheet->GetRootProperty()->styleSelectorWasAdded.Disconnect(this);

        styleSheet->GetRootProperty()->styleSelectorWillBeRemoved.Disconnect(this);
        styleSheet->GetRootProperty()->styleSelectorWasRemoved.Disconnect(this);
    }
    controlNode = nullptr;
    styleSheet = nullptr;
    rootProperty = nullptr;
}

void PropertiesModel::OnPackageChanged(const DAVA::Any& /*package*/)
{
    propertiesUpdater.Abort();
}

void PropertiesModel::BindFields()
{
    using namespace DAVA;

    fieldBinder.reset(new FieldBinder(accessor));
    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<DocumentData>();
        fieldDescr.fieldName = FastName(DocumentData::packagePropertyName);
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &PropertiesModel::OnPackageChanged));
    }
}

QColor PropertiesModel::GetErrorBgColor() const
{
    DAVA::ThemesSettings::eTheme theme = accessor->GetGlobalContext()->GetData<DAVA::ThemesSettings>()->GetTheme();
    return theme == DAVA::ThemesSettings::Light ? QColor(0xff, 0x88, 0x88) : QColor(0x88, 0x00, 0x00);
}

QColor PropertiesModel::GetBoundColor() const
{
    DAVA::ThemesSettings::eTheme theme = accessor->GetGlobalContext()->GetData<DAVA::ThemesSettings>()->GetTheme();
    return theme == DAVA::ThemesSettings::Light ? QColor(0x85, 0x5F, 0xF0) : QColor(0xB1, 0xAB, 0xFF);
}

QString PropertiesModel::GetDataBindingIcon(DAVA::int32 bindingUpdateMode) const
{
    switch (bindingUpdateMode)
    {
    case UIDataBindingComponent::MODE_READ:
        return ":/Icons/link-r.png";
    case UIDataBindingComponent::MODE_WRITE:
        return ":/Icons/link-w.png";
    case UIDataBindingComponent::MODE_READ_WRITE:
        return ":/Icons/link-rw.png";
    }
    DVASSERT(false);
    return "";
}
