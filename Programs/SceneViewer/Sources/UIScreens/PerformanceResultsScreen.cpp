#ifdef WITH_SCENE_PERFORMANCE_TESTS
#include "PerformanceResultsScreen.h"
#include "SceneViewerApp.h"

#include <Math/Polygon2.h>
#include <UI/Layouts/UIAnchorComponent.h>

namespace PerformanceResultsScreenDetails
{
using namespace DAVA;

const float32 SECTOR_AXIFUGAL_OFFSET_IN_PIXELS = 2.f;
const float32 SECTOR_MARGIN_IN_PIXELS = 2.f;

const float32 MARGIN = 10.f;

const float32 BUTTON_HEIGHT = 60.f;

const float32 INFO_COLOR_BOXES_Y0 = MARGIN;
const float32 INFO_FPS_RESULTS_Y0 = 120.f;

const float32 LOW_FPS_THRESHOLD = 40.0f;
const float32 MEDIUM_FPS_THRESHOLD = 50.0f;
}

PerformanceResultsScreen::PerformanceResultsScreen(SceneViewerData& data)
    : data(data)
{
    using namespace PerformanceResultsScreenDetails;

    DAVA::float32 panoramaSide = GetSize().dy - 2 * MARGIN;
    panoramaRect = { MARGIN, MARGIN, panoramaSide, panoramaSide };

    infoColumnRect.x = MARGIN + panoramaRect.dx + MARGIN;
    infoColumnRect.y = MARGIN;
    infoColumnRect.dx = GetSize().dx - infoColumnRect.x - MARGIN;
    infoColumnRect.dy = GetSize().dy - MARGIN - MARGIN;
}

void PerformanceResultsScreen::LoadResources()
{
    BaseScreen::LoadResources();
    AddButtons();
    AddBackgroundMap();
    AddSectors();
    AddColorBoxes();
    AddResultsText();
    AddPreviewControls();
}

void PerformanceResultsScreen::UnloadResources()
{
    RemovePreviewControls();
    RemoveResultsText();
    RemoveColorBoxes();
    RemoveSectors();
    RemoveBackgroundMap();
    RemoveButtons();
    BaseScreen::UnloadResources();
}

void PerformanceResultsScreen::AddButtons()
{
    using namespace PerformanceResultsScreenDetails;
    DAVA::Rect rect(infoColumnRect.x, infoColumnRect.y + infoColumnRect.dy - BUTTON_HEIGHT, infoColumnRect.dx, BUTTON_HEIGHT);
    backButton = CreateButton(rect, L"Back to scene");
    backButton->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &PerformanceResultsScreen::OnBackButton));
    AddControl(backButton);
}

void PerformanceResultsScreen::RemoveButtons()
{
    RemoveControl(backButton);
}

void PerformanceResultsScreen::AddBackgroundMap()
{
    using namespace PerformanceResultsScreenDetails;
    using namespace DAVA;

    RefPtr<UIControl> panoramaContol(new UIControl());
    panoramaContol->SetName("Panorama");

    UIControlBackground* panoramaBackground = panoramaContol->GetOrCreateComponent<UIControlBackground>();
    ScopedPtr<Sprite> sprite(Sprite::CreateFromImage(data.gridTestResult.panoramaImage));
    panoramaBackground->SetSprite(sprite);
    panoramaBackground->SetDrawType(UIControlBackground::DRAW_STRETCH_BOTH);
    panoramaBackground->SetAlign(eAlign::ALIGN_LEFT | eAlign::ALIGN_TOP);

    UIAnchorComponent* panoramaAnchor = panoramaContol->GetOrCreateComponent<UIAnchorComponent>();
    panoramaAnchor->SetLeftAnchor(panoramaRect.x);
    panoramaAnchor->SetTopAnchor(panoramaRect.y);
    panoramaAnchor->SetBottomAnchor(GetSize().dy - panoramaRect.y - panoramaRect.dy);
    panoramaAnchor->SetRightAnchor(GetSize().dx - panoramaRect.x - panoramaRect.dx);

    panoramaAnchor->SetLeftAnchorEnabled(true);
    panoramaAnchor->SetTopAnchorEnabled(true);
    panoramaAnchor->SetBottomAnchorEnabled(true);
    panoramaAnchor->SetRightAnchorEnabled(true);

    AddControl(panoramaContol.Get());
}

