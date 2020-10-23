#include "DAVAEngine.h"
#include "Classes/Qt/DockSceneInfo/SceneInfo.h"
#include "Classes/Qt/Main/QtUtils.h"
#include "Classes/Qt/Scene/SceneSignals.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/SceneHelper.h"
#include "Classes/Qt/Scene/System/EditorStatisticsSystem.h"
#include "Classes/Qt/Scene/System/EditorVegetationSystem.h"
#include "Classes/Qt/TextureBrowser/TextureCache.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Selection/SelectionData.h"
#include "Classes/Selection/Selection.h"
#include "Classes/ImageTools/ImageTools.h"
#include "Classes/Commands2/Base/RECommandNotificationObject.h"

#include <TArc/DataProcessing/DataContext.h>
#include <TArc/SharedModules/ThemesModule/ThemesModule.h>
#include <TArc/Core/FieldBinder.h>

#include <Scene3D/Components/ComponentHelpers.h>
#include <Render/TextureDescriptor.h>
#include <Render/Texture.h>
#include <Render/Material/NMaterialNames.h>
#include <Render/VisibilityQueryResults.h>
#include <Render/2D/Sprite.h>
#include <Base/BaseTypes.h>

#include <QHeaderView>
#include <QTimer>
#include <QPalette>
#include <QApplication>

namespace SceneInfoDetail
{
QPalette GetPalette()
{
    return qApp->palette();
}
}

SceneInfo::SceneInfo(QWidget* parent /* = 0 */)
    : QtPropertyEditor(parent)
    , treeStateHelper(this, curModel)
    , infoUpdated(DAVA::MakeFunction(this, &SceneInfo::RefreshAllData))
{
    using namespace DAVA;
    // global scene manager signals
    SceneSignals* signalDispatcher = SceneSignals::Instance();
    connect(signalDispatcher, &SceneSignals::Activated, this, &SceneInfo::SceneActivated);
    connect(signalDispatcher, &SceneSignals::Deactivated, this, &SceneInfo::SceneDeactivated);
    connect(signalDispatcher, &SceneSignals::StructureChanged, this, &SceneInfo::SceneStructureChanged);
    connect(signalDispatcher, &SceneSignals::CommandExecuted, this, &SceneInfo::OnCommmandExecuted);
    connect(signalDispatcher, &SceneSignals::QualityChanged, this, &SceneInfo::OnQualityChanged);

    fieldBinder.reset(new DAVA::TArc::FieldBinder(REGlobal::GetAccessor()));
    {
        DAVA::TArc::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
        fieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &SceneInfo::OnSelectionChanged));
    }
    {
        DAVA::TArc::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<DAVA::TArc::ThemesSettings>();
        fieldDescr.fieldName = DAVA::FastName("currentTheme");
        fieldBinder->BindField(fieldDescr, [this](const DAVA::Any&) {
            OnThemeChanged();
        });
    }

    // MainWindow actions
    posSaver.Attach(this, "DockSceneInfo");

    VariantType v = posSaver.LoadValue("splitPos");
    if (v.GetType() == VariantType::TYPE_INT32)
        header()->resizeSection(0, v.AsInt32());

    ClearData();

    InitializeInfo();

    viewport()->setBackgroundRole(QPalette::Window);

    TextureCache* cache = TextureCache::Instance();
    DVASSERT(cache != nullptr);
    QObject::connect(cache, &TextureCache::CacheCleared, this, &SceneInfo::TexturesReloaded);
}

SceneInfo::~SceneInfo()
{
    DAVA::VariantType v(header()->sectionSize(0));
    posSaver.SaveValue("splitPos", v);
}

void SceneInfo::InitializeInfo()
{
    RemovePropertyAll();

    InitializeGeneralSection();
    Initialize3DDrawSection();
    InitializeLODSectionInFrame();
    InitializeLODSectionForSelection();
    InitializeSpeedTreeInfoSelection();
    InitializeLayersSection();

    InitializeVegetationInfoSection();
}

void SceneInfo::InitializeGeneralSection()
{
    QtPropertyData* header = CreateInfoHeader("General Scene Info");

    AddChild("Entities Count", header);
    AddChild("Emitters Count", header);

    AddChild("All Textures Size", header);
    AddChild("Material Textures Size", header);
    AddChild("Particles Textures Size", header);

    AddChild("Sprites Count", header);
    AddChild("Particle Textures Count", header);
}

void SceneInfo::RefreshSceneGeneralInfo()
{
    QtPropertyData* header = GetInfoHeader("General Scene Info");

    SetChild("Entities Count", static_cast<DAVA::uint32>(nodesAtScene.size()), header);
    SetChild("Emitters Count", emittersCount, header);

    SetChild("All Textures Size", QString::fromStdString(SizeInBytesToString(static_cast<DAVA::float32>(sceneTexturesSize + particleTexturesSize))), header);
    SetChild("Material Textures Size", QString::fromStdString(SizeInBytesToString(static_cast<DAVA::float32>(sceneTexturesSize))), header);
    SetChild("Particles Textures Size", QString::fromStdString(SizeInBytesToString(static_cast<DAVA::float32>(particleTexturesSize))), header);

    SetChild("Sprites Count", spritesCount, header);
    SetChild("Particle Textures Count", static_cast<DAVA::uint32>(particleTextures.size()), header);
}

