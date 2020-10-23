#include "Classes/SceneTree/SceneTreeModule.h"
#include "Classes/SceneTree/Private/SceneTreeContextMenu.h"
#include "Classes/SceneTree/Private/SceneTreeFilterModelV2.h"
#include "Classes/SceneTree/Private/SceneTreeModelV2.h"
#include "Classes/SceneTree/Private/SceneTreeRoles.h"
#include "Classes/SceneTree/Private/SceneTreeSystem.h"
#include "Classes/SceneTree/Private/SceneTreeView.h"
#include "Classes/SceneTree/Private/CreateEntitySupportDefault.h"

#include <REPlatform/Global/SceneTree/CreateEntitySupport.h>
#include <REPlatform/Global/SceneTree/SceneTreeFiltration.h>

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/SceneHelper.h>
#include <REPlatform/Scene/Systems/CollisionSystem.h>

#include <QtTools/Updaters/LazyUpdater.h>

#include <TArc/Controls/LineEdit.h>
#include <TArc/Controls/ReflectedButton.h>
#include <TArc/Controls/ContentFilter/ContentFilter.h>
#include <TArc/Controls/Widget.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/AnyQMetaType.h>
#include <TArc/Qt/QtIcon.h>
#include <TArc/Qt/QtSize.h>
#include <TArc/Qt/QtString.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Utils/ScopedValueGuard.h>
#include <TArc/Utils/Utils.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/ActionUtils.h>

#include <Base/Any.h>
#include <Base/RefPtr.h>
#include <Base/Set.h>
#include <Base/Vector.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Render/Texture.h>

#include <QPointer>
#include <QToolBar>
#include <QBoxLayout>
#include <QMenu>
#include "Debug/DVAssert.h"

namespace SceneTreeModuleDetail
{
struct AvailableFilterKey
{
    DAVA::Vector<const DAVA::ReflectedType*> types;
    DAVA::Vector<FilterState> states;
    DAVA::String nameFiltration;
    DAVA::int32 uniqueID = 0;

    bool operator==(const AvailableFilterKey& other) const
    {
        return nameFiltration == other.nameFiltration &&
        uniqueID == other.uniqueID &&
        types == other.types &&
        states == other.states;
    }
};

class SceneTreeGlobalData : public DAVA::TArcDataNode
{
public:
    SceneTreeGlobalData()
    {
        using namespace DAVA;
        Vector<const ReflectedType*> ungroupedFilters;
        Map<String, Vector<const ReflectedType*>> filters;
        Function<void(const Type*)> unpackFn = [&unpackFn, &filters, &ungroupedFilters](const DAVA::Type* baseType) {
            const TypeInheritance* inheritance = baseType->GetInheritance();
            if (inheritance == nullptr)
            {
                return;
            }

            for (const TypeInheritance::Info& derived : inheritance->GetDerivedTypes())
            {
                const ReflectedType* type = ReflectedTypeDB::GetByType(derived.type);
                if (derived.type->IsAbstract() == false)
                {
                    DVASSERT(type != nullptr);
                    DVASSERT(type->GetCtor(derived.type->Pointer()) != nullptr);

                    const M::Group* groupMeta = GetReflectedTypeMeta<M::Group>(type);
                    if (groupMeta != nullptr)
                    {
                        filters[groupMeta->groupName].push_back(type);
                    }
                    else
                    {
                        ungroupedFilters.push_back(type);
                    }
                }

                unpackFn(derived.type);
            }
        };
        const Type* baseType = Type::Instance<DAVA::SceneTreeFilterBase>();
        unpackFn(baseType);

        std::sort(ungroupedFilters.begin(), ungroupedFilters.end(), [](const DAVA::ReflectedType* t1, const DAVA::ReflectedType* t2) {
            return t1->GetPermanentName() < t2->GetPermanentName();
        });

        ContentFilter::AvailableFiltersGroup* rootGroup = new ContentFilter::AvailableFiltersGroup();
        filterTypes.reset(rootGroup);

        auto createFiltersFn = [](const DAVA::Vector<const ReflectedType*>& types, ContentFilter::AvailableFiltersGroup* group) {
            for (const ReflectedType* type : types)
            {
                String filterName = type->GetPermanentName();
                DVASSERT(filterName.empty() == false);

                const M::DisplayName* displayName = GetReflectedTypeMeta<M::DisplayName>(type);
                if (displayName != nullptr)
                {
                    filterName = displayName->displayName;
                }

                ContentFilter::AvailableFilter* filter = new ContentFilter::AvailableFilter();
                filter->filterName = FastName(filterName);
                filter->userDefined = false;
                AvailableFilterKey key;
                key.types.push_back(type);
                key.states.push_back(FilterState());
                filter->key = key;
                group->filters.push_back(std::unique_ptr<ContentFilter::AvailableFilterBase>(filter));
            }
        };

        for (const auto& node : filters)
        {
            ContentFilter::AvailableFiltersGroup* group = new ContentFilter::AvailableFiltersGroup();
            group->name = FastName(node.first);
            createFiltersFn(node.second, group);
            rootGroup->filters.push_back(std::unique_ptr<ContentFilter::AvailableFilterBase>(group));
        }

        createFiltersFn(ungroupedFilters, rootGroup);
        rootGroup->filters.push_back(std::make_unique<ContentFilter::SeparatorTag>());
    }