void PerformanceResultsScreen::RemoveBackgroundMap()
{
    using namespace DAVA;

    UIControl* panoramaContol = FindByPath("Panorama");
    UIControlBackground* panoramaBackground = panoramaContol->GetComponent<UIControlBackground>();
    panoramaBackground->SetSprite(nullptr);
}

void PerformanceResultsScreen::AddSectors()
{
    using namespace PerformanceResultsScreenDetails;
    using namespace DAVA;

    const float32& sceneSize = data.gridTestResult.sceneSize;
    const float32& step = data.gridTestResult.gridStep;
    const float32& angleStep = data.gridTestResult.sampleAngleDegrees;

    Vector<GridTestSample>& samples = data.gridTestResult.samples;
    sectors.reserve(samples.size());

    for (uint32 sampleIndex = 0; sampleIndex < samples.size(); ++sampleIndex)
    {
        GridTestSample& sample = samples[sampleIndex];

        Vector2 samplePointOnScreen = ScenePointToScreenPoint(sample.pos);
        samplePointOnScreen.x += SECTOR_AXIFUGAL_OFFSET_IN_PIXELS * sample.cos;
        samplePointOnScreen.y -= SECTOR_AXIFUGAL_OFFSET_IN_PIXELS * sample.sine;
        float32 startAngle = sample.angle - angleStep / 2;
        float32 endAngle = sample.angle + angleStep / 2;
        float32 radius = SceneDistanceToScreenDistance(step / 2) - SECTOR_MARGIN_IN_PIXELS - SECTOR_AXIFUGAL_OFFSET_IN_PIXELS;
        SectorColor sectorType = EvaluateSectorType(sample.fps);
        DAVA::ScopedPtr<Sector> sector(new Sector(samplePointOnScreen, startAngle, endAngle, radius, sectorType));
        sector->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &PerformanceResultsScreen::OnSectorPressed));
        sectors.push_back(sector);
        sectorToSample[sector] = sampleIndex;
        AddControl(sector);
    }
    selectedSector = nullptr;
}

void PerformanceResultsScreen::RemoveSectors()
{
    for (DAVA::ScopedPtr<Sector>& sector : sectors)
    {
        RemoveControl(sector);
    }
    sectors.clear();
    sectorToSample.clear();
    selectedSector = nullptr;
}

void PerformanceResultsScreen::AddColorBoxes()
{
    using namespace PerformanceResultsScreenDetails;
    using namespace DAVA;

    static const float32 boxMargin = 2.f;
    float32 fontSize = fontSmallSize;
    float32 boxSize = fontSize - boxMargin - boxMargin;
    float32 boxDistanceY = fontSize + 5.f;

    auto AddBoxAndText = [this, fontSize, boxSize](DAVA::ScopedPtr<SectorColorBox>& box, DAVA::ScopedPtr<DAVA::UIStaticText>& text, float32 y0, SectorColor type)
    {
        box = new SectorColorBox(Rect(infoColumnRect.x, y0 + boxMargin, boxSize, boxSize), type);
        AddControl(box);

        Rect textRect;
        textRect.x = infoColumnRect.x + boxMargin + boxSize + boxMargin;
        textRect.y = y0;
        textRect.dx = GetSize().dx - textRect.x - MARGIN;
        textRect.dy = fontSize;

        text = new UIStaticText(textRect);
        text->SetFont(font);
        text->SetFontSize(fontSmallSize);
        text->SetTextColor(Color::White);
        text->SetTextColorInheritType(UIControlBackground::COLOR_IGNORE_PARENT);
        text->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
        AddControl(text);
    };

    AddBoxAndText(greenColorBox, greenBoxText, INFO_COLOR_BOXES_Y0, SectorColor::Green);
    greenBoxText->SetText(Format(L"fps > %.f", MEDIUM_FPS_THRESHOLD));

    AddBoxAndText(yellowColorBox, yellowBoxText, INFO_COLOR_BOXES_Y0 + boxDistanceY, SectorColor::Yellow);
    yellowBoxText->SetText(Format(L"fps %.f - %.f", LOW_FPS_THRESHOLD, MEDIUM_FPS_THRESHOLD));

    AddBoxAndText(redColorBox, redBoxText, INFO_COLOR_BOXES_Y0 + boxDistanceY + boxDistanceY, SectorColor::Red);
    redBoxText->SetText(Format(L"fps < %.f", LOW_FPS_THRESHOLD));
}