void SceneInfo::Initialize3DDrawSection()
{
    QtPropertyData* header = CreateInfoHeader("DrawInfo");

    AddChild("Visible Render Object Count", header);
    AddChild("Occluded Render Object Count", header);

    AddChild("DrawPrimitiveCalls", header);
    AddChild("DrawIndexedPrimitiveCalls", header);
    AddChild("LineList", header);
    AddChild("TriangleList", header);
    AddChild("TriangleStrip", header);

    QtPropertyData* header2 = CreateInfoHeader("Bind Info");
    AddChild("Dynamic Param Bind Count", header2);
    AddChild("Material Param Bind Count", header2);
}

void SceneInfo::Refresh3DDrawInfo()
{
    using namespace DAVA;
    if (!activeScene)
        return;
    QtPropertyData* header = GetInfoHeader("DrawInfo");

    const RenderStats& renderStats = activeScene->GetRenderStats();

    SetChild("Visible Render Object Count", renderStats.visibleRenderObjects, header);
    SetChild("Occluded Render Object Count", renderStats.occludedRenderObjects, header);

    SetChild("DrawPrimitiveCalls", renderStats.drawPrimitive, header);
    SetChild("DrawIndexedPrimitiveCalls", renderStats.drawIndexedPrimitive, header);
    SetChild("LineList", renderStats.primitiveLineListCount, header);
    SetChild("TriangleList", renderStats.primitiveTriangleListCount, header);
    SetChild("TriangleStrip", renderStats.primitiveTriangleStripCount, header);

    QtPropertyData* header2 = GetInfoHeader("Bind Info");

    SetChild("Dynamic Param Bind Count", renderStats.dynamicParamBindCount, header2);
    SetChild("Material Param Bind Count", renderStats.materialParamBindCount, header2);
}

void SceneInfo::InitializeSpeedTreeInfoSelection()
{
    QtPropertyData* header = CreateInfoHeader("SpeedTree Info");

    AddChild("Leafs Square Absolute (X-axis)", header);
    AddChild("Leafs Square Absolute (Y-axis)", header);
    AddChild("Leafs Square Absolute (Z-axis)", header);
    AddChild("Leafs Square Relative (X-axis)", header);
    AddChild("Leafs Square Relative (Y-axis)", header);
    AddChild("Leafs Square Relative (Z-axis)", header);
}

void SceneInfo::RefreshSpeedTreeInfoSelection()
{
    QtPropertyData* header = GetInfoHeader("SpeedTree Info");

    DAVA::Vector3 leafsSquareAbsolute, leafsSquareRelative;
    for (const SpeedTreeInfo& info : speedTreesInfo)
    {
        leafsSquareAbsolute += info.leafsSquareAbsolute;
        leafsSquareRelative += info.leafsSquareRelative;
    }

    //percentage
    leafsSquareRelative *= 100.f;

    SetChild("Leafs Square Absolute (X-axis)", DAVA::Format("%.2f m^2", leafsSquareAbsolute.x).c_str(), header);
    SetChild("Leafs Square Absolute (Y-axis)", DAVA::Format("%.2f m^2", leafsSquareAbsolute.y).c_str(), header);
    SetChild("Leafs Square Absolute (Z-axis)", DAVA::Format("%.2f m^2", leafsSquareAbsolute.z).c_str(), header);

    SetChild("Leafs Square Relative (X-axis)", DAVA::Format("%.2f%%", leafsSquareRelative.x).c_str(), header);
    SetChild("Leafs Square Relative (Y-axis)", DAVA::Format("%.2f%%", leafsSquareRelative.y).c_str(), header);
    SetChild("Leafs Square Relative (Z-axis)", DAVA::Format("%.2f%%", leafsSquareRelative.z).c_str(), header);
}

void SceneInfo::InitializeLODSectionInFrame()
{
    QtPropertyData* header = CreateInfoHeader("LOD in Frame");
    QtPropertyToolButton* button = header->AddButton(QtPropertyToolButton::ACTIVE_ALWAYS);
    button->setIcon(QIcon(":/QtIcons/refresh.png"));
    button->setAutoRaise(false);
    QObject::connect(button, &QToolButton::clicked, this, &SceneInfo::RefreshLODInfoInFrame);

    for (DAVA::int32 i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        AddChild(DAVA::Format("Objects LOD%d Triangles", i).c_str(), header);
    }

    AddChild("All LOD Triangles", header);
    AddChild("Objects without LOD Triangles", header);
    AddChild("Landscape Triangles", header);
    AddChild("All Triangles", header);
}

void SceneInfo::InitializeLODSectionForSelection()
{
    QtPropertyData* header = CreateInfoHeader("LOD Info for Selected Entities");
    QtPropertyToolButton* button = header->AddButton(QtPropertyToolButton::ACTIVE_ALWAYS);
    button->setIcon(QIcon(":/QtIcons/refresh.png"));
    button->setAutoRaise(false);
    QObject::connect(button, &QToolButton::clicked, this, &SceneInfo::RefreshLODInfoForSelection);

    for (DAVA::int32 i = 0; i < DAVA::LodComponent::MAX_LOD_LAYERS; ++i)
    {
        AddChild(DAVA::Format("Objects LOD%d Triangles", i).c_str(), header);
    }

    AddChild("All LOD Triangles", header);
    AddChild("Objects without LOD Triangles", header);
    AddChild("All Triangles", header);
}