    void LoadUserDefinedFilters(const DAVA::PropertiesItem& filtersBlock)
    {
        using namespace DAVA;

        int32 count = filtersBlock.Get<int32>("userDefinedFiltersCount", 0);
        for (int32 i = 0; i < count; ++i)
        {
            String headerKey = DAVA::Format("userDefinedFilter_%d", i);
            PropertiesItem currentFilterBlock = filtersBlock.CreateSubHolder(headerKey);
            int32 subTypesCount = currentFilterBlock.Get<int32>("subTypesCount", 0);

            AvailableFilterKey filterKey;
            filterKey.nameFiltration = currentFilterBlock.Get("nameFiltrationKey", String());
            filterKey.uniqueID = uniqueIDCounter++;
            filterKey.types.reserve(subTypesCount);
            filterKey.states.reserve(subTypesCount);

            for (int32 subTypeIndex = 0; subTypeIndex < subTypesCount; ++subTypeIndex)
            {
                String permanentName = currentFilterBlock.Get(Format("subTypeName_%d", subTypeIndex), String());
                const ReflectedType* subType = ReflectedTypeDB::GetByPermanentName(permanentName);
                if (subType != nullptr)
                {
                    filterKey.types.push_back(subType);
                }

                FilterState state;
                state.enabled = currentFilterBlock.Get(Format("subTypeEnabled_%d", subTypeIndex), true);
                state.inverted = currentFilterBlock.Get(Format("subTypeInverted_%d", subTypeIndex), false);
                filterKey.states.push_back(state);
            }

            QByteArray userDefinedFilterNameBase = currentFilterBlock.Get<QByteArray>("filterName", QByteArray());

            std::unique_ptr<ContentFilter::AvailableFilter> userDefinedFilterInstance = std::make_unique<ContentFilter::AvailableFilter>();
            userDefinedFilterInstance->filterName = FastName(QString::fromUtf8(userDefinedFilterNameBase).toStdString());
            userDefinedFilterInstance->key = filterKey;
            userDefinedFilterInstance->userDefined = true;
            filterTypes->filters.push_back(std::move(userDefinedFilterInstance));
        }
    }

    void SaveUserDefinedFilters(DAVA::PropertiesItem& filtersBlock)
    {
        using namespace DAVA;
        Vector<ContentFilter::AvailableFilter*> filterToSerialize;

        for (const std::unique_ptr<ContentFilter::AvailableFilterBase>& base : filterTypes->filters)
        {
            ContentFilter::AvailableFilter* filter = dynamic_cast<ContentFilter::AvailableFilter*>(base.get());
            if (filter != nullptr && filter->userDefined == true)
            {
                filterToSerialize.push_back(filter);
            }
        }

        filtersBlock.Set("userDefinedFiltersCount", static_cast<int32>(filterToSerialize.size()));
        for (size_t i = 0; i < filterToSerialize.size(); ++i)
        {
            ContentFilter::AvailableFilter* currentFilter = filterToSerialize[i];
            const AvailableFilterKey& filterKey = currentFilter->key.Get<AvailableFilterKey>();

            String headerKey = DAVA::Format("userDefinedFilter_%d", static_cast<int32>(i));
            PropertiesItem currentFilterBlock = filtersBlock.CreateSubHolder(headerKey);

            currentFilterBlock.Set("nameFiltrationKey", filterKey.nameFiltration);
            currentFilterBlock.Set("subTypesCount", static_cast<int32>(filterKey.types.size()));

            for (size_t subTypeIndex = 0; subTypeIndex < filterKey.types.size(); ++subTypeIndex)
            {
                currentFilterBlock.Set(Format("subTypeName_%d", subTypeIndex), filterKey.types[subTypeIndex]->GetPermanentName());
                FilterState state = filterKey.states[subTypeIndex];
                currentFilterBlock.Set(Format("subTypeEnabled_%d", subTypeIndex), state.enabled);
                currentFilterBlock.Set(Format("subTypeInverted_%d", subTypeIndex), state.inverted);
            }

            currentFilterBlock.Set("filterName", QString(currentFilter->filterName.c_str()).toUtf8());
        }
    }

    std::unique_ptr<DAVA::ContentFilter::AvailableFiltersGroup> filterTypes;
    DAVA::int32 uniqueIDCounter = 1;

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneTreeGlobalData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<SceneTreeGlobalData>::Begin()
        .End();
    }
};

class SceneTreeData : public DAVA::TArcDataNode
{
public:
    SceneTreeSystem* system = nullptr;
    SceneTreeModelV2* model = nullptr;
    SceneTreeFilterModelV2* filterModel = nullptr;

    QPointer<QItemSelectionModel> selectionModel = nullptr;

    DAVA::Set<QPersistentModelIndex> expandedIndexList;
    DAVA::Set<QPersistentModelIndex> savedIndexListOnFiltration;

    QString immediateFilterText;
    LazyUpdater* immediateFilterUpdater = nullptr;

    const QString& GetFilter() const
    {
        return filterModel->GetFilter();
    }

    void SetFilter(const QString& newFilter)
    {
        if (GetFilter().isEmpty() == true)
        {
            SaveExpandedList();
        }

        immediateFilterText = newFilter;
        filterModel->SetFilter(newFilter);

        if (GetFilter().isEmpty() == true)
        {
            LoadExpandedList();
        }
        else
        {
            ExpandAll();
        }
    }

    void SetImmediateFilter(const QString& immediateFilter)
    {
        immediateFilterText = immediateFilter;
        immediateFilterUpdater->Update(300);
    }

    void ProcessImmediateFilter()
    {
        if (GetFilter() != immediateFilterText)
        {
            SetFilter(immediateFilterText);
        }
    }

    void SaveExpandedList()
    {
        savedIndexListOnFiltration.clear();
        for (QPersistentModelIndex index : expandedIndexList)
        {
            QPersistentModelIndex sourceIndex = filterModel->mapToSource(index);
            savedIndexListOnFiltration.insert(sourceIndex);
        }
    }

    void LoadExpandedList()
    {
        expandedIndexList.clear();
        for (QPersistentModelIndex index : savedIndexListOnFiltration)
        {
            QPersistentModelIndex filteredIndex = filterModel->mapFromSource(index);
            if (filteredIndex.isValid() == true)
            {
                expandedIndexList.insert(filteredIndex);
            }
        }
    }

    void ExpandAll()
    {
        expandedIndexList.clear();
        expandedIndexList.insert(QModelIndex());
    }

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneTreeData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<SceneTreeData>::Begin()
        .End();
    }
};

