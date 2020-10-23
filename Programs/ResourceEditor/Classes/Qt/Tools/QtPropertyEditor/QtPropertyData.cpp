#include "QtPropertyData.h"
#include "QtPropertyModel.h"
#include "QtPropertyDataValidator.h"

QtPropertyData::ChildKey::ChildKey(const QtPropertyData* child_)
    : child(child_)
{
}

bool QtPropertyData::ChildKey::operator<(const ChildKey& other) const
{
    if (child->name != other.child->name)
        return child->name < other.child->name;

    const DAVA::MetaInfo* thisMeta = child->MetaInfo();
    const DAVA::MetaInfo* otherMeta = other.child->MetaInfo();
    if (thisMeta != otherMeta)
        return thisMeta < otherMeta;

    return child->IsEnabled() < other.child->IsEnabled();
}

bool QtPropertyData::ChildKey::operator!=(const ChildKey& other) const
{
    return !(*this == other);
}

bool QtPropertyData::ChildKey::operator==(const ChildKey& other) const
{
    return child->name == other.child->name &&
    child->MetaInfo() == other.child->MetaInfo() &&
    child->IsEnabled() == other.child->IsEnabled();
}

QtPropertyData::QtPropertyData(const DAVA::FastName& name_)
    : name(name_)
{
    childrenData.reserve(16);
    mergedData.reserve(128);
}

QtPropertyData::QtPropertyData(const DAVA::FastName& name_, const QVariant& value)
    : curValue(value)
    , name(name_)
{
    childrenData.reserve(16);
    mergedData.reserve(128);
}

QtPropertyData::~QtPropertyData()
{
    DVASSERT(!updatingValue && "Property can't be removed during it update process");
    childrenData.clear();
    mergedData.clear();

    for (int i = 0; i < optionalButtons.size(); i++)
    {
        optionalButtons.at(i)->deleteLater();
    }

    optionalButtons.clear();
}

QVariant QtPropertyData::data(int role) const
{
    QVariant ret;

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:
        ret = GetAlias();
        if (!ret.isValid())
        {
            ret = GetValue();
        }
        break;
    case Qt::ToolTipRole:
        ret = GetToolTip();
        if (!ret.isValid())
        {
            ret = data(Qt::DisplayRole);
        }
        break;
    case Qt::CheckStateRole:
        if (GetFlags() & Qt::ItemIsUserCheckable)
        {
            ret = GetValue();
            if (!ret.isValid())
            {
                ret = Qt::PartiallyChecked;
            }
            else
            {
                ret = GetValue().toBool() ? Qt::Checked : Qt::Unchecked;
            }
        }
        break;
    case Qt::FontRole:
    {
        ret = style.value(role);
        if (ret.isValid() && ret.canConvert<QFont>())
        {
            QFont font = ret.value<QFont>();
            // We should set font family manually, to set familyResolved flag in font.
            // If we don't do this, Qt will get resolve family almost randomly
            font.setFamily(font.family());
            ret = QVariant::fromValue(font);
        }
    }
    break;
    case Qt::DecorationRole:
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
        ret = style.value(role);
        break;
    default:
        break;
    }

    return ret;
}

bool QtPropertyData::setData(const QVariant& value, int role)
{
    bool ret = false;

    switch (role)
    {
    case Qt::EditRole:
        SetValue(value, QtPropertyData::VALUE_EDITED);
        ret = true;
        break;
    case Qt::CheckStateRole:
        (value.toInt() == Qt::Unchecked) ? SetValue(false, QtPropertyData::VALUE_EDITED) : SetValue(true, QtPropertyData::VALUE_EDITED);
        ret = true;
        break;
    case Qt::FontRole:
    case Qt::DecorationRole:
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
        style.insert(role, value);
        ret = true;
        break;
    default:
        break;
    }

    return ret;
}

void QtPropertyData::BuildCurrentValue()
{
    // Build value
    const QVariant master = GetValueInternal();
    bool isAllEqual = true;

    isValuesMerged = false;
    ForeachMergedItem([&master, &isAllEqual](QtPropertyData* item) {
        isAllEqual = (master == item->GetValue());
        return isAllEqual;
    });

    curValue = isAllEqual ? master : QVariant();
    isValuesMerged = isAllEqual;

    // Update Qt MVC properties
    if (!isAllEqual)
    {
        auto it = style.find(Qt::DecorationRole);
        if (it != style.end())
        {
            *it = QVariant();
        }
    }
}