void SceneInfo::RefreshLODInfoInFrame()
{
    using namespace DAVA;
    QtPropertyData* header = GetInfoHeader("LOD in Frame");

    EditorStatisticsSystem* statisticsSystem = GetCurrentEditorStatisticsSystem();
    if (statisticsSystem != nullptr)
    {
        const auto& triangles = statisticsSystem->GetTriangles(eEditorMode::MODE_ALL_SCENE, false);

        uint32 lodTriangles = 0;
        for (int32 i = 0; i < LodComponent::MAX_LOD_LAYERS; ++i)
        {
            SetChild(Format("Objects LOD%d Triangles", i).c_str(), triangles[i + 1], header);
            lodTriangles += triangles[i + 1];
        }

        int32 landTriangles = (landscape) ? landscape->GetDrawIndices() : 0;
        landTriangles /= 3;
        SetChild("All LOD Triangles", lodTriangles, header);
        SetChild("Objects without LOD Triangles", triangles[0], header);
        SetChild("Landscape Triangles", landTriangles, header);
        SetChild("All Triangles", triangles[0] + landTriangles + lodTriangles, header);
    }
    else
    {
        for (int32 i = 0; i < LodComponent::MAX_LOD_LAYERS; ++i)
        {
            SetChild(Format("Objects LOD%d Triangles", i).c_str(), 0, header);
        }

        SetChild("All LOD Triangles", 0, header);
        SetChild("Objects without LOD Triangles", 0, header);
        SetChild("Landscape Triangles", 0, header);
        SetChild("All Triangles", 0, header);
    }
}

void SceneInfo::RefreshLODInfoForSelection()
{
    using namespace DAVA;
    QtPropertyData* header = GetInfoHeader("LOD Info for Selected Entities");

    EditorStatisticsSystem* statisticsSystem = GetCurrentEditorStatisticsSystem();
    if (statisticsSystem != nullptr)
    {
        const auto& triangles = statisticsSystem->GetTriangles(eEditorMode::MODE_SELECTION, false);

        uint32 lodTriangles = 0;
        for (int32 i = 0; i < LodComponent::MAX_LOD_LAYERS; ++i)
        {
            SetChild(Format("Objects LOD%d Triangles", i).c_str(), triangles[i + 1], header);
            lodTriangles += triangles[i + 1];
        }

        SetChild("All LOD Triangles", lodTriangles, header);

        SetChild("Objects without LOD Triangles", triangles[0], header);
        SetChild("All Triangles", triangles[0] + lodTriangles, header);
    }
    else
    {
        for (int32 i = 0; i < LodComponent::MAX_LOD_LAYERS; ++i)
        {
            SetChild(Format("Objects LOD%d Triangles", i).c_str(), 0, header);
        }

        SetChild("All LOD Triangles", 0, header);

        SetChild("Objects without LOD Triangles", 0, header);
        SetChild("All Triangles", 0, header);
    }
}

DAVA::uint32 SceneInfo::CalculateTextureSize(const DAVA::TexturesMap& textures)
{
    using namespace DAVA;
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);

    String projectPath = data->GetProjectPath().GetAbsolutePathname();
    uint32 textureSize = 0;

    CommonInternalSettings* settings = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>();
    eGPUFamily requestedGPU = settings->textureViewGPU;

    TexturesMap::const_iterator endIt = textures.end();
    for (TexturesMap::const_iterator it = textures.begin(); it != endIt; ++it)
    {
        FilePath pathname = it->first;
        Texture* tex = it->second;
        DVASSERT(tex);

        if (tex->IsPinkPlaceholder())
        {
            continue;
        }

        if (String::npos == pathname.GetAbsolutePathname().find(projectPath))
        {
            Logger::Warning("[SceneInfo::CalculateTextureSize] Path (%s) doesn't belong to project.", pathname.GetAbsolutePathname().c_str());
            continue;
        }

        auto baseMipmap = tex->GetBaseMipMap();

        auto descriptor = tex->GetDescriptor();
        eGPUFamily gpu = descriptor->IsCompressedFile() ? descriptor->gpu : requestedGPU;
        textureSize += ImageTools::GetTexturePhysicalSize(tex->GetDescriptor(), gpu, baseMipmap);
    }

    return textureSize;
}

void SceneInfo::CollectSceneData()
{
    ClearData();

    if (activeScene)
    {
        activeScene->GetChildNodes(nodesAtScene);

        SceneHelper::TextureCollector collector(SceneHelper::TextureCollector::OnlyActiveTextures);
        SceneHelper::EnumerateSceneTextures(activeScene, collector);
        sceneTexturesSize = CalculateTextureSize(collector.GetTextures());

        CollectParticlesData();
        particleTexturesSize = CalculateTextureSize(particleTextures);
    }
}

void SceneInfo::ClearData()
{
    nodesAtScene.clear();
    particleTextures.clear();

    ClearSelectionData();

    sceneTexturesSize = 0;
    particleTexturesSize = 0;
    emittersCount = 0;
    spritesCount = 0;

    speedTreesInfo.clear();
}

void SceneInfo::ClearSelectionData()
{
    selectedRenderObjects.clear();
}