class EntityCreationData : public DAVA::TArcDataNode
{
public:
    std::unique_ptr<DAVA::BaseEntityCreator> rootCreator;
    DAVA::EntityCreator* activeCreator = nullptr;

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(EntityCreationData)
    {
        DAVA::ReflectionRegistrator<EntityCreationData>::Begin()
        .End();
    }
};

} // namespace SceneTreeModuleDetail

namespace DAVA
{
template <>
struct AnyCompare<SceneTreeModuleDetail::AvailableFilterKey>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        return v1.Get<SceneTreeModuleDetail::AvailableFilterKey>() == v2.Get<SceneTreeModuleDetail::AvailableFilterKey>();
    }
};
} // namespace DAVA

SceneTreeModule::~SceneTreeModule()
{
    using namespace SceneTreeModuleDetail;
    DAVA::ContextAccessor* accessor = GetAccessor();
    DAVA::PropertiesItem pi = accessor->CreatePropertiesNode("sceneTreeCustomFilters");
    accessor->GetGlobalContext()->GetData<SceneTreeGlobalData>()->SaveUserDefinedFilters(pi);
}

void SceneTreeModule::OnContextCreated(DAVA::DataContext* context)
{
    using namespace SceneTreeModuleDetail;

    DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
    DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

    SceneTreeData* data = new SceneTreeData();
    data->system = new SceneTreeSystem(scene);
    data->system->syncIsNecessary.Connect(DAVA::MakeFunction(this, &SceneTreeModule::OnSyncRequested));
    data->system->DisableSystem();
    scene->AddSystem(data->system, 0, DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS);
    data->system->EnableSystem();

    DAVA::ContextAccessor* accessor = GetAccessor();

    data->model = new SceneTreeModelV2(scene, accessor);

    data->filterModel = new SceneTreeFilterModelV2(accessor);
    data->filterModel->setSourceModel(data->model);

    data->selectionModel = new QItemSelectionModel(data->filterModel);
    auto immediateFilterSetFn = [this, accessor]() {
        DAVA::DataContext* ctx = accessor->GetActiveContext();
        if (ctx == nullptr)
        {
            return;
        }

        DAVA::SelectableGroup selection = ctx->GetData<DAVA::SelectionData>()->GetSelection();

        SceneTreeData* data = ctx->GetData<SceneTreeData>();
        data->ProcessImmediateFilter();
        OnSceneSelectionChanged(selection);
    };

    data->immediateFilterUpdater = new LazyUpdater(immediateFilterSetFn, data->filterModel);

    connections.AddConnection(data->selectionModel, &QItemSelectionModel::selectionChanged,
                              DAVA::Bind(&SceneTreeModule::OnSceneTreeSelectionChanged, this, DAVA::_1, DAVA::_2, data->selectionModel));

    context->CreateData(std::unique_ptr<DAVA::TArcDataNode>(data));
}

void SceneTreeModule::OnContextDeleted(DAVA::DataContext* context)
{
    using namespace SceneTreeModuleDetail;

    DAVA::SceneData* sceneData = context->GetData<DAVA::SceneData>();
    DAVA::SceneEditor2* scene = sceneData->GetScene().Get();

    SceneTreeData* data = context->GetData<SceneTreeData>();
    DAVA::SafeDelete(data->filterModel);
    DAVA::SafeDelete(data->model);
    if (data->selectionModel.isNull() == false)
    {
        QItemSelectionModel* object = data->selectionModel.data();
        DAVA::SafeDelete(object);
    }

    data->system->syncIsNecessary.DisconnectAll();
    scene->RemoveSystem(data->system);
    DAVA::SafeDelete(data->system);

    context->DeleteData<SceneTreeData>();
}

void SceneTreeModule::OnContextWillBeChanged(DAVA::DataContext* current, DAVA::DataContext* newOne)
{
    SceneTreeModuleDetail::EntityCreationData* data = GetAccessor()->GetGlobalContext()->GetData<SceneTreeModuleDetail::EntityCreationData>();
    if (data->activeCreator != nullptr)
    {
        data->activeCreator->Cancel();
        data->activeCreator = nullptr;
    }
}

