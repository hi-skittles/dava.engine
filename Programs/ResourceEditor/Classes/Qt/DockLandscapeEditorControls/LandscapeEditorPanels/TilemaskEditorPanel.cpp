#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorPanels/TilemaskEditorPanel.h"

#include "Classes/Qt/Scene/SceneSignals.h"
#include "Classes/Qt/Tools/TileTexturePreviewWidget/TileTexturePreviewWidget.h"
#include "Classes/Qt/Tools/SliderWidget/SliderWidget.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorShortcutManager.h"

#include <REPlatform/Commands/RECommandNotificationObject.h>
#include <REPlatform/Global/Constants.h>
#include <REPlatform/Global/StringConstants.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/TilemaskEditorSystem.h>
#include <REPlatform/Scene/Utils/ImageTools.h>

#include <Render/PixelFormatDescriptor.h>

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include "REPlatform/Commands/TilemaskEditorCommands.h"

TilemaskEditorPanel::TilemaskEditorPanel(QWidget* parent)
    : LandscapeEditorBasePanel(parent)
    , sliderWidgetBrushSize(NULL)
    , sliderWidgetStrength(NULL)
    , comboBrushImage(NULL)
    , radioDraw(NULL)
    , radioCopyPaste(NULL)
    , frameStrength(NULL)
    , frameTileTexturesPreview(NULL)
{
    InitUI();
    ConnectToSignals();
}

TilemaskEditorPanel::~TilemaskEditorPanel()
{
}

bool TilemaskEditorPanel::GetEditorEnabled()
{
    return GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->IsLandscapeEditingEnabled();
}

void TilemaskEditorPanel::SetWidgetsState(bool enabled)
{
    sliderWidgetBrushSize->setEnabled(enabled);
    sliderWidgetStrength->setEnabled(enabled);
    comboBrushImage->setEnabled(enabled);
    tileTexturePreviewWidget->setEnabled(enabled);
    radioDraw->setEnabled(enabled);
    radioCopyPaste->setEnabled(enabled);
}

void TilemaskEditorPanel::BlockAllSignals(bool block)
{
    sliderWidgetBrushSize->blockSignals(block);
    sliderWidgetStrength->blockSignals(block);
    comboBrushImage->blockSignals(block);
    tileTexturePreviewWidget->blockSignals(block);
    radioDraw->blockSignals(block);
    radioCopyPaste->blockSignals(block);
}