void SceneInfo::CollectParticlesData()
{
    using namespace DAVA;
    Set<Sprite*> sprites;

    emittersCount = 0;
    for (uint32 n = 0; n < nodesAtScene.size(); ++n)
    {
        ParticleEffectComponent* effect = GetEffectComponent(nodesAtScene[n]);
        if (!effect)
            continue;

        for (int32 i = 0, sz = effect->GetEmittersCount(); i < sz; ++i)
        {
            ++emittersCount;
            Vector<ParticleLayer*>& layers = effect->GetEmitterInstance(i)->GetEmitter()->layers;
            for (uint32 lay = 0; lay < layers.size(); ++lay)
            {
                ProcessParticleSprite(layers[lay]->sprite, sprites);
                ProcessParticleSprite(layers[lay]->flowmap, sprites);
                ProcessParticleSprite(layers[lay]->noise, sprites);
                ProcessParticleSprite(layers[lay]->alphaRemapSprite, sprites);
            }
        }
    }

    spritesCount = (uint32)sprites.size();
}

void SceneInfo::ProcessParticleSprite(DAVA::Sprite* sprite, DAVA::Set<DAVA::Sprite*>& sprites)
{
    using namespace DAVA;
    if (sprite == nullptr)
        return;
    sprites.insert(sprite);

    for (int32 fr = 0; fr < sprite->GetFrameCount(); ++fr)
    {
        Texture* tex = sprite->GetTexture(fr);
        CollectTexture(particleTextures, tex->GetPathname(), tex);
    }
}

DAVA::uint32 SceneInfo::GetTrianglesForNotLODEntityRecursive(DAVA::Entity* entity, bool onlyVisibleBatches)
{
    using namespace DAVA;
    if (GetLodComponent(entity))
        return 0;

    uint32 triangles = 0;

    RenderObject* ro = GetRenderObject(entity);
    if (ro && ro->GetType() != RenderObject::TYPE_PARTICLE_EMITTER)
    {
        uint32 batchCount = (onlyVisibleBatches) ? ro->GetActiveRenderBatchCount() : ro->GetRenderBatchCount();
        for (uint32 i = 0; i < batchCount; ++i)
        {
            RenderBatch* rb = (onlyVisibleBatches) ? ro->GetActiveRenderBatch(i) : ro->GetRenderBatch(i);
            PolygonGroup* pg = rb->GetPolygonGroup();
            if (pg)
            {
                triangles += pg->GetPrimitiveCount();
            }
        }
    }

    DAVA::uint32 count = entity->GetChildrenCount();
    for (DAVA::uint32 i = 0; i < count; ++i)
    {
        triangles += GetTrianglesForNotLODEntityRecursive(entity->GetChild(i), onlyVisibleBatches);
    }

    return triangles;
}

void SceneInfo::CollectTexture(DAVA::TexturesMap& textures, const DAVA::FilePath& name, DAVA::Texture* tex)
{
    if (!name.IsEmpty() && tex)
    {
        textures[FILEPATH_MAP_KEY(name)] = tex;
    }
}

QtPropertyData* SceneInfo::CreateInfoHeader(const QString& key)
{
    QtPropertyData* headerData = new QtPropertyData(DAVA::FastName(key.toStdString()));
    headerData->SetEditable(false);
    headerData->SetBackground(SceneInfoDetail::GetPalette().alternateBase());
    AppendProperty(std::unique_ptr<QtPropertyData>(headerData));
    return headerData;
}

QtPropertyData* SceneInfo::GetInfoHeader(const QString& key)
{
    QtPropertyData* header = nullptr;
    QtPropertyData* root = GetRootProperty();
    if (NULL != root)
    {
        header = root->ChildGet(DAVA::FastName(key.toStdString()));
    }
    return header;
}

void SceneInfo::AddChild(const QString& key, QtPropertyData* parent)
{
    std::unique_ptr<QtPropertyData> propData(new QtPropertyData(DAVA::FastName(key.toStdString())));
    propData->SetEditable(false);
    propData->SetBackground(SceneInfoDetail::GetPalette().base());
    parent->ChildAdd(std::move(propData));
}

void SceneInfo::AddChild(const QString& key, const QString& toolTip, QtPropertyData* parent)
{
    std::unique_ptr<QtPropertyData> propData(new QtPropertyData(DAVA::FastName(key.toStdString())));
    propData->SetEditable(false);
    propData->SetToolTip(toolTip);
    propData->SetBackground(SceneInfoDetail::GetPalette().base());
    parent->ChildAdd(std::move(propData));
}

void SceneInfo::SetChild(const QString& key, const QVariant& value, QtPropertyData* parent)
{
    if (NULL != parent)
    {
        QtPropertyData* propData = parent->ChildGet(DAVA::FastName(key.toStdString()));
        if (NULL != propData)
        {
            propData->SetValue(value);
        }
    }
}

bool SceneInfo::HasChild(const QString& key, QtPropertyData* parent)
{
    bool hasChild = false;
    if (NULL != parent)
    {
        QtPropertyData* propData = parent->ChildGet(DAVA::FastName(key.toStdString()));
        hasChild = (propData != NULL);
    }

    return hasChild;
}

void SceneInfo::SaveTreeState()
{
    // Store the current Property Editor Tree state before switching to the new node.
    // Do not clear the current states map - we are using one storage to share opened
    // Property Editor nodes between the different Scene Nodes.
    treeStateHelper.SaveTreeViewState(false);
}