void SceneTreeModule::PostInit()
{
    using namespace SceneTreeModuleDetail;
    using namespace DAVA;

    DAVA::ContextAccessor* accessor = GetAccessor();

    RegisterPredefinedFilters();
    SceneTreeGlobalData* sceneTreeGlobalData = new SceneTreeGlobalData();
    sceneTreeGlobalData->LoadUserDefinedFilters(accessor->CreatePropertiesNode("sceneTreeCustomFilters"));
    accessor->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArcDataNode>(sceneTreeGlobalData));

    const QString sceneTreeBlockName = QStringLiteral("Scene Tree");
    ActionPlacementInfo actionsPlacementInfo(CreateInvisiblePoint());

    fieldBinder.reset(new FieldBinder(accessor));

    FieldDescriptor selectionDescr;
    selectionDescr.type = ReflectedTypeDB::Get<SelectionData>();
    selectionDescr.fieldName = FastName(SelectionData::selectionPropertyName);
    fieldBinder->BindField(selectionDescr, MakeFunction(this, &SceneTreeModule::OnSceneSelectionChanged));

    EntityCreationData* entityCreationData = new EntityCreationData();
    entityCreationData->rootCreator = CreateEntityCreationTree();
    accessor->GetGlobalContext()->CreateData(std::unique_ptr<DAVA::TArcDataNode>(entityCreationData));

    UI* ui = GetUI();

    Widget* sceneTreeWidget = new Widget();
    QVBoxLayout* widgetLayout = new QVBoxLayout(sceneTreeWidget->ToWidgetCast());
    widgetLayout->setSpacing(0);
    widgetLayout->setMargin(0);
    sceneTreeWidget->SetLayout(widgetLayout);

    FieldDescriptor sceneDescr;
    sceneDescr.fieldName = FastName(DAVA::SceneData::scenePropertyName);
    sceneDescr.type = ReflectedTypeDB::Get<DAVA::SceneData>();
    auto isActionEnabled = [](const Any& v) {
        return Any(v.IsEmpty() == false);
    };

    QtAction* expandAction = new QtAction(accessor, SharedIcon(":/QtIcons/expand_all.png"), QStringLiteral("Expand All"));
    expandAction->setToolTip(expandAction->text());
    expandAction->SetStateUpdationFunction(QtAction::Enabled, sceneDescr, isActionEnabled);
    connections.AddConnection(expandAction, &QAction::triggered, MakeFunction(this, &SceneTreeModule::ExpandAll));

    KeyBindableActionInfo expandInfo;
    expandInfo.blockName = sceneTreeBlockName;
    MakeActionKeyBindable(expandAction, expandInfo);
    ui->AddAction(DAVA::mainWindowKey, actionsPlacementInfo, expandAction);

    QtAction* collapseAction = new QtAction(accessor, SharedIcon(":/QtIcons/collapse_all.png"), QStringLiteral("Collapse All"));
    collapseAction->SetStateUpdationFunction(QtAction::Enabled, sceneDescr, isActionEnabled);
    collapseAction->setToolTip(collapseAction->text());
    connections.AddConnection(collapseAction, &QAction::triggered, MakeFunction(this, &SceneTreeModule::CollapseAll));

    KeyBindableActionInfo collapseInfo;
    collapseInfo.blockName = sceneTreeBlockName;
    MakeActionKeyBindable(collapseAction, collapseInfo);
    ui->AddAction(DAVA::mainWindowKey, actionsPlacementInfo, collapseAction);

    QtAction* addEntityAction = nullptr;
    QtAction* removeSelectionAction = nullptr;

    Reflection reflectedModel = Reflection::Create(ReflectedObject(this));
    // Toolbar
    {
        QToolBar* toolBar = new QToolBar(sceneTreeWidget->ToWidgetCast());
        toolBar->setObjectName("SceneTreeToolBar");
        toolBar->setMaximumHeight(32);
        addEntityAction = new QtAction(accessor, SharedIcon(":/QtIcons/add_green.png"), QStringLiteral("Add Entity"));
        addEntityAction->SetStateUpdationFunction(QtAction::Enabled, sceneDescr, isActionEnabled);

        QMenu* addEntityMenu = new QMenu();
        EntityCreatorsGroup* rootGroup = dynamic_cast<EntityCreatorsGroup*>(entityCreationData->rootCreator.get());
        DVASSERT(rootGroup != nullptr);
        for (std::unique_ptr<DAVA::BaseEntityCreator>& baseCreator : rootGroup->creatorsGroup)
        {
            BuildCreateMenu(baseCreator.get(), addEntityMenu);
        }

        addEntityAction->setMenu(addEntityMenu);

        removeSelectionAction = new QtAction(accessor, SharedIcon(":/QtIcons/remove.png"), QStringLiteral("Remove selection"));
        removeSelectionAction->SetStateUpdationFunction(QtAction::Enabled, selectionDescr, [](const Any& v) {
            if (v.IsEmpty())
            {
                return false;
            }

            DVASSERT(v.CanGet<SelectableGroup>());
            const SelectableGroup& selection = v.Get<SelectableGroup>();
            return selection.GetSize() > 0;
        });
        connections.AddConnection(removeSelectionAction, &QAction::triggered, [this]() {
            DataContext* ctx = GetAccessor()->GetActiveContext();
            DVASSERT(ctx != nullptr);
            DAVA::RemoveSelection(ctx->GetData<DAVA::SceneData>()->GetScene().Get());
        });

        KeyBindableActionInfo removeSelectionInfo;
        removeSelectionInfo.blockName = sceneTreeBlockName;
        removeSelectionInfo.defaultShortcuts << QKeySequence(Qt::Key_Delete) << QKeySequence(Qt::CTRL + Qt::Key_Backspace);
        removeSelectionInfo.readOnly = true;
        MakeActionKeyBindable(removeSelectionAction, removeSelectionInfo);
        ui->AddAction(DAVA::mainWindowKey, actionsPlacementInfo, removeSelectionAction);

        QtAction* reloadSelectedTextures = new QtAction(accessor, SharedIcon(":/QtIcons/reloadtextures.png"), QStringLiteral("Reload textures in selected Entities"));
        reloadSelectedTextures->SetStateUpdationFunction(QtAction::Enabled, selectionDescr, [](const Any& v) {
            if (v.IsEmpty())
            {
                return false;
            }

            DVASSERT(v.CanGet<SelectableGroup>());
            const SelectableGroup& selection = v.Get<SelectableGroup>();
            return selection.GetSize() > 0;
        });

        KeyBindableActionInfo reloadSelectedTexturesInfo;
        reloadSelectedTexturesInfo.blockName = sceneTreeBlockName;
        MakeActionKeyBindable(reloadSelectedTextures, reloadSelectedTexturesInfo);
        ui->AddAction(DAVA::mainWindowKey, actionsPlacementInfo, reloadSelectedTextures);

        connections.AddConnection(reloadSelectedTextures, &QAction::triggered, DAVA::MakeFunction(this, &SceneTreeModule::ReloadTexturesInSelected));

        toolBar->addAction(addEntityAction);
        QToolButton* addEntityButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(addEntityAction));
        DVASSERT(addEntityButton != nullptr);
        addEntityButton->setPopupMode(QToolButton::InstantPopup);
        toolBar->addAction(removeSelectionAction);
        toolBar->addAction(expandAction);
        toolBar->addAction(collapseAction);
        toolBar->addAction(reloadSelectedTextures);
        widgetLayout->addWidget(toolBar);
    }

    // Top-Level Menu
    {
        QMenu* createEntityMenu = new QMenu(QStringLiteral("Entity"));
        createEntityMenu->addAction(addEntityAction);
        createEntityMenu->addAction(removeSelectionAction);

        ActionPlacementInfo placement(CreateMenuPoint("", { InsertionParams::eInsertionMethod::BeforeItem, "Scene" }));
        ui->AddAction(DAVA::mainWindowKey, placement, createEntityMenu->menuAction());
    }

    // Filter edit
    {
        ContentFilter::Params p(accessor, ui, DAVA::mainWindowKey);
        p.fields[ContentFilter::Fields::FilterText] = "filterText";
        p.fields[ContentFilter::Fields::ImmediateText] = "immediateTextChanged";
        p.fields[ContentFilter::Fields::Enabled] = "hasActiveContext";
        ContentFilter::FilterFieldNames names;
        names.nameRole = FastName(DAVA::SceneTreeFilterBase::titleFieldName);
        names.enabledRole = FastName(DAVA::SceneTreeFilterBase::enabledFieldName);
        names.inversedRole = FastName(DAVA::SceneTreeFilterBase::inversedFieldName);
        p.fields[ContentFilter::Fields::SingleFilterDescriptor].BindConstValue(names);
        p.fields[ContentFilter::Fields::FiltersChain] = "filtersChain";
        p.fields[ContentFilter::Fields::AddFilterToChain] = "addFilterToChain";
        p.fields[ContentFilter::Fields::RemoveFilterFromChain] = "removeFilterFromChain";
        p.fields[ContentFilter::Fields::AvailableFilters] = "getAvailableFilters";
        p.fields[ContentFilter::Fields::SaveCurrentFiltersChain] = "saveCurrentChain";

        ContentFilter* filterWidget = new ContentFilter(p, reflectedModel, sceneTreeWidget->ToWidgetCast());
        filterWidget->ToWidgetCast()->findChild<QLineEdit*>()->setObjectName("SceneTreeFilterTextEdit");

        sceneTreeWidget->AddControl(filterWidget);
    }

    // SceneTreeView
    {
        SceneTreeView::Params params(GetAccessor(), GetUI(), DAVA::mainWindowKey);
        params.fields[SceneTreeView::Fields::DataModel] = "dataModel";
        params.fields[SceneTreeView::Fields::SelectionModel] = "selectionModel";
        params.fields[SceneTreeView::Fields::ExpandedIndexList] = "expandedIndexList";
        params.fields[SceneTreeView::Fields::DoubleClicked] = "itemDoubleClicked";
        params.fields[SceneTreeView::Fields::ContextMenuRequested] = "contextMenuRequested";
        params.fields[SceneTreeView::Fields::DropExecuted] = "dropExecuted";

        SceneTreeView* view = new SceneTreeView(params, GetAccessor(), reflectedModel);
        view->SetObjectName("SceneTreeView");
        view->AddAction(collapseAction);
        view->AddAction(expandAction);
        view->AddAction(removeSelectionAction);
        sceneTreeWidget->AddControl(view);

        {
            QAction* action = new QAction("Inverse collapse state of selected", view->ToWidgetCast());
            view->AddAction(action);
            connections.AddConnection(action, &QAction::triggered, DAVA::MakeFunction(this, &SceneTreeModule::OnInverseCollapsing));
            KeyBindableActionInfo info;
            info.blockName = sceneTreeBlockName;
            info.defaultShortcuts << QKeySequence(Qt::Key_X);
            MakeActionKeyBindable(action, info);
            ui->AddAction(DAVA::mainWindowKey, actionsPlacementInfo, action);
        }
    }

    DockPanelInfo info;
    info.title = QStringLiteral("Scene Tree");
    info.tabbed = false;
    info.area = Qt::LeftDockWidgetArea;

    PanelKey panelKey(info.title, info);
    ui->AddControlView(DAVA::mainWindowKey, panelKey, sceneTreeWidget);

    RegisterOperation(SetSceneTreeFilter.ID, this, &SceneTreeModule::OnFilterChanged);
    RegisterOperation(ReloadTexturesInSelectedOperation.ID, this, &SceneTreeModule::ReloadTexturesInSelected);
}