QVariant QtPropertyData::GetValue() const
{
    QtPropertyData* self = const_cast<QtPropertyData*>(this);

    bool hasValue = curValue.isValid() || !curValue.isNull();

    if (hasValue)
    {
        self->UpdateValue();
    }

    self->BuildCurrentValue();
    return curValue;
}

bool QtPropertyData::IsMergedDataEqual() const
{
    return isValuesMerged;
}

void QtPropertyData::ForeachMergedItem(DAVA::Function<bool(QtPropertyData*)> const& functor) const
{
    for (const std::unique_ptr<QtPropertyData>& item : mergedData)
    {
        if (functor(item.get()) == false)
            break;
    }
}

int QtPropertyData::GetMergedItemCount() const
{
    return static_cast<int>(!mergedData.empty());
}

void QtPropertyData::SetValue(const QVariant& value, ValueChangeReason reason)
{
    QVariant oldValue = curValue;

    {
        updatingValue = true;
        SCOPE_EXIT
        {
            updatingValue = false;
        };

        auto setValueFunctor = [value, reason](QtPropertyData* item) {
            QtPropertyDataValidator* mergedValidator = item->GetValidator();
            QVariant validatedValue = value;

            if (reason == VALUE_EDITED && mergedValidator != nullptr)
            {
                if (!mergedValidator->Validate(validatedValue))
                {
                    return true;
                }
            }

            item->SetValueInternal(validatedValue);
            return true;
        };

        ForeachMergedItem(setValueFunctor);
        setValueFunctor(this);
    }

    // and get what was really set
    // it can differ from input "value"
    // (example: we are trying to set 10, but accepted range is 0-5
    //   value is 10
    //   curValue becomes 0-5)
    UpdateValue();
    curValue = GetValueInternal();

    if (curValue != oldValue)
    {
        updatingValue = true;

        UpdateDown();
        UpdateUp();

        EmitDataChanged(reason);

        updatingValue = false;
    }
}

bool QtPropertyData::UpdateValue(bool force)
{
    bool ret = false;

    if (!updatingValue)
    {
        updatingValue = true;

        if (UpdateValueInternal() || force)
        {
            EmitDataChanged(VALUE_SOURCE_CHANGED);
            ret = true;
        }

        updatingValue = false;
    }

    return ret;
}

QVariant QtPropertyData::GetAlias() const
{
    // this will force update internalValue if
    // it source was changed
    GetValue();
    const QVariant alias = IsMergedDataEqual() ? GetValueAlias() : QVariant();

    return alias;
}

const DAVA::FastName& QtPropertyData::GetName() const
{
    return name;
}

QString QtPropertyData::GetPath() const
{
    QString path = name.c_str();

    // search top level parent
    const QtPropertyData* parent = this;
    while (NULL != parent->Parent())
    {
        parent = parent->Parent();
        path = QString(parent->name.c_str()) + "/" + path;
    }

    return path;
}

void QtPropertyData::SetColorButtonIcon(const QIcon& icon)
{
    auto it = std::find_if(optionalButtons.begin(), optionalButtons.end(), [](const QtPropertyToolButton* btn) {
        return btn->objectName() == "colorButton";
    });
    if (it != optionalButtons.end())
    {
        (*it)->setIcon(icon);
    }
}

void QtPropertyData::SetIcon(const QIcon& icon)
{
    setData(QVariant(icon), Qt::DecorationRole);
}

QIcon QtPropertyData::GetIcon() const
{
    return qvariant_cast<QIcon>(data(Qt::DecorationRole));
}

QFont QtPropertyData::GetFont() const
{
    return qvariant_cast<QFont>(data(Qt::FontRole));
}

void QtPropertyData::SetFont(const QFont& font)
{
    setData(QVariant(font), Qt::FontRole);
}

QBrush QtPropertyData::GetBackground() const
{
    return qvariant_cast<QBrush>(data(Qt::BackgroundRole));
}