void SceneInfo::RestoreTreeState()
{
    // Restore back the tree view state from the shared storage.
    if (!treeStateHelper.IsTreeStateStorageEmpty())
    {
        treeStateHelper.RestoreTreeViewState();
    }
    else
    {
        // Expand the root elements as default value.
        expandToDepth(0);
    }
}

void SceneInfo::showEvent(QShowEvent* event)
{
    if (!isUpToDate)
    {
        isUpToDate = true;
        RefreshAllData();
    }

    QtPropertyEditor::showEvent(event);
}

void SceneInfo::UpdateInfoByTimer()
{
    if (!isVisible())
        return;

    Refresh3DDrawInfo();
    if (REGlobal::GetGlobalContext()->GetData<RenderStatsSettings>()->calculatePerFrame == true)
    {
        RefreshLODInfoInFrame();
        RefreshLODInfoForSelection();
    }

    CollectSpeedInfo(&Selection::GetSelection());
    RefreshSpeedTreeInfoSelection();

    RefreshVegetationInfoSection();

    RefreshLayersSection();
}

void SceneInfo::RefreshAllData()
{
    CollectSceneData();

    SaveTreeState();

    RefreshSceneGeneralInfo();
    Refresh3DDrawInfo();
    RefreshLODInfoInFrame();
    RefreshLODInfoForSelection();

    CollectSpeedInfo(&Selection::GetSelection());
    RefreshSpeedTreeInfoSelection();

    RefreshVegetationInfoSection();

    RefreshLayersSection();

    RestoreTreeState();
}

void SceneInfo::SceneActivated(SceneEditor2* scene)
{
    activeScene = scene;
    landscape = FindLandscape(activeScene);

    isUpToDate = isVisible();
    if (isUpToDate)
    {
        infoUpdated.Update();
    }
}

void SceneInfo::SceneDeactivated(SceneEditor2* scene)
{
    if (activeScene == scene)
    {
        activeScene = NULL;
        landscape = NULL;
        RefreshAllData();
    }
}

void SceneInfo::SceneStructureChanged(SceneEditor2* scene, DAVA::Entity* parent)
{
    if (activeScene == scene)
    {
        landscape = FindLandscape(activeScene);

        isUpToDate = isVisible();
        if (isUpToDate)
        {
            infoUpdated.Update();
        }
    }
}

void SceneInfo::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<SelectableGroup>())
    {
        const SelectableGroup& selection = selectionAny.Get<SelectableGroup>();
        ClearSelectionData();
        CollectSelectedRenderObjects(&selection);
        if (REGlobal::GetGlobalContext()->GetData<RenderStatsSettings>()->calculatePerFrame == true)
        {
            RefreshLODInfoForSelection();
        }

        CollectSpeedInfo(&selection);
        RefreshSpeedTreeInfoSelection();
    }
}

void SceneInfo::OnCommmandExecuted(SceneEditor2* scene, const RECommandNotificationObject& commandNotification)
{
    static const DAVA::Vector<DAVA::uint32> commandIDs =
    {
      CMDID_MATERIAL_CHANGE_CURRENT_CONFIG, CMDID_MATERIAL_CREATE_CONFIG,
      CMDID_MATERIAL_REMOVE_TEXTURE, CMDID_INSP_MEMBER_MODIFY, CMDID_INSP_DYNAMIC_MODIFY,
      CMDID_REFLECTED_FIELD_MODIFY
    };

    if (commandNotification.MatchCommandIDs(commandIDs))
    {
        infoUpdated.Update();
    }
}

void SceneInfo::OnThemeChanged()
{
    InitializeInfo();
}

void SceneInfo::CollectSelectedRenderObjects(const SelectableGroup* selected)
{
    for (auto entity : selected->ObjectsOfType<DAVA::Entity>())
    {
        CollectSelectedRenderObjectsRecursivly(entity);
    }
}

void SceneInfo::CollectSelectedRenderObjectsRecursivly(DAVA::Entity* entity)
{
    using namespace DAVA;
    DVASSERT(entity != nullptr);

    RenderObject* renderObject = GetRenderObject(entity);
    if (renderObject)
    {
        selectedRenderObjects.insert(renderObject);
    }

    for (int32 i = 0, sz = entity->GetChildrenCount(); i < sz; ++i)
    {
        CollectSelectedRenderObjectsRecursivly(entity->GetChild(i));
    }
}

void SceneInfo::CollectSpeedInfo(const SelectableGroup* forGroup)
{
    using namespace DAVA;
    speedTreesInfo.clear();

    for (auto entity : forGroup->ObjectsOfType<DAVA::Entity>())
    {
        RenderObject* ro = GetRenderObject(entity);
        if (ro && ro->GetType() == RenderObject::TYPE_SPEED_TREE)
            speedTreesInfo.push_back(GetSpeedTreeInfo(static_cast<SpeedTreeObject*>(ro)));
    }
}