QAbstractItemModel* SceneTreeModule::GetDataModel() const
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return nullptr;
    }

    return ctx->GetData<SceneTreeData>()->filterModel;
}

QItemSelectionModel* SceneTreeModule::GetSelectionModel() const
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return nullptr;
    }

    return ctx->GetData<SceneTreeData>()->selectionModel;
}

void SceneTreeModule::OnFilterChanged(const DAVA::String& newFilter)
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    SceneTreeData* data = ctx->GetData<SceneTreeData>();
    data->SetImmediateFilter(QString::fromStdString(newFilter));
}

void SceneTreeModule::OnSceneSelectionChanged(const DAVA::Any& value)
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    SceneTreeData* sceneTreeData = ctx->GetData<SceneTreeData>();

    DAVA::ScopedValueGuard<bool> guard(inSelectionSync, true);

    DVASSERT(value.CanGet<DAVA::SelectableGroup>());
    const DAVA::SelectableGroup& currentSelection = value.Get<DAVA::SelectableGroup>();

    DAVA::Map<QModelIndex, DAVA::Vector<QModelIndex>> selectionMap;

    for (const DAVA::Selectable& object : currentSelection.GetContent())
    {
        QModelIndex objectIndex = sceneTreeData->filterModel->mapFromSource(sceneTreeData->model->GetIndexByObject(object));
        if (objectIndex.isValid())
        {
            selectionMap[objectIndex.parent()].push_back(objectIndex);
        }
    }

    QItemSelection itemSelection;
    for (auto iter = selectionMap.begin(); iter != selectionMap.end(); ++iter)
    {
        DAVA::Vector<QModelIndex>& selectionRows = iter->second;
        DVASSERT(selectionRows.empty() == false);

        std::sort(selectionRows.begin(), selectionRows.end(), [](const QModelIndex& left, const QModelIndex& right) {
            return left.row() < right.row();
        });

        QModelIndex startIndex = selectionRows.front();
        QModelIndex endIndex = startIndex;
        QItemSelectionModel::SelectionFlags command = QItemSelectionModel::Select;

        for (size_t i = 1; i < selectionRows.size(); ++i)
        {
            QModelIndex currentIndex = selectionRows[i];
            if (currentIndex.row() - endIndex.row() != 1)
            {
                QItemSelection subRange(startIndex, endIndex);
                itemSelection.merge(subRange, QItemSelectionModel::Select);
                startIndex = currentIndex;
                endIndex = currentIndex;
            }
            else
            {
                endIndex = currentIndex;
            }
        }

        QItemSelection subRange(startIndex, endIndex);
        itemSelection.merge(subRange, QItemSelectionModel::Select);
    }

    sceneTreeData->selectionModel->select(itemSelection, QItemSelectionModel::ClearAndSelect);
    executor.DelayedExecute([this]() {
        DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
        if (ctx != nullptr)
        {
            ctx->GetData<SceneTreeData>()->filterModel->Refilter();
        }
    });
}