void QtPropertyData::SetBackground(const QBrush& brush)
{
    setData(QVariant(brush), Qt::BackgroundRole);
}

QBrush QtPropertyData::GetForeground() const
{
    return qvariant_cast<QBrush>(data(Qt::ForegroundRole));
}

void QtPropertyData::SetForeground(const QBrush& brush)
{
    setData(QVariant(brush), Qt::ForegroundRole);
}

void QtPropertyData::ResetStyle()
{
    style.remove(Qt::ForegroundRole);
    style.remove(Qt::BackgroundRole);
    style.remove(Qt::FontRole);
}

Qt::ItemFlags QtPropertyData::GetFlags() const
{
    return curFlags;
}

void QtPropertyData::SetFlags(Qt::ItemFlags flags)
{
    curFlags = flags;
}

void QtPropertyData::SetCheckable(bool checkable)
{
    (checkable) ? (curFlags |= Qt::ItemIsUserCheckable) : (curFlags &= ~Qt::ItemIsUserCheckable);
}

bool QtPropertyData::IsCheckable() const
{
    return (curFlags & Qt::ItemIsUserCheckable);
}

void QtPropertyData::SetChecked(bool checked)
{
    setData(QVariant(checked), Qt::CheckStateRole);
}

bool QtPropertyData::IsChecked() const
{
    return data(Qt::CheckStateRole).toBool();
}

void QtPropertyData::SetEditable(bool editable)
{
    (editable) ? (curFlags |= Qt::ItemIsEditable) : (curFlags &= ~Qt::ItemIsEditable);
    UpdateOWState();

    EmitDataChanged(STATE_CHANGED);
}

bool QtPropertyData::IsEditable() const
{
    return (curFlags & Qt::ItemIsEditable);
}

void QtPropertyData::SetEnabled(bool enabled)
{
    (enabled) ? (curFlags |= Qt::ItemIsEnabled) : (curFlags &= ~Qt::ItemIsEnabled);
    UpdateOWState();

    EmitDataChanged(STATE_CHANGED);
}

void QtPropertyData::UpdateOWState()
{
    bool isItemEditable = IsEditable();
    bool isItemEnabled = IsEnabled();

    for (int i = 0; i < optionalButtons.size(); ++i)
    {
        optionalButtons[i]->UpdateState(isItemEnabled, isItemEditable);
    }
}

void QtPropertyData::SetUserData(UserData* data)
{
    userData.reset(data);
    EmitDataChanged(VALUE_SET);
}

QtPropertyData::UserData* QtPropertyData::GetUserData() const
{
    return userData.get();
}

void QtPropertyData::SetToolTip(const QVariant& toolTip)
{
    tooltipValue = toolTip;
}

QVariant QtPropertyData::GetToolTip() const
{
    return tooltipValue;
}

const DAVA::MetaInfo* QtPropertyData::MetaInfo() const
{
    return nullptr;
}

bool QtPropertyData::IsEnabled() const
{
    return (curFlags & Qt::ItemIsEnabled);
}

QtPropertyModel* QtPropertyData::GetModel() const
{
    return model;
}

void QtPropertyData::Merge(std::unique_ptr<QtPropertyData>&& data)
{
    DVASSERT(data);

    if (!data->IsMergable())
    {
        data.reset();
        return;
    }

    data->parent = nullptr;

    DAVA::Vector<std::unique_ptr<QtPropertyData>> children;
    data->ChildrenExtract(children);
    mergedData.emplace_back(std::move(data));

    for (std::unique_ptr<QtPropertyData>& item : children)
    {
        MergeChild(std::move(item));
    }
    children.clear();

    UpdateValue(true);
}

void QtPropertyData::MergeChild(std::unique_ptr<QtPropertyData>&& data)
{
    DVASSERT(data);

    ChildKey key(data.get());
    TChildMap::const_iterator iter = keyToDataMap.find(key);
    if (iter != keyToDataMap.end())
    {
        childrenData[iter->second]->Merge(std::move(data));
    }
    else
    {
        size_t position = childrenData.size();
        for (size_t i = 0; i < childrenData.size(); ++i)
        {
            if (childrenData[i]->GetName() == data->GetName())
            {
                position = i;
                break;
            }
        }

        ChildInsert(std::move(data), static_cast<int>(position));
    }
}