SceneInfo::SpeedTreeInfo SceneInfo::GetSpeedTreeInfo(DAVA::SpeedTreeObject* renderObject)
{
    using namespace DAVA;
    DVASSERT(renderObject != nullptr);

    SpeedTreeInfo info;

    int32 rbCount = renderObject->GetActiveRenderBatchCount();
    for (int32 i = 0; i < rbCount; ++i)
    {
        RenderBatch* rb = renderObject->GetActiveRenderBatch(i);

        PolygonGroup* pg = rb->GetPolygonGroup();

        if ((pg->GetFormat() & DAVA::EVF_PIVOT4) == 0)
        {
            continue;
        }

        String fxName = rb->GetMaterial()->GetEffectiveFXName().c_str();
        std::transform(fxName.begin(), fxName.end(), fxName.begin(), ::tolower);

        if ((strstr(fxName.c_str(), "alphatest") == nullptr) && (strstr(fxName.c_str(), "alphablend") == nullptr))
        {
            continue;
        }

        int32 triangleCount = pg->GetPrimitiveCount();
        for (int32 t = 0; t < triangleCount; t++)
        {
            int32 i1, i2, i3;
            int32 baseVertexIndex = t * 3;
            pg->GetIndex(baseVertexIndex, i1);
            pg->GetIndex(baseVertexIndex + 1, i2);
            pg->GetIndex(baseVertexIndex + 2, i3);

            Vector3 v1, v2, v3;
            pg->GetCoord(i1, v1);
            pg->GetCoord(i2, v2);
            pg->GetCoord(i3, v3);

            Vector4 pivot;
            pg->GetPivot(i1, pivot);
            
#define CALCULATE_TRIANGLE_SQUEARE(v1, v2, v3) ((((v2) - (v1)).CrossProduct((v3) - (v1))).Length() / 2.f)

            if (pivot.w > DAVA::EPSILON) //billboard
            {
                float32 square = CALCULATE_TRIANGLE_SQUEARE(
                DAVA::Vector3(v1.x, v1.y, 0.f),
                DAVA::Vector3(v2.x, v2.y, 0.f),
                DAVA::Vector3(v3.x, v3.y, 0.f)
                );

                info.leafsSquareAbsolute = info.leafsSquareAbsolute + square;
            }
            else
            {
                info.leafsSquareAbsolute.x += CALCULATE_TRIANGLE_SQUEARE(
                DAVA::Vector3(v1.x, 0.f, v1.z),
                DAVA::Vector3(v2.x, 0.f, v2.z),
                DAVA::Vector3(v3.x, 0.f, v3.z)
                );
                info.leafsSquareAbsolute.y += CALCULATE_TRIANGLE_SQUEARE(
                DAVA::Vector3(0.f, v1.y, v1.z),
                DAVA::Vector3(0.f, v2.y, v2.z),
                DAVA::Vector3(0.f, v3.y, v3.z)
                );
                info.leafsSquareAbsolute.z += CALCULATE_TRIANGLE_SQUEARE(
                DAVA::Vector3(v1.x, v1.y, 0.f),
                DAVA::Vector3(v2.x, v2.y, 0.f),
                DAVA::Vector3(v3.x, v3.y, 0.f)
                );
            }
            
#undef CALCULATE_TRIANGLE_SQUEARE
        }
    }

    Matrix4* worldTransformPtr = renderObject->GetWorldTransformPtr();
    Vector3 objectScale = (worldTransformPtr != nullptr) ? worldTransformPtr->GetScaleVector() : Vector3(1.f, 1.f, 1.f);
    info.leafsSquareAbsolute.x *= objectScale.y * objectScale.z;
    info.leafsSquareAbsolute.y *= objectScale.x * objectScale.z;
    info.leafsSquareAbsolute.z *= objectScale.x * objectScale.y;

    Vector3 bboxSize = renderObject->GetWorldBoundingBox().GetSize();
    info.leafsSquareRelative.x = info.leafsSquareAbsolute.x / (bboxSize.y * bboxSize.z);
    info.leafsSquareRelative.y = info.leafsSquareAbsolute.y / (bboxSize.x * bboxSize.z);
    info.leafsSquareRelative.z = info.leafsSquareAbsolute.z / (bboxSize.x * bboxSize.y);

    return info;
}

void SceneInfo::TexturesReloaded()
{
    SceneHelper::TextureCollector collector(SceneHelper::TextureCollector::OnlyActiveTextures);
    SceneHelper::EnumerateSceneTextures(activeScene, collector);
    sceneTexturesSize = CalculateTextureSize(collector.GetTextures());

    RefreshSceneGeneralInfo();
}

void SceneInfo::SpritesReloaded()
{
    particleTextures.clear();

    CollectParticlesData();
    particleTexturesSize = CalculateTextureSize(particleTextures);

    RefreshSceneGeneralInfo();
}

void SceneInfo::OnQualityChanged()
{
    RefreshAllData();
}