void SceneTreeModule::OnSceneTreeSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/, QItemSelectionModel* selectionModel)
{
    SCOPED_VALUE_GUARD(bool, inSelectionSync, true, void());

    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    SceneTreeData* sceneTreeData = ctx->GetData<SceneTreeData>();
    DVASSERT(sceneTreeData->selectionModel = selectionModel);

    DAVA::SelectionData* selectionData = ctx->GetData<DAVA::SelectionData>();
    DVASSERT(selectionData != nullptr);

    if (selectionData->IsLocked() == true)
    {
        sceneTreeData->selectionModel->clear();
        return;
    }

    DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();
    DAVA::SceneCollisionSystem* collisionSystem = sceneData->GetScene()->GetSystem<DAVA::SceneCollisionSystem>();
    DVASSERT(collisionSystem);

    QModelIndexList selectedRows = selectionModel->selectedRows();

    DAVA::SelectableGroup::CollectionType selectedCollection;
    selectedCollection.reserve(selectedRows.size());

    foreach (QModelIndex index, selectedRows)
    {
        DAVA::Selectable object = sceneTreeData->model->GetObjectByIndex(sceneTreeData->filterModel->mapToSource(index));
        object.SetBoundingBox(collisionSystem->GetBoundingBox(object.GetContainedObject()));
        selectedCollection.push_back(object);
    }

    DAVA::SelectableGroup newSelection;
    newSelection.Add(selectedCollection);
    selectionData->SetSelection(newSelection);
    OnSceneSelectionChanged(selectionData->GetSelection());
}

void SceneTreeModule::OnSyncRequested()
{
    executor.DelayedExecute([this]() {
        using namespace SceneTreeModuleDetail;
        const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
        DVASSERT(ctx != nullptr);

        SceneTreeData* data = ctx->GetData<SceneTreeData>();
        data->model->SyncChanges();
        OnSceneSelectionChanged(ctx->GetData<DAVA::SelectionData>()->GetSelection());
        data->filterModel->Refilter();
    });
}

void SceneTreeModule::BuildCreateMenu(DAVA::BaseEntityCreator* baseCreator, QMenu* menu)
{
    baseCreator->Init(GetAccessor(), GetUI());
    DAVA::EntityCreatorsGroup* group = dynamic_cast<DAVA::EntityCreatorsGroup*>(baseCreator);
    if (group != nullptr)
    {
        QMenu* subMenu = menu->addMenu(group->icon, group->text);
        for (std::unique_ptr<DAVA::BaseEntityCreator>& baseGroupCreator : group->creatorsGroup)
        {
            BuildCreateMenu(baseGroupCreator.get(), subMenu);
        }
    }
    else
    {
        DAVA::EntityCreator* creator = dynamic_cast<DAVA::EntityCreator*>(baseCreator);
        DVASSERT(creator != nullptr);
        QAction* action = menu->addAction(creator->icon, creator->text);
        connections.AddConnection(action, &QAction::triggered, DAVA::Bind(&SceneTreeModule::OnAddEntityClicked, this, creator));
    }
}

void SceneTreeModule::OnAddEntityClicked(DAVA::EntityCreator* creator)
{
    DVASSERT(creator != nullptr);

    DAVA::ContextAccessor* accessor = GetAccessor();
    SceneTreeModuleDetail::EntityCreationData* data = accessor->GetGlobalContext()->GetData<SceneTreeModuleDetail::EntityCreationData>();
    if (data->activeCreator != nullptr)
    {
        data->activeCreator->creationFinished.DisconnectAll();
        data->activeCreator->Cancel();
    }

    DAVA::DataContext* ctx = accessor->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    data->activeCreator = creator;
    data->activeCreator->creationFinished.Connect([data]() {
        data->activeCreator = nullptr;
    });

    creator->StartEntityCreation(ctx->GetData<DAVA::SceneData>()->GetScene().Get());
}

void SceneTreeModule::OnItemDoubleClicked(const QModelIndex& index)
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    DVASSERT(ctx != nullptr);

    DAVA::LookAtSelection(ctx->GetData<DAVA::SceneData>()->GetScene().Get());
}

void SceneTreeModule::OnContextMenuRequested(const QModelIndex& index, const QPoint& globalPos)
{
    using namespace SceneTreeModuleDetail;
    DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    DAVA::SceneData* sceneData = ctx->GetData<DAVA::SceneData>();
    SceneTreeData* sceneTreeData = ctx->GetData<SceneTreeData>();

    DAVA::Selectable currentObject = sceneTreeData->model->GetObjectByIndex(sceneTreeData->filterModel->mapToSource(index));
    DAVA::Vector<DAVA::Selectable> selectedObjects;
    foreach (const QModelIndex& selectedIndex, sceneTreeData->selectionModel->selectedRows())
    {
        selectedObjects.push_back(sceneTreeData->model->GetObjectByIndex(sceneTreeData->filterModel->mapToSource(selectedIndex)));
    }

    if (selectedObjects.empty() == true)
    {
        return;
    }

    std::unique_ptr<BaseContextMenu> ctxMenu = CreateSceneTreeContextMenu(sceneData->GetScene().Get(), sceneTreeData->model, selectedObjects, currentObject);
    DVASSERT(ctxMenu != nullptr);

    ctxMenu->Init(GetAccessor(), GetUI(), GetInvoker());
    ctxMenu->Show(globalPos);
    sceneTreeData->model->SyncChanges();
}