bool QtPropertyData::IsMergable() const
{
    // Must be overrided, if data should not be merged
    // For example, if child data modification will affect parent value

    return true;
}

void QtPropertyData::SetModel(QtPropertyModel* model_)
{
    model = model_;

    for (std::unique_ptr<QtPropertyData>& child : childrenData)
    {
        DVASSERT(child != nullptr);
        child->SetModel(model);
    }
}

void QtPropertyData::SetValidator(QtPropertyDataValidator* value)
{
    validator.reset(value);
}

QtPropertyDataValidator* QtPropertyData::GetValidator() const
{
    return validator.get();
}

QWidget* QtPropertyData::CreateEditor(QWidget* parent, const QStyleOptionViewItem& option) const
{
    return CreateEditorInternal(parent, option);
}

bool QtPropertyData::EditorDone(QWidget* editor)
{
    return EditorDoneInternal(editor);
}

bool QtPropertyData::SetEditorData(QWidget* editor)
{
    return SetEditorDataInternal(editor);
}

void QtPropertyData::EmitDataChanged(ValueChangeReason reason)
{
    if (NULL != model)
    {
        model->DataChanged(this, reason);
    }
}

void QtPropertyData::UpdateUp()
{
    if (NULL != parent)
    {
        parent->UpdateValue();
        parent->UpdateUp();
    }
}

void QtPropertyData::UpdateDown()
{
    for (std::unique_ptr<QtPropertyData>& child : childrenData)
    {
        DVASSERT(child != nullptr);
        child->UpdateValue();
        child->UpdateDown();
    }
}

QtPropertyData* QtPropertyData::Parent() const
{
    return parent;
}

void QtPropertyData::ChildAdd(std::unique_ptr<QtPropertyData>&& data)
{
    if (data == nullptr)
        return;

    int position = ChildCount();
    QtPropertyModel::InsertionGuard insertGuard(model, this, position, position);
    data->parent = this;
    data->SetModel(model);
    data->SetOWViewport(optionalButtonsViewport);
    keyToDataMap.emplace(ChildKey(data.get()), childrenData.size());
    childrenData.push_back(std::move(data));
}

void QtPropertyData::ChildrenAdd(DAVA::Vector<std::unique_ptr<QtPropertyData>>&& data)
{
    if (data.empty())
        return;

    int currentSize = static_cast<int>(childrenData.size());
    int newSize = static_cast<int>(currentSize + data.size());
    QtPropertyModel::InsertionGuard insertGuard(model, this, currentSize, newSize - 1);

    childrenData.reserve(newSize);

    for (std::unique_ptr<QtPropertyData>& item : data)
    {
        DVASSERT(item != nullptr);
        item->parent = this;
        item->SetModel(model);
        item->SetOWViewport(optionalButtonsViewport);

        keyToDataMap.emplace(ChildKey(item.get()), childrenData.size());
        childrenData.push_back(std::move(item));
    }
    data.clear();
}

void QtPropertyData::ChildInsert(std::unique_ptr<QtPropertyData>&& data, int pos)
{
    if (data == nullptr)
        return;

    int insertPosition = static_cast<int>(childrenData.size());
    QtPropertyModel::InsertionGuard insertGuard(model, this, insertPosition, insertPosition);

    data->parent = this;
    data->SetModel(model);
    data->SetOWViewport(optionalButtonsViewport);

    size_t position = static_cast<size_t>(pos);
    if (position < childrenData.size())
    {
        childrenData.insert(childrenData.begin() + position, std::move(data));
        RefillSearchIndex();
    }
    else
    {
        keyToDataMap.emplace(ChildKey(data.get()), childrenData.size());
        childrenData.push_back(std::move(data));
    }
}

int QtPropertyData::ChildCount() const
{
    return static_cast<int>(childrenData.size());
}

QtPropertyData* QtPropertyData::ChildGet(int i) const
{
    DVASSERT(static_cast<size_t>(i) < childrenData.size());
    return childrenData[static_cast<size_t>(i)].get();
}

