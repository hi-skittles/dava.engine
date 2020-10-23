#include "PropertiesTreeItemDelegate.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QPainter>
#include <QAction>
#include <QStylePainter>
#include <QApplication>
#include <QToolButton>
#include <QEvent>
#include <QMouseEvent>
#include <QSortFilterProxyModel>
#include <QAbstractItemModel>

#include "Model/ControlProperties/AbstractProperty.h"
#include "Utils/QtDavaConvertion.h"
#include "Vector2PropertyDelegate.h"
#include "EnumPropertyDelegate.h"
#include "PropertiesModel.h"
#include "StringPropertyDelegate.h"
#include "ComboPropertyDelegate.h"
#include "FilePathPropertyDelegate.h"
#include "FMODEventPropertyDelegate.h"
#include "ColorPropertyDelegate.h"
#include "IntegerPropertyDelegate.h"
#include "FloatPropertyDelegate.h"
#include "BoolPropertyDelegate.h"
#include "ResourceFilePropertyDelegate.h"
#include "Vector4PropertyDelegate.h"
#include "FontPropertyDelegate.h"
#include "TablePropertyDelegate.h"
#include "BindingPropertyDelegate.h"
#include "CompletionsProviderForScrollBar.h"
#include "CompletionsProviderForUIReflection.h"
#include "PredefinedCompletionsProvider.h"
#include "Modules/LegacySupportModule/Private/Project.h"

using namespace DAVA;