void TilemaskEditorPanel::InitUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    sliderWidgetBrushSize = new SliderWidget(this);
    sliderWidgetStrength = new SliderWidget(this);
    comboBrushImage = new QComboBox(this);
    tileTexturePreviewWidget = new TileTexturePreviewWidget(this);
    radioDraw = new QRadioButton(this);
    radioCopyPaste = new QRadioButton(this);

    QLabel* labelBrushImageDesc = new QLabel(this);
    QLabel* labelTileTextureDesc = new QLabel(this);
    QFrame* frameBrushImage = new QFrame(this);
    QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHBoxLayout* layoutBrushImage = new QHBoxLayout();
    QVBoxLayout* layoutTileTexture = new QVBoxLayout();

    QHBoxLayout* layoutBrushSize = new QHBoxLayout();
    QLabel* labelBrushSize = new QLabel();
    labelBrushSize->setText(DAVA::ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_CAPTION.c_str());
    layoutBrushSize->addWidget(labelBrushSize);
    layoutBrushSize->addWidget(sliderWidgetBrushSize);

    QGridLayout* layoutDrawTypes = new QGridLayout();
    layoutDrawTypes->addWidget(radioDraw, 0, 0);
    layoutDrawTypes->addWidget(radioCopyPaste, 0, 1);

    QHBoxLayout* layoutStrength = new QHBoxLayout();
    QLabel* labelStrength = new QLabel();
    labelStrength->setText(DAVA::ResourceEditor::TILEMASK_EDITOR_STRENGTH_CAPTION.c_str());
    layoutStrength->addWidget(labelStrength);
    layoutStrength->addWidget(sliderWidgetStrength);
    frameStrength = new QFrame(this);
    frameStrength->setLayout(layoutStrength);

    layoutBrushImage->addWidget(labelBrushImageDesc);
    layoutBrushImage->addWidget(comboBrushImage);
    layoutTileTexture->addWidget(labelTileTextureDesc);
    layoutTileTexture->QLayout::addWidget(tileTexturePreviewWidget);
    frameTileTexturesPreview = new QFrame(this);
    frameTileTexturesPreview->setLayout(layoutTileTexture);

    frameBrushImage->setLayout(layoutBrushImage);

    layout->addLayout(layoutBrushSize);
    layout->addWidget(frameBrushImage);
    layout->addLayout(layoutDrawTypes);
    layout->addWidget(frameStrength);
    layout->addWidget(frameTileTexturesPreview);
    layout->addSpacerItem(spacer);

    setLayout(layout);

    SetWidgetsState(false);
    BlockAllSignals(true);

    labelBrushImageDesc->setText(DAVA::ResourceEditor::TILEMASK_EDITOR_BRUSH_IMAGE_CAPTION.c_str());
    labelBrushImageDesc->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    labelTileTextureDesc->setText(DAVA::ResourceEditor::TILEMASK_EDITOR_TILE_TEXTURE_CAPTION.c_str());
    labelTileTextureDesc->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    sliderWidgetBrushSize->Init(false, DEF_BRUSH_MAX_SIZE, DEF_BRUSH_MIN_SIZE, DEF_BRUSH_MIN_SIZE);
    sliderWidgetBrushSize->SetRangeBoundaries(DAVA::ResourceEditor::BRUSH_MIN_BOUNDARY, DAVA::ResourceEditor::BRUSH_MAX_BOUNDARY);
    sliderWidgetStrength->Init(false, DEF_STRENGTH_MAX_VALUE, DEF_STRENGTH_MIN_VALUE, DEF_STRENGTH_MIN_VALUE);
    sliderWidgetStrength->SetRangeBoundaries(DEF_STRENGTH_MIN_VALUE, STRENGTH_MAX_BOUNDARY);

    radioDraw->setText(DAVA::ResourceEditor::TILEMASK_EDITOR_DRAW_CAPTION.c_str());
    radioCopyPaste->setText(DAVA::ResourceEditor::TILEMASK_EDITOR_COPY_PASTE_CAPTION.c_str());

    tileTexturePreviewWidget->setFixedHeight(130);

    layoutBrushImage->setContentsMargins(0, 0, 0, 0);
    layoutTileTexture->setContentsMargins(0, 0, 0, 0);
    layoutStrength->setContentsMargins(0, 0, 0, 0);

    InitBrushImages();
}

void TilemaskEditorPanel::ConnectToSignals()
{
    connect(SceneSignals::Instance(), &SceneSignals::LandscapeEditorToggled, this, &TilemaskEditorPanel::EditorToggled);
    connect(SceneSignals::Instance(), &SceneSignals::CommandExecuted, this, &TilemaskEditorPanel::OnCommandExecuted);

    connect(sliderWidgetBrushSize, SIGNAL(ValueChanged(int)), this, SLOT(SetBrushSize(int)));
    connect(sliderWidgetStrength, SIGNAL(ValueChanged(int)), this, SLOT(SetStrength(int)));
    connect(comboBrushImage, SIGNAL(currentIndexChanged(int)), this, SLOT(SetToolImage(int)));
    connect(tileTexturePreviewWidget, SIGNAL(SelectionChanged(int)), this, SLOT(SetDrawTexture(int)));
    connect(tileTexturePreviewWidget, &TileTexturePreviewWidget::TileColorChanged, this, &TilemaskEditorPanel::OnTileColorChanged);

    connect(radioDraw, SIGNAL(clicked()), this, SLOT(SetNormalDrawing()));
    connect(radioCopyPaste, SIGNAL(clicked()), this, SLOT(SetCopyPaste()));
}