void SceneInfo::InitializeVegetationInfoSection()
{
    QtPropertyData* header = CreateInfoHeader("Vegetation Info");

    AddChild("Poly count", "Visible triangles count", header);
    AddChild("Instance count", "Visible vegetation instance count", header);

    AddChild("Poly count in LOD #0", "Visible triangles count in LOD #0", header);
    AddChild("Poly count in LOD #1", "Visible triangles count in LOD #1", header);
    AddChild("Poly count in LOD #2", "Visible triangles count in LOD #2", header);

    AddChild("Instance count in LOD #0", "Visible vegetation instance count in LOD #0", header);
    AddChild("Instance count in LOD #1", "Visible vegetation instance count in LOD #1", header);
    AddChild("Instance count in LOD #2", "Visible vegetation instance count in LOD #2", header);

    AddChild("Poly count in layer #0", "Visible triangles count in layer #0", header);
    AddChild("Poly count in layer #1", "Visible triangles count in layer #1", header);
    AddChild("Poly count in layer #2", "Visible triangles count in layer #2", header);
    AddChild("Poly count in layer #3", "Visible triangles count in layer #3", header);

    AddChild("Instance count in layer #0", "Visible vegetation instance count in layer #0", header);
    AddChild("Instance count in layer #1", "Visible vegetation instance count in layer #1", header);
    AddChild("Instance count in layer #2", "Visible vegetation instance count in layer #2", header);
    AddChild("Instance count in layer #3", "Visible vegetation instance count in layer #3", header);

    AddChild("Poly count in LODs in layer #0", "Vegetation model info for layer_0 entity. LOD 0 / LOD 1 / LOD 2", header);
    AddChild("Poly count in LODs in layer #1", "Vegetation model info for layer_1 entity. LOD 0 / LOD 1 / LOD 2", header);
    AddChild("Poly count in LODs in layer #2", "Vegetation model info for layer_2 entity. LOD 0 / LOD 1 / LOD 2", header);
    AddChild("Poly count in LODs in layer #3", "Vegetation model info for layer_3 entity. LOD 0 / LOD 1 / LOD 2", header);

    AddChild("Quadtree leaf count", "Number of quad tree leafs covering visible vegetation surface", header);

    AddChild("Quadtree leaf count in LOD #0", "Number of quad tree leafs covering visible vegetation surface for LOD #0", header);
    AddChild("Quadtree leaf count in LOD #1", "Number of quad tree leafs covering visible vegetation surface for LOD #1", header);
    AddChild("Quadtree leaf count in LOD #2", "Number of quad tree leafs covering visible vegetation surface for LOD #2", header);

    AddChild("RenderBatch count", "Number of renderbatches used to render visible vegetation", header);
}

void SceneInfo::RefreshVegetationInfoSection()
{
    using namespace DAVA;
    if (activeScene != NULL)
    {
        DAVA::Vector<DAVA::VegetationRenderObject*> activeVegetationObjects;
        activeScene->editorVegetationSystem->GetActiveVegetation(activeVegetationObjects);

        DAVA::uint32 activeObjectsSize = static_cast<DAVA::uint32>(activeVegetationObjects.size());
        QtPropertyData* header = GetInfoHeader("Vegetation Info");

        VegetationMetrics metrics;
        if (activeObjectsSize == 1)
        {
            VegetationRenderObject* renderObj = activeVegetationObjects[0];
            renderObj->CollectMetrics(metrics);
        }

        if (metrics.isValid)
        {
            static const char* INSTANCE_PER_LOD_HEADER[] =
            {
              "Instance count in LOD #0",
              "Instance count in LOD #1",
              "Instance count in LOD #2"
            };

            static const char* INSTANCE_PER_LAYER_HEADER[] =
            {
              "Instance count in layer #0",
              "Instance count in layer #1",
              "Instance count in layer #2",
              "Instance count in layer #3"
            };

            static const char* POLY_PER_LOD_HEADER[] =
            {
              "Poly count in LOD #0",
              "Poly count in LOD #1",
              "Poly count in LOD #2"
            };

            static const char* POLY_PER_LAYER_HEADER[] =
            {
              "Poly count in layer #0",
              "Poly count in layer #1",
              "Poly count in layer #2",
              "Poly count in layer #3"
            };

            static const char* QUADTREELEAF_PER_LOD_HEADER[] =
            {
              "Quadtree leaf count in LOD #0",
              "Quadtree leaf count in LOD #1",
              "Quadtree leaf count in LOD #2"
            };

            static const char* POLY_PER_LOD_PER_LAYER_HEADER[] =
            {
              "Poly count in LODs in layer #0",
              "Poly count in LODs in layer #1",
              "Poly count in LODs in layer #2",
              "Poly count in LODs in layer #3"
            };

            uint32 totalInstanceCount = 0;
            for (uint32 lodIndex = 0; lodIndex < COUNT_OF(INSTANCE_PER_LOD_HEADER); ++lodIndex)
            {
                if (metrics.visibleInstanceCountPerLOD.size() > lodIndex)
                {
                    totalInstanceCount += metrics.visibleInstanceCountPerLOD[lodIndex];
                }

                SetChild(INSTANCE_PER_LOD_HEADER[lodIndex], metrics.visibleInstanceCountPerLOD[lodIndex], header);
            }
            SetChild("Instance count", totalInstanceCount, header);

            for (uint32 layerIndex = 0; layerIndex < COUNT_OF(INSTANCE_PER_LAYER_HEADER); ++layerIndex)
            {
                if (metrics.visibleInstanceCountPerLayer.size() > layerIndex)
                {
                    SetChild(INSTANCE_PER_LAYER_HEADER[layerIndex], metrics.visibleInstanceCountPerLayer[layerIndex], header);
                }
            }

            uint32 totalPolyCount = 0;
            for (uint32 lodIndex = 0; lodIndex < COUNT_OF(POLY_PER_LOD_HEADER); ++lodIndex)
            {
                if (metrics.visiblePolyCountPerLOD.size() > lodIndex)
                {
                    totalPolyCount += metrics.visiblePolyCountPerLOD[lodIndex];
                }

                SetChild(POLY_PER_LOD_HEADER[lodIndex], metrics.visiblePolyCountPerLOD[lodIndex], header);
            }
            SetChild("Poly count", totalPolyCount, header);

            for (uint32 layerIndex = 0; layerIndex < COUNT_OF(POLY_PER_LAYER_HEADER); ++layerIndex)
            {
                if (metrics.visiblePolyCountPerLayer.size() > layerIndex)
                {
                    SetChild(POLY_PER_LAYER_HEADER[layerIndex], metrics.visiblePolyCountPerLayer[layerIndex], header);
                }
            }

            uint32 totalLeafCount = 0;
            for (uint32 lodIndex = 0; lodIndex < COUNT_OF(QUADTREELEAF_PER_LOD_HEADER); ++lodIndex)
            {
                if (metrics.quadTreeLeafCountPerLOD.size() > lodIndex)
                {
                    totalLeafCount += metrics.quadTreeLeafCountPerLOD[lodIndex];
                }

                SetChild(QUADTREELEAF_PER_LOD_HEADER[lodIndex], metrics.quadTreeLeafCountPerLOD[lodIndex], header);
            }
            SetChild("Quadtree leaf count", totalLeafCount, header);

            SetChild("RenderBatch count", metrics.renderBatchCount, header);

            for (uint32 layerIndex = 0; layerIndex < COUNT_OF(POLY_PER_LOD_PER_LAYER_HEADER); ++layerIndex)
            {
                if (metrics.polyCountPerLayerPerLod.size() > layerIndex)
                {
                    String str = Format("%d / %d / %d", metrics.polyCountPerLayerPerLod[layerIndex][0], metrics.polyCountPerLayerPerLod[layerIndex][1], metrics.polyCountPerLayerPerLod[layerIndex][2]);
                    SetChild(POLY_PER_LOD_PER_LAYER_HEADER[layerIndex], str.c_str(), header);
                }
            }
        }
        else
        {
            QString dummy;
            if (activeObjectsSize > 1)
            {
                dummy = "error";
            }

            SetChild("Poly count", dummy, header);
            SetChild("Instance count", dummy, header);

            SetChild("Poly count in LOD #0", dummy, header);
            SetChild("Poly count in LOD #1", dummy, header);
            SetChild("Poly count in LOD #2", dummy, header);

            SetChild("Instance count in LOD #0", dummy, header);
            SetChild("Instance count in LOD #1", dummy, header);
            SetChild("Instance count in LOD #2", dummy, header);

            SetChild("Poly count in layer #0", dummy, header);
            SetChild("Poly count in layer #1", dummy, header);
            SetChild("Poly count in layer #2", dummy, header);
            SetChild("Poly count in layer #3", dummy, header);

            SetChild("Instance count in layer #0", dummy, header);
            SetChild("Instance count in layer #1", dummy, header);
            SetChild("Instance count in layer #2", dummy, header);
            SetChild("Instance count in layer #3", dummy, header);

            SetChild("Poly count in LODs in layer #0", dummy, header);
            SetChild("Poly count in LODs in layer #1", dummy, header);
            SetChild("Poly count in LODs in layer #2", dummy, header);
            SetChild("Poly count in LODs in layer #3", dummy, header);

            SetChild("Quadtree leaf count", dummy, header);

            SetChild("Quadtree leaf count in LOD #0", dummy, header);
            SetChild("Quadtree leaf count in LOD #1", dummy, header);
            SetChild("Quadtree leaf count in LOD #2", dummy, header);

            SetChild("RenderBatch count", dummy, header);
        }
    }
}