QtPropertyData* QtPropertyData::ChildGet(const DAVA::FastName& key) const
{
    for (const std::unique_ptr<QtPropertyData>& item : childrenData)
    {
        if (item->name == key)
            return item.get();
    }

    return nullptr;
}

int QtPropertyData::ChildIndex(const QtPropertyData* data) const
{
    TChildMap::const_iterator iter = keyToDataMap.find(ChildKey(data));
    if (iter != keyToDataMap.end())
    {
        if (iter->first.child == data)
        {
            return static_cast<int>(iter->second);
        }

        auto iter = std::find_if(childrenData.begin(), childrenData.end(), [data](const std::unique_ptr<QtPropertyData>& child)
                                 {
                                     return data == child.get();
                                 });
        if (iter != childrenData.end())
        {
            return std::distance(childrenData.begin(), iter);
        }
    }

    return -1;
}

void QtPropertyData::ChildrenExtract(DAVA::Vector<std::unique_ptr<QtPropertyData>>& children)
{
    if (childrenData.empty())
        return;

    QtPropertyModel::DeletionGuard deletionGuard(model, this, 0, static_cast<int>(childrenData.size() - 1));
    children = std::move(childrenData);
    keyToDataMap.clear();
}

void QtPropertyData::ChildRemove(const QtPropertyData* data)
{
    TChildMap::const_iterator iter = keyToDataMap.find(ChildKey(data));
    if (iter != keyToDataMap.end())
    {
        int index = static_cast<int>(iter->second);
        DVASSERT(index < static_cast<int>(childrenData.size()));
        QtPropertyModel::DeletionGuard deletionGuard(model, this, index, index);
        childrenData.erase(childrenData.begin() + index);
        RefillSearchIndex();
    }
}

void QtPropertyData::ChildRemoveAll()
{
    if (childrenData.empty())
        return;

    QtPropertyModel::DeletionGuard deletionGuard(model, this, 0, static_cast<int>(childrenData.size() - 1));
    ResetChildren();
}

void QtPropertyData::ResetChildren()
{
    childrenData.clear();
    keyToDataMap.clear();
}

void QtPropertyData::FinishTreeCreation()
{
    for (std::unique_ptr<QtPropertyData>& child : childrenData)
    {
        child->FinishTreeCreation();
    }
}

int QtPropertyData::GetButtonsCount() const
{
    return optionalButtons.size();
}

QtPropertyToolButton* QtPropertyData::GetButton(int index)
{
    QtPropertyToolButton* ret = NULL;

    if (index >= 0 && index < optionalButtons.size())
    {
        ret = optionalButtons.at(index);
    }

    return ret;
}

QtPropertyToolButton* QtPropertyData::AddButton(QtPropertyToolButton::StateVariant stateVariant /* = QtPropertyToolButton::ACTIVE_ALWAYS */)
{
    QtPropertyToolButton* button = new QtPropertyToolButton(this, optionalButtonsViewport);

    optionalButtons.append(button);
    button->stateVariant = stateVariant;
    button->setGeometry(0, 0, 18, 18);
    button->setAttribute(Qt::WA_NoSystemBackground, true);
    button->hide();

    UpdateOWState();

    return button;
}

void QtPropertyData::RemButton(QtPropertyToolButton* button)
{
    QVector<QtPropertyToolButton*>::iterator iter = std::find(optionalButtons.begin(), optionalButtons.end(), button);
    if (iter != optionalButtons.end())
    {
        delete *iter;
        optionalButtons.erase(iter);
    }
}

QWidget* QtPropertyData::GetOWViewport() const
{
    return optionalButtonsViewport;
}

void QtPropertyData::SetOWViewport(QWidget* viewport)
{
    optionalButtonsViewport = viewport;

    for (int i = 0; i < optionalButtons.size(); ++i)
    {
        if (NULL != optionalButtons[i])
        {
            optionalButtons[i]->setParent(viewport);
        }
    }

    for (size_t i = 0; i < childrenData.size(); i++)
    {
        childrenData[i]->SetOWViewport(viewport);
    }
}