void TilemaskEditorPanel::StoreState()
{
    DAVA::KeyedArchive* customProperties = GetOrCreateCustomProperties(GetActiveScene())->GetArchive();
    customProperties->SetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MIN,
                               static_cast<DAVA::int32>(sliderWidgetBrushSize->GetRangeMin()));
    customProperties->SetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MAX,
                               static_cast<DAVA::int32>(sliderWidgetBrushSize->GetRangeMax()));
    customProperties->SetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_STRENGTH_MIN,
                               static_cast<DAVA::int32>(sliderWidgetStrength->GetRangeMin()));
    customProperties->SetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_STRENGTH_MAX,
                               static_cast<DAVA::int32>(sliderWidgetStrength->GetRangeMax()));
}

void TilemaskEditorPanel::RestoreState()
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();
    DAVA::TilemaskEditorSystem* system = sceneEditor->GetSystem<DAVA::TilemaskEditorSystem>();

    bool enabled = system->IsLandscapeEditingEnabled();
    DAVA::int32 brushSize = BrushSizeSystemToUI(system->GetBrushSize());
    DAVA::int32 strength = StrengthSystemToUI(system->GetStrength());
    DAVA::uint32 tileTexture = system->GetTileTextureIndex();
    DAVA::int32 toolImage = system->GetToolImage();

    DAVA::int32 brushRangeMin = DEF_BRUSH_MIN_SIZE;
    DAVA::int32 brushRangeMax = DEF_BRUSH_MAX_SIZE;
    DAVA::int32 strRangeMin = DEF_STRENGTH_MIN_VALUE;
    DAVA::int32 strRangeMax = DEF_STRENGTH_MAX_VALUE;

    DAVA::KeyedArchive* customProperties = GetCustomPropertiesArchieve(sceneEditor);
    if (customProperties)
    {
        brushRangeMin = customProperties->GetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MIN, DEF_BRUSH_MIN_SIZE);
        brushRangeMax = customProperties->GetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_BRUSH_SIZE_MAX, DEF_BRUSH_MAX_SIZE);
        strRangeMin = customProperties->GetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_STRENGTH_MIN, DEF_STRENGTH_MIN_VALUE);
        strRangeMax = customProperties->GetInt32(DAVA::ResourceEditor::TILEMASK_EDITOR_STRENGTH_MAX, DEF_STRENGTH_MAX_VALUE);
    }

    SetWidgetsState(enabled);

    BlockAllSignals(true);
    sliderWidgetBrushSize->SetRangeMin(brushRangeMin);
    sliderWidgetBrushSize->SetRangeMax(brushRangeMax);
    sliderWidgetBrushSize->SetValue(brushSize);

    sliderWidgetStrength->SetRangeMin(strRangeMin);
    sliderWidgetStrength->SetRangeMax(strRangeMax);
    sliderWidgetStrength->SetValue(strength);

    UpdateTileTextures();
    tileTexturePreviewWidget->SetSelectedTexture(tileTexture);
    comboBrushImage->setCurrentIndex(toolImage);

    UpdateControls();

    BlockAllSignals(!enabled);
}

void TilemaskEditorPanel::OnEditorEnabled()
{
    UpdateTileTextures();

    SetToolImage(comboBrushImage->currentIndex());
}

void TilemaskEditorPanel::InitBrushImages()
{
    comboBrushImage->clear();

    QSize iconSize = comboBrushImage->iconSize();
    iconSize = iconSize.expandedTo(QSize(32, 32));
    comboBrushImage->setIconSize(iconSize);

    DAVA::FilePath toolsPath(DAVA::ResourceEditor::TILEMASK_EDITOR_TOOLS_PATH);

    DAVA::ScopedPtr<DAVA::FileList> fileList(new DAVA::FileList(toolsPath));
    for (DAVA::uint32 iFile = 0; iFile < fileList->GetCount(); ++iFile)
    {
        auto pathname = fileList->GetPathname(iFile);
        if (DAVA::TextureDescriptor::IsSourceTextureExtension(pathname.GetExtension()))
        {
            QIcon toolIcon(QPixmap::fromImage(DAVA::ImageTools::FromDavaImage(pathname)));

            auto fullname = pathname.GetAbsolutePathname();
            comboBrushImage->addItem(toolIcon, pathname.GetBasename().c_str(), QVariant(QString::fromStdString(fullname)));
        }
    }
}