void SceneInfo::InitializeLayersSection()
{
    using namespace DAVA;
    QtPropertyData* header = CreateInfoHeader("Fragments Info");

    for (int32 i = 0; i < VisibilityQueryResults::QUERY_INDEX_COUNT; ++i)
    {
        FastName queryName = VisibilityQueryResults::GetQueryIndexName(static_cast<VisibilityQueryResults::eQueryIndex>(i));
        AddChild(queryName.c_str(), header);
    }
}

void SceneInfo::RefreshLayersSection()
{
    using namespace DAVA;
    if (activeScene)
    {
        const RenderStats& renderStats = activeScene->GetRenderStats();
        QtPropertyData* header = GetInfoHeader("Fragments Info");

        static const uint32 dava3DViewMargin = 3; //TODO: add 3d view margin to ResourceEditor settings
        float32 viewportSize = (float32)(Renderer::GetFramebufferWidth() - dava3DViewMargin * 2) * (Renderer::GetFramebufferHeight() - dava3DViewMargin * 2);

        for (int32 i = 0; i < VisibilityQueryResults::QUERY_INDEX_COUNT; ++i)
        {
            FastName queryName = VisibilityQueryResults::GetQueryIndexName(static_cast<VisibilityQueryResults::eQueryIndex>(i));
            auto it = renderStats.visibilityQueryResults.find(queryName);
            uint32 fragmentStats = 0U;
            if (it != renderStats.visibilityQueryResults.end())
            {
                fragmentStats = it->second;
            }

            String str = Format("%d / %.2f%%", fragmentStats, (fragmentStats * 100.0) / viewportSize);
            SetChild(queryName.c_str(), str.c_str(), header);
        }
    }
}

EditorStatisticsSystem* SceneInfo::GetCurrentEditorStatisticsSystem() const
{
    if (activeScene == nullptr)
        return nullptr;

    return activeScene->editorStatisticsSystem;
}