void PerformanceResultsScreen::RemoveColorBoxes()
{
    RemoveControl(greenColorBox);
    RemoveControl(yellowBoxText);
    RemoveControl(redColorBox);
    RemoveControl(greenBoxText);
    RemoveControl(yellowBoxText);
    RemoveControl(redBoxText);
    greenColorBox.reset();
    yellowBoxText.reset();
    redColorBox.reset();
    greenBoxText.reset();
    yellowBoxText.reset();
    redBoxText.reset();
}

void PerformanceResultsScreen::AddResultsText()
{
    using namespace PerformanceResultsScreenDetails;
    using namespace DAVA;

    fpsResultsText = new UIStaticText(Rect(infoColumnRect.x, INFO_FPS_RESULTS_Y0, infoColumnRect.dx, 200.f));
    fpsResultsText->SetFont(font);
    fpsResultsText->SetTextColor(Color::White);
    fpsResultsText->SetTextColorInheritType(UIControlBackground::COLOR_IGNORE_PARENT);
    fpsResultsText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    fpsResultsText->SetMultiline(true);
    fpsResultsText->SetText(Format(L"Average FPS: %.1f\nMin FPS: %.1f\nMax FPS: %.1f", data.gridTestResult.avgFPS, data.gridTestResult.minFPS, data.gridTestResult.maxFPS));
    AddControl(fpsResultsText);
}

void PerformanceResultsScreen::RemoveResultsText()
{
    RemoveControl(fpsResultsText);
    fpsResultsText.reset();
}

void PerformanceResultsScreen::AddPreviewControls()
{
    using namespace DAVA;
    using namespace PerformanceResultsScreenDetails;

    Rect previewRect;
    previewRect.dx = infoColumnRect.dx;
    previewRect.dy = infoColumnRect.dx / data.screenAspect;
    previewRect.x = infoColumnRect.x;
    previewRect.y = infoColumnRect.y + infoColumnRect.dy - BUTTON_HEIGHT - MARGIN - previewRect.dy;

    preview = new UI3DView(previewRect);
    preview->SetScene(data.scene);
    AddControl(preview);

    camera = data.scene->GetCurrentCamera();
    DVASSERT(camera != nullptr);
    camera->SetupPerspective(70.f, data.screenAspect, 0.5f, 2500.f);
    camera->SetUp(DAVA::Vector3(0.f, 0.f, 1.f));

    Rect fpsRect;
    fpsRect.dx = infoColumnRect.dx;
    fpsRect.dy = fontSize;
    fpsRect.x = infoColumnRect.x;
    fpsRect.y = previewRect.y - fpsRect.dy;

    previewFpsText = new UIStaticText(fpsRect);
    previewFpsText->SetFont(font);
    previewFpsText->SetTextColor(Color::White);
    previewFpsText->SetTextColorInheritType(UIControlBackground::COLOR_IGNORE_PARENT);
    previewFpsText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    AddControl(previewFpsText);

    SelectLowestFpsSector();
}