void TilemaskEditorPanel::SplitImageToChannels(DAVA::Image* image, DAVA::Image*& r, DAVA::Image*& g, DAVA::Image*& b, DAVA::Image*& a)
{
    DVASSERT(image->GetPixelFormat() == DAVA::FORMAT_RGBA8888);

    const DAVA::int32 CHANNELS_COUNT = 4;

    DAVA::uint32 width = image->GetWidth();
    DAVA::uint32 height = image->GetHeight();
    DAVA::int32 size = width * height;

    DAVA::Image* images[CHANNELS_COUNT];
    for (DAVA::int32 i = 0; i < CHANNELS_COUNT; ++i)
    {
        images[i] = DAVA::Image::Create(width, height, DAVA::FORMAT_RGBA8888);
    }

    static const DAVA::int32 pixelSizeInBytes = 4;
    for (DAVA::int32 i = 0; i < size; ++i)
    {
        DAVA::int32 offset = i * pixelSizeInBytes;

        images[0]->data[offset] = image->data[offset];
        images[0]->data[offset + 1] = image->data[offset];
        images[0]->data[offset + 2] = image->data[offset];
        images[0]->data[offset + 3] = 255;

        images[1]->data[offset] = image->data[offset + 1];
        images[1]->data[offset + 1] = image->data[offset + 1];
        images[1]->data[offset + 2] = image->data[offset + 1];
        images[1]->data[offset + 3] = 255;

        images[2]->data[offset] = image->data[offset + 2];
        images[2]->data[offset + 1] = image->data[offset + 2];
        images[2]->data[offset + 2] = image->data[offset + 2];
        images[2]->data[offset + 3] = 255;

        images[3]->data[offset] = image->data[offset + 3];
        images[3]->data[offset + 1] = image->data[offset + 3];
        images[3]->data[offset + 2] = image->data[offset + 3];
        images[3]->data[offset + 3] = 255;
    }

    r = images[0];
    g = images[1];
    b = images[2];
    a = images[3];
}

void TilemaskEditorPanel::UpdateTileTextures()
{
    tileTexturePreviewWidget->Clear();

    DAVA::SceneEditor2* sceneEditor = GetActiveScene();
    DAVA::TilemaskEditorSystem* system = sceneEditor->GetSystem<DAVA::TilemaskEditorSystem>();

    QSize iconSize = QSize(TileTexturePreviewWidget::TEXTURE_PREVIEW_WIDTH,
                           TileTexturePreviewWidget::TEXTURE_PREVIEW_HEIGHT);

    DAVA::int32 count = static_cast<DAVA::int32>(system->GetTileTextureCount());
    DAVA::Image** images = new DAVA::Image*[count];

    DAVA::FilePath tileTexturePath = system->GetTileTexture()->GetDescriptor()->GetSourceTexturePathname();

    DAVA::Vector<DAVA::Image*> imgs;
    DAVA::ImageSystem::Load(tileTexturePath, imgs);
    DVASSERT(imgs.size() == 1);

    imgs[0]->ResizeCanvas(iconSize.width(), iconSize.height());

    SplitImageToChannels(imgs[0], images[0], images[1], images[2], images[3]);
    SafeRelease(imgs[0]);

    tileTexturePreviewWidget->SetMode(TileTexturePreviewWidget::MODE_WITH_COLORS);

    for (DAVA::int32 i = 0; i < count; ++i)
    {
        DAVA::Color color = system->GetTileColor(i);

        tileTexturePreviewWidget->AddTexture(images[i], color);

        DAVA::SafeRelease(images[i]);
    }

    SafeDelete(images);
}

void TilemaskEditorPanel::SetBrushSize(int brushSize)
{
    GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->SetBrushSize(BrushSizeUIToSystem(brushSize));
}

void TilemaskEditorPanel::SetToolImage(int imageIndex)
{
    QString s = comboBrushImage->itemData(imageIndex).toString();

    if (!s.isEmpty())
    {
        DAVA::FilePath fp(s.toStdString());
        GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->SetToolImage(fp, imageIndex);
    }
}