std::unique_ptr<DAVA::Command> QtPropertyData::CreateLastCommand() const
{
    // can be re-implemented by sub-class
    return std::unique_ptr<DAVA::Command>();
}

QVariant QtPropertyData::GetValueInternal() const
{
    // should be re-implemented by sub-class

    return curValue;
}

bool QtPropertyData::UpdateValueInternal()
{
    return false;
}

QVariant QtPropertyData::GetValueAlias() const
{
    // should be re-implemented by sub-class

    return QVariant();
}

void QtPropertyData::SetTempValue(const QVariant& value)
{
    auto setValueFunctor = [](QtPropertyData* data, QVariant value) {
        QtPropertyDataValidator* validator = data->GetValidator();
        if (validator == nullptr || validator->Validate(value))
        {
            data->SetTempValueInternal(value);
        }
    };

    ForeachMergedItem([&setValueFunctor, &value](QtPropertyData* item) {
        setValueFunctor(item, value);
        return true;
    });

    setValueFunctor(this, value);
}

void QtPropertyData::SetValueInternal(const QVariant& value)
{
    // should be re-implemented by sub-class

    curValue = value;
}

void QtPropertyData::SetTempValueInternal(QVariant const& value)
{
    // should be re-implemented by sub-class
    Q_UNUSED(value);
}

QWidget* QtPropertyData::CreateEditorInternal(QWidget* parent, const QStyleOptionViewItem& option) const
{
    // should be re-implemented by sub-class

    return NULL;
}

bool QtPropertyData::EditorDoneInternal(QWidget* editor)
{
    // should be re-implemented by sub-class
    return false;
}

bool QtPropertyData::SetEditorDataInternal(QWidget* editor)
{
    // should be re-implemented by sub-class
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// QtPropertyToolButton
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

QtPropertyToolButton::QtPropertyToolButton(QtPropertyData* data, QWidget* parent /* = 0 */)
    : QToolButton(parent)
    , eventsPassThrought(false)
    , overlayed(false)
    , propertyData(data)
    , stateVariant(ACTIVE_ALWAYS)
{
}

QtPropertyToolButton::~QtPropertyToolButton()
{
}

QtPropertyData* QtPropertyToolButton::GetPropertyData() const
{
    return propertyData;
}

bool QtPropertyToolButton::event(QEvent* event)
{
    int type = event->type();

    if (eventsPassThrought)
    {
        if (type != QEvent::Enter &&
            type != QEvent::Leave &&
            type != QEvent::MouseMove)
        {
            QToolButton::event(event);
        }

        return false;
    }

    return QToolButton::event(event);
}

QtPropertyToolButton::StateVariant QtPropertyToolButton::GetStateVariant() const
{
    return stateVariant;
}

void QtPropertyToolButton::SetStateVariant(StateVariant state)
{
    stateVariant = state;
}

void QtPropertyToolButton::UpdateState(bool itemIsEnabled, bool itemIsEditable)
{
    bool enabled = false;
    switch (stateVariant)
    {
    case QtPropertyToolButton::ACTIVE_ALWAYS:
        enabled = true;
        break;
    case QtPropertyToolButton::ACTIVE_WHEN_ITEM_IS_ENABLED:
        enabled = itemIsEnabled;
        break;
    case QtPropertyToolButton::ACTIVE_WHEN_ITEM_IS_EDITABLE:
        enabled = itemIsEditable;
        break;
    case QtPropertyToolButton::ACTIVE_WHEN_ITEM_IS_EDITABLE_OR_ENABLED:
        enabled = (itemIsEnabled || itemIsEditable);
        break;
    case QtPropertyToolButton::ACTIVE_WHEN_ITEM_IS_EDITABLE_AND_ENABLED:
        enabled = (itemIsEnabled && itemIsEditable);
        break;
    default:
        break;
    }

    setEnabled(enabled);
}

void QtPropertyData::RefillSearchIndex()
{
    keyToDataMap.clear();
    for (size_t i = 0; i < childrenData.size(); ++i)
    {
        const std::unique_ptr<QtPropertyData>& data = childrenData[i];

        const auto emplacedPair = keyToDataMap.emplace(ChildKey(data.get()), i);
        DVASSERT(emplacedPair.second);
    }
}
