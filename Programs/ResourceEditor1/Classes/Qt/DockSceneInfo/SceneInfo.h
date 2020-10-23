#pragma once

#include "Tools/QtPosSaver/QtPosSaver.h"
#include "Tools/QtPropertyEditor/QtPropertyEditor.h"
#include "Tools/QtPropertyEditor/PropertyEditorStateHelper.h"

#include <QtTools/Updaters/LazyUpdater.h>

#include <QShowEvent>

#include <Base/Set.h>

namespace DAVA
{
class Sprite;

namespace TArc
{
class FieldBinder;
}
}

class RECommandNotificationObject;
class SceneEditor2;
class SelectableGroup;
class EditorStatisticsSystem;

class SceneInfo : public QtPropertyEditor
{
    Q_OBJECT

protected:
    struct SpeedTreeInfo
    {
        DAVA::Vector3 leafsSquareAbsolute;
        DAVA::Vector3 leafsSquareRelative;
    };

public:
    SceneInfo(QWidget* parent = 0);
    ~SceneInfo() override;

public slots:
    void UpdateInfoByTimer();
    void TexturesReloaded();
    void SpritesReloaded();
    void OnQualityChanged();

protected slots:
    void SceneActivated(SceneEditor2* scene);
    void SceneDeactivated(SceneEditor2* scene);
    void SceneStructureChanged(SceneEditor2* scene, DAVA::Entity* parent);
    void OnCommmandExecuted(SceneEditor2* scene, const RECommandNotificationObject& commandNotification);
    void OnThemeChanged();

private:
    void OnSelectionChanged(const DAVA::Any& selection);

    void showEvent(QShowEvent* event) override;

    EditorStatisticsSystem* GetCurrentEditorStatisticsSystem() const;

    void InitializeInfo();
    void InitializeGeneralSection();
    void Initialize3DDrawSection();
    void InitializeLODSectionInFrame();
    void InitializeLODSectionForSelection();

    void InitializeSpeedTreeInfoSelection();

    void InitializeVegetationInfoSection();

    void InitializeLayersSection();

    void RefreshSceneGeneralInfo();
    void Refresh3DDrawInfo();
    void RefreshLODInfoInFrame();
    void RefreshLODInfoForSelection();
    void RefreshSpeedTreeInfoSelection();

    void RefreshVegetationInfoSection();

    void RefreshLayersSection();

    void RefreshAllData();

    void ClearData();
    void ClearSelectionData();

    void SaveTreeState();
    void RestoreTreeState();

    QtPropertyData* CreateInfoHeader(const QString& key);
    QtPropertyData* GetInfoHeader(const QString& key);

    void AddChild(const QString& key, QtPropertyData* parent);
    void AddChild(const QString& key, const QString& toolTip, QtPropertyData* parent);
    void SetChild(const QString& key, const QVariant& value, QtPropertyData* parent);
    bool HasChild(const QString& key, QtPropertyData* parent);

    void CollectSceneData();
    void CollectParticlesData();
    void ProcessParticleSprite(DAVA::Sprite* sprite, DAVA::Set<DAVA::Sprite*>& sprites);
    void CollectSpeedInfo(const SelectableGroup* forGroup);
    void CollectSelectedRenderObjects(const SelectableGroup* selected);
    void CollectSelectedRenderObjectsRecursivly(DAVA::Entity* entity);

    void CollectTexture(DAVA::TexturesMap& textures, const DAVA::FilePath& pathname, DAVA::Texture* tex);

    static DAVA::uint32 CalculateTextureSize(const DAVA::TexturesMap& textures);

    static DAVA::uint32 GetTrianglesForNotLODEntityRecursive(DAVA::Entity* entity, bool onlyVisibleBatches);

    static SpeedTreeInfo GetSpeedTreeInfo(DAVA::SpeedTreeObject* renderObject);

protected:
    QtPosSaver posSaver;
    PropertyEditorStateHelper treeStateHelper;

    SceneEditor2* activeScene = nullptr;
    DAVA::Vector<DAVA::Entity*> nodesAtScene;
    DAVA::Landscape* landscape = nullptr;

    DAVA::TexturesMap particleTextures;

    DAVA::Vector<DAVA::DataNode*> dataNodesAtScene;

    DAVA::Vector<SpeedTreeInfo> speedTreesInfo;

    DAVA::uint32 sceneTexturesSize = 0;
    DAVA::uint32 particleTexturesSize = 0;

    DAVA::uint32 emittersCount = 0;
    DAVA::uint32 spritesCount = 0;

    DAVA::Vector<DAVA::RenderObject*> visibilityArray;
    DAVA::Set<DAVA::RenderObject*> selectedRenderObjects;

    LazyUpdater infoUpdated;

    bool isUpToDate = false;

    std::unique_ptr<DAVA::TArc::FieldBinder> fieldBinder;
};