void TilemaskEditorPanel::SetStrength(int strength)
{
    GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->SetStrength(StrengthUIToSystem(strength));
}

void TilemaskEditorPanel::SetDrawTexture(int textureIndex)
{
    GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->SetTileTexture(textureIndex);
}

// these functions are designed to convert values from sliders in ui
// to the values suitable for tilemask editor system
DAVA::int32 TilemaskEditorPanel::BrushSizeUIToSystem(DAVA::int32 uiValue)
{
    DAVA::int32 systemValue = uiValue * DAVA::ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
    return systemValue;
}

DAVA::int32 TilemaskEditorPanel::BrushSizeSystemToUI(DAVA::int32 systemValue)
{
    DAVA::int32 uiValue = systemValue / DAVA::ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
    return uiValue;
}

DAVA::float32 TilemaskEditorPanel::StrengthUIToSystem(DAVA::int32 uiValue)
{
    // strength in tilemask editor is the real number in the range [0 .. 0.5]
    // (by default, upper bound could be different)
    DAVA::float32 max = 2.0 * DEF_STRENGTH_MAX_VALUE;
    DAVA::float32 systemValue = uiValue / max;
    return systemValue;
}

DAVA::int32 TilemaskEditorPanel::StrengthSystemToUI(DAVA::float32 systemValue)
{
    DAVA::int32 uiValue = static_cast<DAVA::int32>(systemValue * 2.f * DEF_STRENGTH_MAX_VALUE);
    return uiValue;
}
// end of convert functions ==========================

void TilemaskEditorPanel::ConnectToShortcuts()
{
    LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();

    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL), SIGNAL(activated()),
            this, SLOT(IncreaseBrushSize()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL), SIGNAL(activated()),
            this, SLOT(DecreaseBrushSize()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE), SIGNAL(activated()),
            this, SLOT(IncreaseBrushSizeLarge()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE), SIGNAL(activated()),
            this, SLOT(DecreaseBrushSizeLarge()));

    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
            this, SLOT(IncreaseStrength()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
            this, SLOT(DecreaseStrength()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
            this, SLOT(IncreaseStrengthLarge()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
            this, SLOT(DecreaseStrengthLarge()));

    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
            this, SLOT(NextTexture()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
            this, SLOT(PrevTexture()));

    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT), SIGNAL(activated()),
            this, SLOT(NextTool()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV), SIGNAL(activated()),
            this, SLOT(PrevTool()));

    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_NORMAL_DRAW_TILEMASK), SIGNAL(activated()),
            this, SLOT(SetNormalDrawing()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_COPY_PASTE_TILEMASK), SIGNAL(activated()),
            this, SLOT(SetCopyPaste()));

    shortcutManager->SetTileMaskEditorShortcutsEnabled(true);
    shortcutManager->SetBrushSizeShortcutsEnabled(true);
    shortcutManager->SetStrengthShortcutsEnabled(true);
    shortcutManager->SetTextureSwitchingShortcutsEnabled(true);
    shortcutManager->SetBrushImageSwitchingShortcutsEnabled(true);
}

void TilemaskEditorPanel::DisconnectFromShortcuts()
{
    LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();

    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL), SIGNAL(activated()),
               this, SLOT(IncreaseBrushSize()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL), SIGNAL(activated()),
               this, SLOT(DecreaseBrushSize()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE), SIGNAL(activated()),
               this, SLOT(IncreaseBrushSizeLarge()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE), SIGNAL(activated()),
               this, SLOT(DecreaseBrushSizeLarge()));

    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
               this, SLOT(IncreaseStrength()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
               this, SLOT(DecreaseStrength()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
               this, SLOT(IncreaseStrengthLarge()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
               this, SLOT(DecreaseStrengthLarge()));

    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
               this, SLOT(NextTexture()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
               this, SLOT(PrevTexture()));

    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT), SIGNAL(activated()),
               this, SLOT(NextTool()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV), SIGNAL(activated()),
               this, SLOT(PrevTool()));

    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_NORMAL_DRAW_TILEMASK), SIGNAL(activated()),
               this, SLOT(SetNormalDrawing()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_COPY_PASTE_TILEMASK), SIGNAL(activated()),
               this, SLOT(SetCopyPaste()));

    shortcutManager->SetTileMaskEditorShortcutsEnabled(false);
    shortcutManager->SetBrushSizeShortcutsEnabled(false);
    shortcutManager->SetStrengthShortcutsEnabled(false);
    shortcutManager->SetTextureSwitchingShortcutsEnabled(false);
    shortcutManager->SetBrushImageSwitchingShortcutsEnabled(false);
}