void PerformanceResultsScreen::SelectLowestFpsSector()
{
    DAVA::Vector<GridTestSample>& samples = data.gridTestResult.samples;

    DAVA::float32 lowestFps = 100.f;
    Sector* lowestFpsSector = nullptr;

    for (auto& entry : sectorToSample)
    {
        DAVA::uint32 sampleId = entry.second;
        DVASSERT(sampleId < samples.size());
        if (samples[sampleId].fps < lowestFps)
        {
            lowestFps = samples[sampleId].fps;
            lowestFpsSector = entry.first;
        }
    }

    SetSectorSelected(lowestFpsSector);
}

void PerformanceResultsScreen::RemovePreviewControls()
{
    RemoveControl(preview);
    RemoveControl(previewFpsText);
    preview.reset();
    previewFpsText.reset();
}

SectorColor PerformanceResultsScreen::EvaluateSectorType(DAVA::float32 fps)
{
    using namespace PerformanceResultsScreenDetails;

    if (fps > MEDIUM_FPS_THRESHOLD)
        return SectorColor::Green;
    else if (fps > LOW_FPS_THRESHOLD)
        return SectorColor::Yellow;
    else
        return SectorColor::Red;
}

DAVA::float32 PerformanceResultsScreen::SceneDistanceToScreenDistance(DAVA::float32 sceneDist)
{
    return (sceneDist / data.gridTestResult.sceneSize) * panoramaRect.dx;
}

DAVA::Vector2 PerformanceResultsScreen::ScenePointToScreenPoint(DAVA::Vector3 scenePoint)
{
    DAVA::float32& sceneSize = data.gridTestResult.sceneSize;
    DAVA::float32& sceneMin = data.gridTestResult.sceneMin;
    DAVA::float32& sceneMax = data.gridTestResult.sceneMax;

    DAVA::Vector2 screenPoint;
    screenPoint.x = (scenePoint.x - sceneMin) / sceneSize * panoramaRect.dx + panoramaRect.x;
    screenPoint.y = (sceneMax - scenePoint.y) / sceneSize * panoramaRect.dy + panoramaRect.y;

    return screenPoint;
}

void PerformanceResultsScreen::Update(DAVA::float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
}

void PerformanceResultsScreen::OnBackButton(DAVA::BaseObject* caller, void* param, void* callerData)
{
    SetPreviousScreen();
}

void PerformanceResultsScreen::SetSamplePosition(DAVA::Scene* scene, const GridTestSample& sample)
{
    camera->SetPosition(sample.pos);
    camera->SetDirection(DAVA::Vector3(sample.cos, sample.sine, 0));
}

void PerformanceResultsScreen::OnSectorPressed(DAVA::BaseObject* caller, void* param, void* callerData)
{
    Sector* sector = static_cast<Sector*>(caller);
    if (sector == nullptr)
    {
        DVASSERT(false, "can't cast to Sector*");
        return;
    }

    SetSectorSelected(sector);
}

void PerformanceResultsScreen::SetSectorSelected(Sector* sector)
{
    DVASSERT(sector != nullptr);

    if (selectedSector != nullptr)
    {
        selectedSector->SetMode(Sector::UNSELECTED);
        previewFpsText->SetText(L"");
    }

    auto it = sectorToSample.find(sector);
    if (it == sectorToSample.end())
    {
        DVASSERT(false, "Can'find sector");
        return;
    }

    DAVA::uint32 sampleIndex = it->second;
    if (sampleIndex >= data.gridTestResult.samples.size())
    {
        DVASSERT(false, DAVA::Format("incorrect sample index %u. Samples count is %u", sampleIndex, data.gridTestResult.samples.size()).c_str());
        return;
    }

    GridTestSample& sample = data.gridTestResult.samples[sampleIndex];
    previewFpsText->SetText(DAVA::Format(L"%.1f FPS", sample.fps));
    previewFpsText->SetTextColor(GetColorByType(EvaluateSectorType(sample.fps)));

    SetSamplePosition(data.scene, sample);

    selectedSector = sector;
    selectedSector->SetMode(Sector::SELECTED);
}
#endif