void SceneTreeModule::CollapseAll()
{
    SetExpandedIndexList(DAVA::Set<QPersistentModelIndex>());
}

void SceneTreeModule::ExpandAll()
{
    using namespace SceneTreeModuleDetail;
    DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    DVASSERT(ctx != nullptr);
    ctx->GetData<SceneTreeData>()->ExpandAll();
}

void SceneTreeModule::OnInverseCollapsing()
{
    using namespace SceneTreeModuleDetail;
    DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    SceneTreeData* data = ctx->GetData<SceneTreeData>();
    DAVA::Set<QPersistentModelIndex> indexSet = data->expandedIndexList;
    QModelIndexList indexList = data->selectionModel->selection().indexes();
    foreach (const QModelIndex& index, indexList)
    {
        if (data->filterModel->hasChildren(index))
        {
            auto iter = data->expandedIndexList.find(index);
            if (iter == data->expandedIndexList.end())
            {
                indexSet.insert(index);
            }
            else
            {
                indexSet.erase(index);
            }
        }
    }

    SetExpandedIndexList(indexSet);
}

const DAVA::Set<QPersistentModelIndex>& SceneTreeModule::GetExpandedIndexList() const
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        static DAVA::Set<QPersistentModelIndex> empty;
        return empty;
    }

    return ctx->GetData<SceneTreeData>()->expandedIndexList;
}

void SceneTreeModule::SetExpandedIndexList(const DAVA::Set<QPersistentModelIndex>& expandedIndexList)
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    SceneTreeData* data = ctx->GetData<SceneTreeData>();
    SceneTreeView::EraseEmptyIndexes(data->expandedIndexList);
    DAVA::Set<QPersistentModelIndex> prevExpanded = data->expandedIndexList;

    DAVA::Set<QPersistentModelIndex> becameCollapsed;
    DAVA::Set<QPersistentModelIndex> becameExpanded;

    std::set_difference(prevExpanded.begin(), prevExpanded.end(),
                        expandedIndexList.begin(), expandedIndexList.end(),
                        std::inserter(becameCollapsed, becameCollapsed.end()));

    std::set_difference(expandedIndexList.begin(), expandedIndexList.end(),
                        prevExpanded.begin(), prevExpanded.end(),
                        std::inserter(becameExpanded, becameExpanded.end()));

    auto setSolidFlag = [](const QModelIndex& index, bool isSolid) {
        QVariant object = index.data(ToItemRoleCast(eSceneTreeRoles::InternalObjectRole));
        DVASSERT(object.canConvert<DAVA::Any>());
        DAVA::Selectable selectable(object.value<DAVA::Any>());
        if (selectable.CanBeCastedTo<DAVA::Entity>())
        {
            DAVA::Entity* entity = selectable.Cast<DAVA::Entity>();
            entity->SetSolid(isSolid);
        }
    };

    using TPropagateFn = DAVA::Function<void(QAbstractItemModel*, const QModelIndex&)>;
    TPropagateFn propagateSolidFlag = [&expandedIndexList, &setSolidFlag, &propagateSolidFlag](QAbstractItemModel* model, const QModelIndex& index) {
        int rowCount = model->rowCount(index);
        for (int i = 0; i < rowCount; ++i)
        {
            QModelIndex childIndex = model->index(i, 0, index);
            setSolidFlag(childIndex, expandedIndexList.count(childIndex) == 0);
            propagateSolidFlag(model, childIndex);
        }
    };

    for (const QPersistentModelIndex& index : becameCollapsed)
    {
        if (index.isValid())
        {
            setSolidFlag(index, true);
            propagateSolidFlag(data->filterModel, index);
        }
    }

    for (const QPersistentModelIndex& index : becameExpanded)
    {
        if (index.isValid())
        {
            setSolidFlag(index, false);
            propagateSolidFlag(data->filterModel, index);
        }
    }

    DAVA::SelectionData* selectionData = ctx->GetData<DAVA::SelectionData>();
    const DAVA::SelectableGroup& selection = selectionData->GetSelection();
    DAVA::Vector<DAVA::Selectable> newSelectedObject;
    newSelectedObject.reserve(selection.GetSize());
    for (const DAVA::Selectable& obj : selection.GetContent())
    {
        QModelIndex objectIndex = data->filterModel->mapFromSource(data->model->GetIndexByObject(obj));
        QModelIndex objectParentIndex = objectIndex.parent();
        bool wholePathExpanded = true;
        while (objectParentIndex.isValid())
        {
            if (expandedIndexList.count(objectParentIndex) == 0)
            {
                wholePathExpanded = false;
                break;
            }

            objectParentIndex = objectParentIndex.parent();
        }

        if (wholePathExpanded == true)
        {
            newSelectedObject.push_back(obj);
        }
    }

    DAVA::SelectableGroup newSelectionGroup;
    newSelectionGroup.Add(newSelectedObject);
    selectionData->SetSelection(newSelectionGroup);

    data->expandedIndexList = expandedIndexList;
}

void SceneTreeModule::ReloadTexturesInSelected()
{
    DAVA::SelectionData* selectionData = GetAccessor()->GetActiveContext()->GetData<DAVA::SelectionData>();
    const DAVA::SelectableGroup& selection = selectionData->GetSelection();

    DAVA::Vector<DAVA::Texture*> reloadTextures;
    for (auto selectEntity : selection.ObjectsOfType<DAVA::Entity>())
    {
        DAVA::SceneHelper::TextureCollector collector;
        DAVA::SceneHelper::EnumerateEntityTextures(selectEntity->GetScene(), selectEntity, collector);
        DAVA::TexturesMap& textures = collector.GetTextures();

        for (auto& tex : textures)
        {
            auto found = std::find(reloadTextures.begin(), reloadTextures.end(), tex.second);

            if (found == reloadTextures.end())
            {
                reloadTextures.push_back(tex.second);
            }
        }
    }

    InvokeOperation(DAVA::ReloadTextures.ID, reloadTextures);
}