PropertiesTreeItemDelegate::PropertiesTreeItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    propertyItemDelegates[AbstractProperty::TYPE_ENUM] = new EnumPropertyDelegate(this);
    anyItemDelegates[Type::Instance<Vector2>()] = new Vector2PropertyDelegate(this);
    anyItemDelegates[Type::Instance<String>()] = new StringPropertyDelegate(this);
    anyItemDelegates[Type::Instance<FastName>()] = new StringPropertyDelegate(this);
    anyItemDelegates[Type::Instance<Color>()] = new ColorPropertyDelegate(this);
    anyItemDelegates[Type::Instance<WideString>()] = new StringPropertyDelegate(this);
    anyItemDelegates[Type::Instance<FilePath>()] = new FilePathPropertyDelegate(this);
    anyItemDelegates[Type::Instance<int8>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<uint8>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<int16>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<uint16>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<int32>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<uint32>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<int64>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<uint64>()] = new IntegerPropertyDelegate(this);
    anyItemDelegates[Type::Instance<float32>()] = new FloatPropertyDelegate(this);
    anyItemDelegates[Type::Instance<bool>()] = new BoolPropertyDelegate(this);
    anyItemDelegates[Type::Instance<Vector4>()] = new Vector4PropertyDelegate(this);

    const QList<QString> gfxExtensions{ Project::GetGraphicsFileExtension() };
    const QList<QString> particleExtensions{ Project::Get3dFileExtension() };
    const QList<QString> spineSkeletonExtensions{ ".json", ".skel" };
    const QList<QString> spineAtlasExtensions{ ".atlas" };
    const QList<QString> dataBindingModelExtension{ ".model" };
    const QList<QString> dataBindingSourceDataExtensions{ ".model", ".yaml" };
    const QList<QString> uiExtensions{ ".yaml" };
    const QList<QString> luaExtensions{ ".lua" };
    const QList<QString> fontExtensions{ ".ttf", ".otf", ".fnt", ".fntconf" };

    QStringList formulaCompletions;
    formulaCompletions << "childrenSum"
                       << "maxChild"
                       << "firstChild"
                       << "lastChild"
                       << "content"
                       << "parent"
                       << "parentRest"
                       << "parentLine"
                       << "min(parentRest, content)"
                       << "max(parent, childrenSum)"
                       << "visibilityMargins.left"
                       << "visibilityMargins.right"
                       << "visibilityMargins.top"
                       << "visibilityMargins.bottom"
                       << "safeAreaInsets.left"
                       << "safeAreaInsets.right"
                       << "safeAreaInsets.top"
                       << "safeAreaInsets.bottom";

    propertyNameTypeItemDelegates[PropertyPath("*", "sprite")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "mask")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "detail")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "gradient")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "contour")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "effectPath")] = new ResourceFilePropertyDelegate(particleExtensions, "/3d/", this, false);
    propertyNameTypeItemDelegates[PropertyPath("*", "font")] = new FontPropertyDelegate(this);
    propertyNameTypeItemDelegates[PropertyPath("ScrollBarDelegate", "delegate")] = new ComboPropertyDelegate(this, std::make_unique<CompletionsProviderForScrollBar>(), true);

    propertyNameTypeItemDelegates[PropertyPath("*", "bg-sprite")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "bg-mask")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "bg-detail")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "bg-gradient")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "bg-contour")] = new ResourceFilePropertyDelegate(gfxExtensions, "/Gfx/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "text-fontName")] = new FontPropertyDelegate(this);
    propertyNameTypeItemDelegates[PropertyPath("*", "particleEffect-effectPath")] = new ResourceFilePropertyDelegate(particleExtensions, "/3d/", this, false);

    propertyNameTypeItemDelegates[PropertyPath("Sound", "*")] = new FMODEventPropertyDelegate(this);

    propertyNameTypeItemDelegates[PropertyPath("*", "sound-touchDown")] = new FMODEventPropertyDelegate(this);
    propertyNameTypeItemDelegates[PropertyPath("*", "sound-touchUpInside")] = new FMODEventPropertyDelegate(this);
    propertyNameTypeItemDelegates[PropertyPath("*", "sound-touchUpOutside")] = new FMODEventPropertyDelegate(this);
    propertyNameTypeItemDelegates[PropertyPath("*", "sound-touchValueChanged")] = new FMODEventPropertyDelegate(this);

    bindingPropertyDelegate = new BindingPropertyDelegate(this);
    propertyNameTypeItemDelegates[PropertyPath("UIDataSourceComponent", "dataFile")] = new ResourceFilePropertyDelegate(dataBindingSourceDataExtensions, "/UI/", this, false);
    propertyNameTypeItemDelegates[PropertyPath("UIDataListComponent", "cellPackage")] = new ResourceFilePropertyDelegate(dataBindingSourceDataExtensions, "/UI/", this, false);
    propertyNameTypeItemDelegates[PropertyPath("UIDataViewModelComponent", "viewModel")] = new ResourceFilePropertyDelegate(dataBindingModelExtension, "/UI/", this, false);

    propertyNameTypeItemDelegates[PropertyPath("*", "aliases")] = new TablePropertyDelegate(QList<QString>({ "Alias", "Xml" }), this);

    propertyNameTypeItemDelegates[PropertyPath("SizePolicy", "horizontalFormula")] = new ComboPropertyDelegate(this, std::make_unique<PredefinedCompletionsProvider>(formulaCompletions), true);
    propertyNameTypeItemDelegates[PropertyPath("SizePolicy", "verticalFormula")] = new ComboPropertyDelegate(this, std::make_unique<PredefinedCompletionsProvider>(formulaCompletions), true);

    propertyNameTypeItemDelegates[PropertyPath("UISpineComponent", "skeletonPath")] = new ResourceFilePropertyDelegate(spineSkeletonExtensions, "", this, false);
    propertyNameTypeItemDelegates[PropertyPath("UISpineComponent", "atlasPath")] = new ResourceFilePropertyDelegate(spineAtlasExtensions, "/Gfx/", this, false);
    propertyNameTypeItemDelegates[PropertyPath("UISpineComponent", "animationName")] = new ComboPropertyDelegate(this, std::make_unique<CompletionsProviderForUIReflection>("animationsNames", "UISpineComponent"), false);
    propertyNameTypeItemDelegates[PropertyPath("UISpineComponent", "skinName")] = new ComboPropertyDelegate(this, std::make_unique<CompletionsProviderForUIReflection>("skinsNames", "UISpineComponent"), false);
    propertyNameTypeItemDelegates[PropertyPath("UISpineAttachControlsToBonesComponent", "bonesBinds")] = new TablePropertyDelegate(QList<QString>({ "Bone", "Control" }), this);

    propertyNameTypeItemDelegates[PropertyPath("UITextComponent", "fontName")] = new FontPropertyDelegate(this);
    propertyNameTypeItemDelegates[PropertyPath("UITextComponent", "fontPath")] = new ResourceFilePropertyDelegate(fontExtensions, "/Fonts/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("UITextField", "fontPath")] = new ResourceFilePropertyDelegate(fontExtensions, "/Fonts/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("*", "text-fontPath")] = new ResourceFilePropertyDelegate(fontExtensions, "/Fonts/", this, true);

    propertyNameTypeItemDelegates[PropertyPath("UIScriptComponent", "luaScriptPath")] = new ResourceFilePropertyDelegate(luaExtensions, "/Lua/", this, true);

    propertyNameTypeItemDelegates[PropertyPath("UIFlowViewComponent", "viewYaml")] = new ResourceFilePropertyDelegate(uiExtensions, "/UI/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("UIFlowControllerComponent", "luaScriptPath")] = new ResourceFilePropertyDelegate(luaExtensions, "/Lua/", this, true);
    propertyNameTypeItemDelegates[PropertyPath("UIFlowTransitionComponent", "transitions")] = new TablePropertyDelegate(QList<QString>({ "Event", "Action", "New State or Event" }), this);
    propertyNameTypeItemDelegates[PropertyPath("UIFlowStateComponent", "services")] = new TablePropertyDelegate(QList<QString>({ "Alias", "Typename" }), this);

    propertyNameTypeItemDelegates[PropertyPath("UIShortcutEventComponent", "shortcuts")] = new TablePropertyDelegate(QList<QString>({ "Event", "Shortcut" }), this);
}

PropertiesTreeItemDelegate::~PropertiesTreeItemDelegate()
{
    for (auto iter = propertyItemDelegates.begin(); iter != propertyItemDelegates.end(); ++iter)
    {
        SafeDelete(iter.value());
    }

    for (auto iter = anyItemDelegates.begin(); iter != anyItemDelegates.end(); ++iter)
    {
        SafeDelete(iter.value());
    }

    for (auto iter = propertyNameTypeItemDelegates.begin(); iter != propertyNameTypeItemDelegates.end(); ++iter)
    {
        SafeDelete(iter.value());
    }

    SafeDelete(bindingPropertyDelegate);
}

QWidget* PropertiesTreeItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QModelIndex sourceIndex = GetSourceIndex(index, nullptr);
    AbstractPropertyDelegate* currentDelegate = GetCustomItemDelegateForIndex(sourceIndex);
    if (currentDelegate)
    {
        PropertyWidget* editorWidget = new PropertyWidget(currentDelegate, parent);
        editorWidget->setObjectName(QString::fromUtf8("editorWidget"));
        QWidget* editor = currentDelegate->createEditor(editorWidget, context, option, sourceIndex);
        if (!editor)
        {
            DAVA::SafeDelete(editorWidget);
        }
        else
        {
            editorWidget->editWidget = editor;
            editorWidget->setFocusPolicy(Qt::WheelFocus);

            QHBoxLayout* horizontalLayout = new QHBoxLayout(editorWidget);
            horizontalLayout->setSpacing(1);
            horizontalLayout->setContentsMargins(0, 0, 0, 0);
            horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
            editorWidget->setLayout(horizontalLayout);

            editorWidget->setAutoFillBackground(true);
            editorWidget->setFocusProxy(editor);

            editorWidget->layout()->addWidget(editor);

            QList<QAction*> actions;
            currentDelegate->enumEditorActions(editorWidget, sourceIndex, actions);

            foreach (QAction* action, actions)
            {
                QToolButton* toolButton = new QToolButton(editorWidget);
                toolButton->setDefaultAction(action);
                toolButton->setIconSize(QSize(15, 15));
                toolButton->setFocusPolicy(Qt::StrongFocus);
                editorWidget->layout()->addWidget(toolButton);
            }
        }

        return editorWidget;
    }

    if (sourceIndex.data(Qt::EditRole).type() == QVariant::Bool)
        return NULL;

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void PropertiesTreeItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QModelIndex sourceIndex = GetSourceIndex(index, nullptr);

    PropertyWidget* propertyWidget = DynamicTypeCheck<PropertyWidget*>(editor);
    AbstractPropertyDelegate* currentDelegate = propertyWidget->delegate;

    AbstractProperty* property = static_cast<AbstractProperty*>(sourceIndex.internalPointer());

    if (property->IsBound() == (currentDelegate == bindingPropertyDelegate))
    {
        return currentDelegate->setEditorData(editor, sourceIndex);
    }

    QStyledItemDelegate::setEditorData(editor, index);
}

void PropertiesTreeItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QModelIndex sourceIndex = GetSourceIndex(index, model);
    AbstractPropertyDelegate* currentDelegate = GetCustomItemDelegateForIndex(sourceIndex);
    if (currentDelegate)
    {
        currentDelegate->setModelData(editor, model, index);
        return;
    }

    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit && !lineEdit->isModified())
        return;

    QStyledItemDelegate::setModelData(editor, model, index);
}

AbstractPropertyDelegate* PropertiesTreeItemDelegate::GetCustomItemDelegateForIndex(const QModelIndex& index) const
{
    AbstractProperty* property = static_cast<AbstractProperty*>(index.internalPointer());
    if (property)
    {
        if (property->IsBound())
        {
            return bindingPropertyDelegate;
        }

        auto prop_iter = propertyItemDelegates.find(property->GetType());
        if (prop_iter != propertyItemDelegates.end())
        {
            return prop_iter.value();
        }

        QString parentName;
        AbstractProperty* parentProperty = property->GetParent();
        if (parentProperty)
        {
            parentName = QString::fromStdString(parentProperty->GetName());
        }

        QMap<PropertyPath, AbstractPropertyDelegate*>::const_iterator propNameIt;
        propNameIt = propertyNameTypeItemDelegates.find(PropertyPath(parentName, QString::fromStdString(property->GetName())));
        if (propNameIt == propertyNameTypeItemDelegates.end())
        {
            propNameIt = propertyNameTypeItemDelegates.find(PropertyPath("*", QString::fromStdString(property->GetName())));
        }

        if (propNameIt == propertyNameTypeItemDelegates.end())
        {
            propNameIt = propertyNameTypeItemDelegates.find(PropertyPath(parentName, "*"));
        }

        if (propNameIt != propertyNameTypeItemDelegates.end())
        {
            return propNameIt.value();
        }

        auto varIt = anyItemDelegates.find(property->GetValueType());
        if (varIt != anyItemDelegates.end())
        {
            return varIt.value();
        }

        varIt = anyItemDelegates.find(property->GetValueType()->Decay());
        if (varIt != anyItemDelegates.end())
        {
            return varIt.value();
        }
    }

    return nullptr;
}

