#pragma once

#ifdef WITH_SCENE_PERFORMANCE_TESTS

#include "SceneViewerApp.h"
#include "BaseScreen.h"
#include "UIControls/Sector.h"

#include <GridTest.h>

class PerformanceResultsScreen : public BaseScreen
{
public:
    PerformanceResultsScreen(SceneViewerData& data);

    // UIScreen
    void LoadResources() override;
    void UnloadResources() override;

    // UIControl
    void Update(DAVA::float32 timeElapsed) override;

private:
    void OnBackButton(BaseObject* caller, void* param, void* callerData);
    void OnSectorPressed(BaseObject* caller, void* param, void* callerData);

    void AddButtons();
    void RemoveButtons();

    void AddSectors();
    void RemoveSectors();

    void AddBackgroundMap();
    void RemoveBackgroundMap();

    void AddColorBoxes();
    void RemoveColorBoxes();

    void AddResultsText();
    void RemoveResultsText();

    void AddPreviewControls();
    void RemovePreviewControls();

    void SaveTexture(DAVA::Texture* screenshot);

    DAVA::float32 SceneDistanceToScreenDistance(DAVA::float32 distance);
    DAVA::Vector2 ScenePointToScreenPoint(DAVA::Vector3 scenePoint);
    SectorColor EvaluateSectorType(DAVA::float32 fps);
    void SetSamplePosition(DAVA::Scene* scene, const GridTestSample& sample);
    void SelectLowestFpsSector();
    void SetSectorSelected(Sector*);

private:
    SceneViewerData& data;

    DAVA::Camera* camera = nullptr;

    DAVA::Rect infoColumnRect;
    DAVA::Rect panoramaRect;

    DAVA::Vector<DAVA::ScopedPtr<Sector>> sectors;
    DAVA::UnorderedMap<Sector*, DAVA::uint32> sectorToSample;
    Sector* selectedSector = nullptr;
    DAVA::ScopedPtr<SectorColorBox> greenColorBox;
    DAVA::ScopedPtr<SectorColorBox> yellowColorBox;
    DAVA::ScopedPtr<SectorColorBox> redColorBox;
    DAVA::ScopedPtr<DAVA::UIStaticText> greenBoxText;
    DAVA::ScopedPtr<DAVA::UIStaticText> yellowBoxText;
    DAVA::ScopedPtr<DAVA::UIStaticText> redBoxText;
    DAVA::ScopedPtr<DAVA::UIStaticText> previewFpsText;
    DAVA::ScopedPtr<DAVA::UIStaticText> fpsResultsText;
    DAVA::ScopedPtr<DAVA::UI3DView> preview;
    DAVA::ScopedPtr<DAVA::UIButton> backButton;
};
#endif