const DAVA::Vector<DAVA::SceneTreeFilterBase*>& SceneTreeModule::GetFiltersChain() const
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    if (ctx == nullptr)
    {
        static DAVA::Vector<DAVA::SceneTreeFilterBase*> empty;
        return empty;
    }

    SceneTreeData* data = ctx->GetData<SceneTreeData>();
    return data->filterModel->GetFiltersChain();
}

void SceneTreeModule::AddFilterToChain(const DAVA::Any& filterTypeKey)
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    DVASSERT(ctx != nullptr);

    SceneTreeData* data = ctx->GetData<SceneTreeData>();

    const AvailableFilterKey& key = filterTypeKey.Get<AvailableFilterKey>();
    DVASSERT(key.types.size() == key.states.size());
    for (size_t i = 0; i < key.types.size(); ++i)
    {
        data->filterModel->AddFilterToChain(key.types[i], key.states[i]);
    }

    if (key.nameFiltration.empty() == false)
    {
        data->SetFilter(QString::fromStdString(key.nameFiltration));
    }
}

void SceneTreeModule::RemoveFilterFromChain(const DAVA::Any& filterIndex)
{
    using namespace SceneTreeModuleDetail;
    DAVA::int32 index = filterIndex.Cast<DAVA::int32>();
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    DVASSERT(ctx != nullptr);

    SceneTreeData* data = ctx->GetData<SceneTreeData>();
    data->filterModel->DeleteFilter(index);
}

DAVA::ContentFilter::AvailableFilterBase* SceneTreeModule::GetAvailableFilterTypes() const
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetGlobalContext();
    SceneTreeGlobalData* data = ctx->GetData<SceneTreeGlobalData>();
    return data->filterTypes.get();
}

void SceneTreeModule::SaveCurrentChainAsFilterType(QString filterName) const
{
    using namespace SceneTreeModuleDetail;
    const DAVA::DataContext* ctx = GetAccessor()->GetActiveContext();
    DVASSERT(ctx != nullptr);
    SceneTreeData* data = ctx->GetData<SceneTreeData>();
    SceneTreeGlobalData* globalGata = ctx->GetData<SceneTreeGlobalData>();

    AvailableFilterKey key;
    key.nameFiltration = data->filterModel->GetFilter().toStdString();
    key.uniqueID = globalGata->uniqueIDCounter++;
    for (DAVA::SceneTreeFilterBase* filterBase : data->filterModel->GetFiltersChain())
    {
        key.types.push_back(DAVA::ReflectedTypeDB::GetByPointer(filterBase));
        FilterState state;
        state.enabled = filterBase->IsEnabled();
        state.inverted = filterBase->IsInverted();
        key.states.push_back(state);
    }

    DVASSERT(key.types.empty() == false);

    std::unique_ptr<DAVA::ContentFilter::AvailableFilter> newFilterType = std::make_unique<DAVA::ContentFilter::AvailableFilter>();
    newFilterType->filterName = DAVA::FastName(filterName.toStdString());
    newFilterType->key = key;
    newFilterType->userDefined = true;
    globalGata->filterTypes->filters.push_back(std::move(newFilterType));

    DAVA::PropertiesItem pi = GetAccessor()->CreatePropertiesNode("sceneTreeCustomFilters");
    globalGata->SaveUserDefinedFilters(pi);
}

DAVA_VIRTUAL_REFLECTION_IMPL(SceneTreeModule)
{
    auto hasActiveContext = [](SceneTreeModule* m) -> bool {
        return m->GetAccessor()->GetActiveContext() != nullptr;
    };

    auto getFilterText = [](SceneTreeModule* m) -> DAVA::String {
        using namespace SceneTreeModuleDetail;
        const DAVA::DataContext* ctx = m->GetAccessor()->GetActiveContext();
        if (ctx == nullptr)
        {
            return DAVA::String("");
        }

        return ctx->GetData<SceneTreeData>()->GetFilter().toStdString();
    };

    auto setFilterText = [](SceneTreeModule* m, const DAVA::String& filter) -> void {
        using namespace SceneTreeModuleDetail;
        const DAVA::DataContext* ctx = m->GetAccessor()->GetActiveContext();
        if (ctx == nullptr)
        {
            return;
        }

        DAVA::SelectionData* selectionData = ctx->GetData<DAVA::SelectionData>();
        DAVA::SelectableGroup selection = selectionData->GetSelection();

        SceneTreeData* data = ctx->GetData<SceneTreeData>();
        QString newFilter = QString::fromStdString(filter);
        data->SetFilter(newFilter);
        m->OnSceneSelectionChanged(selection);
    };

    DAVA::ReflectionRegistrator<SceneTreeModule>::Begin()
    .ConstructorByPointer()
    .Field("dataModel", &SceneTreeModule::GetDataModel, nullptr)
    .Field("selectionModel", &SceneTreeModule::GetSelectionModel, nullptr)
    .Field("expandedIndexList", &SceneTreeModule::GetExpandedIndexList, &SceneTreeModule::SetExpandedIndexList)
    .Field("hasActiveContext", hasActiveContext, nullptr)
    .Field("filterText", getFilterText, setFilterText)
    .Method("immediateTextChanged", &SceneTreeModule::OnFilterChanged)
    .Method("itemDoubleClicked", &SceneTreeModule::OnItemDoubleClicked)
    .Method("contextMenuRequested", &SceneTreeModule::OnContextMenuRequested)
    .Method("dropExecuted", &SceneTreeModule::OnSyncRequested)
    //Filtration
    .Field("filtersChain", &SceneTreeModule::GetFiltersChain, nullptr)
    .Method("addFilterToChain", &SceneTreeModule::AddFilterToChain)
    .Method("removeFilterFromChain", &SceneTreeModule::RemoveFilterFromChain)
    .Method("getAvailableFilters", &SceneTreeModule::GetAvailableFilterTypes)
    .Method("saveCurrentChain", &SceneTreeModule::SaveCurrentChainAsFilterType)
    .End();
}

DECL_TARC_MODULE(SceneTreeModule);