void TilemaskEditorPanel::IncreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::DecreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::IncreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::DecreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::IncreaseStrength()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   + DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::DecreaseStrength()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   - DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void TilemaskEditorPanel::IncreaseStrengthLarge()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   + DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::DecreaseStrengthLarge()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   - DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void TilemaskEditorPanel::PrevTexture()
{
    DAVA::int32 curIndex = tileTexturePreviewWidget->GetSelectedTexture();
    if (curIndex)
    {
        tileTexturePreviewWidget->SetSelectedTexture(curIndex - 1);
    }
}

void TilemaskEditorPanel::NextTexture()
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();

    DAVA::int32 curIndex = tileTexturePreviewWidget->GetSelectedTexture();
    if (curIndex < static_cast<DAVA::int32>(sceneEditor->GetSystem<DAVA::TilemaskEditorSystem>()->GetTileTextureCount()) - 1)
    {
        tileTexturePreviewWidget->SetSelectedTexture(curIndex + 1);
    }
}

void TilemaskEditorPanel::PrevTool()
{
    DAVA::int32 curIndex = comboBrushImage->currentIndex();
    if (curIndex)
    {
        comboBrushImage->setCurrentIndex(curIndex - 1);
    }
}

void TilemaskEditorPanel::NextTool()
{
    DAVA::int32 curIndex = comboBrushImage->currentIndex();
    if (curIndex < comboBrushImage->count() - 1)
    {
        comboBrushImage->setCurrentIndex(curIndex + 1);
    }
}

void TilemaskEditorPanel::OnTileColorChanged(DAVA::int32 tileNumber, DAVA::Color color)
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();
    sceneEditor->GetSystem<DAVA::TilemaskEditorSystem>()->SetTileColor(tileNumber, color);
}

void TilemaskEditorPanel::OnCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();
    if (scene != sceneEditor || !GetEditorEnabled())
    {
        return;
    }

    if (commandNotification.MatchCommandTypes<DAVA::SetTileColorCommand>())
    {
        DAVA::TilemaskEditorSystem* system = sceneEditor->GetSystem<DAVA::TilemaskEditorSystem>();
        DAVA::uint32 count = system->GetTileTextureCount();
        for (DAVA::uint32 i = 0; i < count; ++i)
        {
            DAVA::Color color = system->GetTileColor(i);
            tileTexturePreviewWidget->UpdateColor(i, color);
        }
    }
}

void TilemaskEditorPanel::SetNormalDrawing()
{
    GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->SetDrawingType(DAVA::TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);
    UpdateControls();
}

void TilemaskEditorPanel::SetCopyPaste()
{
    GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->SetDrawingType(DAVA::TilemaskEditorSystem::TILEMASK_DRAW_COPY_PASTE);
    UpdateControls();
}

void TilemaskEditorPanel::UpdateControls()
{
    DAVA::TilemaskEditorSystem::eTilemaskDrawType type = GetActiveScene()->GetSystem<DAVA::TilemaskEditorSystem>()->GetDrawingType();

    bool blocked = radioDraw->signalsBlocked();
    radioDraw->blockSignals(true);
    radioCopyPaste->blockSignals(true);

    radioDraw->setChecked(type == DAVA::TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);
    radioCopyPaste->setChecked(type == DAVA::TilemaskEditorSystem::TILEMASK_DRAW_COPY_PASTE);
    frameTileTexturesPreview->setVisible(type == DAVA::TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);
    frameStrength->setVisible(type == DAVA::TilemaskEditorSystem::TILEMASK_DRAW_NORMAL);

    radioDraw->blockSignals(blocked);
    radioCopyPaste->blockSignals(blocked);
}