void PropertiesTreeItemDelegate::SetProject(const Project* project)
{
    context.project = project;
}

void PropertiesTreeItemDelegate::SetInvoker(DAVA::OperationInvoker* invoker_)
{
    context.invoker = invoker_;
}

DAVA::OperationInvoker* PropertiesTreeItemDelegate::GetInvoker()
{
    return context.invoker;
}

void PropertiesTreeItemDelegate::SetAccessor(DAVA::ContextAccessor* accessor_)
{
    context.accessor = accessor_;
}

void PropertiesTreeItemDelegate::emitCommitData(QWidget* editor)
{
    emit commitData(editor);
}

void PropertiesTreeItemDelegate::emitCloseEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
    emit closeEditor(editor, hint);
}

void PropertiesTreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const
{
    QStyleOptionViewItemV3 opt = option;

    QStyledItemDelegate::paint(painter, opt, index);

    opt.palette.setCurrentColorGroup(QPalette::Active);
    QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
    painter->save();
    painter->setPen(QPen(color));

    int right = (option.direction == Qt::LeftToRight) ? option.rect.right() : option.rect.left();
    painter->drawLine(right, option.rect.y(), right, option.rect.bottom());
    painter->restore();
}

QModelIndex PropertiesTreeItemDelegate::GetSourceIndex(QModelIndex index, QAbstractItemModel* itemModel) const
{
    QModelIndex sourceIndex = index;
    const QAbstractItemModel* model = itemModel ? itemModel : index.model();
    const QSortFilterProxyModel* sortModel = dynamic_cast<const QSortFilterProxyModel*>(model);
    if (sortModel != nullptr)
    {
        sourceIndex = sortModel->mapToSource(index);
    }

    return sourceIndex;
}

PropertyWidget::PropertyWidget(AbstractPropertyDelegate* delegate_, QWidget* parent /*= NULL*/)
    : QWidget(parent)
    , editWidget(NULL)
    , delegate(delegate_)
{
}

bool PropertyWidget::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::ShortcutOverride:
        if (static_cast<QObject*>(editWidget)->event(e))
            return true;
        break;

    case QEvent::InputMethod:
        return static_cast<QObject*>(editWidget)->event(e);

    default:
        break;
    }

    return QWidget::event(e);
}

void PropertyWidget::keyPressEvent(QKeyEvent* e)
{
    static_cast<QObject*>(editWidget)->event(e);
}

void PropertyWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton)
        return;

    e->ignore();
}

void PropertyWidget::mouseReleaseEvent(QMouseEvent* e)
{
    e->accept();
}

void PropertyWidget::focusInEvent(QFocusEvent* e)
{
    static_cast<QObject*>(editWidget)->event(e);
    QWidget::focusInEvent(e);
}

void PropertyWidget::focusOutEvent(QFocusEvent* e)
{
    static_cast<QObject*>(editWidget)->event(e);
    QWidget::focusOutEvent(e);
}
