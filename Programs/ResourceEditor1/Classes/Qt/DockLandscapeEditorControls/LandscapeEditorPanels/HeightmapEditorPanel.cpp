#include "HeightmapEditorPanel.h"
#include "../../Scene/SceneSignals.h"
#include "../../Scene/SceneEditor2.h"
#include "../../Tools/SliderWidget/SliderWidget.h"
#include "Constants.h"
#include "Qt/Scene/System/LandscapeEditorDrawSystem/HeightmapProxy.h"
#include "../LandscapeEditorShortcutManager.h"

#include "ImageTools/ImageTools.h"


#include <QLayout>
#include <QLabel>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QKeyEvent>

HeightmapEditorPanel::HeightmapEditorPanel(QWidget* parent)
    : LandscapeEditorBasePanel(parent)
    , sliderWidgetBrushSize(NULL)
    , sliderWidgetStrength(NULL)
    , sliderWidgetAverageStrength(NULL)
    , comboBrushImage(NULL)
    , radioCopyPaste(NULL)
    , radioAbsDrop(NULL)
    , radioAbsolute(NULL)
    , radioAverage(NULL)
    , radioDropper(NULL)
    , radioRelative(NULL)
    , editHeight(NULL)
{
    InitUI();
    ConnectToSignals();
}

HeightmapEditorPanel::~HeightmapEditorPanel()
{
}

bool HeightmapEditorPanel::GetEditorEnabled()
{
    return GetActiveScene()->heightmapEditorSystem->IsLandscapeEditingEnabled();
}

void HeightmapEditorPanel::SetWidgetsState(bool enabled)
{
    sliderWidgetBrushSize->setEnabled(enabled);
    sliderWidgetStrength->setEnabled(enabled);
    sliderWidgetAverageStrength->setEnabled(enabled);
    comboBrushImage->setEnabled(enabled);
    radioCopyPaste->setEnabled(enabled);
    radioAbsDrop->setEnabled(enabled);
    radioAbsolute->setEnabled(enabled);
    radioAverage->setEnabled(enabled);
    radioDropper->setEnabled(enabled);
    radioRelative->setEnabled(enabled);
    editHeight->setEnabled(enabled);
}

void HeightmapEditorPanel::BlockAllSignals(bool block)
{
    sliderWidgetBrushSize->blockSignals(block);
    sliderWidgetStrength->blockSignals(block);
    sliderWidgetAverageStrength->blockSignals(block);
    comboBrushImage->blockSignals(block);
    radioCopyPaste->blockSignals(block);
    radioAbsDrop->blockSignals(block);
    radioAbsolute->blockSignals(block);
    radioAverage->blockSignals(block);
    radioDropper->blockSignals(block);
    radioRelative->blockSignals(block);
    editHeight->blockSignals(block);
}

void HeightmapEditorPanel::InitUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    sliderWidgetBrushSize = new SliderWidget(this);
    sliderWidgetStrength = new SliderWidget(this);
    sliderWidgetAverageStrength = new SliderWidget(this);
    comboBrushImage = new QComboBox(this);
    radioCopyPaste = new QRadioButton(this);
    radioAbsDrop = new QRadioButton(this);
    radioAbsolute = new QRadioButton(this);
    radioAverage = new QRadioButton(this);
    radioDropper = new QRadioButton(this);
    radioRelative = new QRadioButton(this);
    editHeight = new QDoubleSpinBox(this);

    QHBoxLayout* layoutBrushImage = new QHBoxLayout();
    QLabel* labelBrushImageDesc = new QLabel(this);
    layoutBrushImage->addWidget(labelBrushImageDesc);
    layoutBrushImage->addWidget(comboBrushImage);

    QHBoxLayout* layoutBrushSize = new QHBoxLayout();
    QLabel* labelBrushSize = new QLabel();
    labelBrushSize->setText(ResourceEditor::HEIGHTMAP_EDITOR_BRUSH_SIZE_CAPTION.c_str());
    layoutBrushSize->addWidget(labelBrushSize);
    layoutBrushSize->addWidget(sliderWidgetBrushSize);

    QHBoxLayout* layoutStrength = new QHBoxLayout();
    QLabel* labelStrength = new QLabel();
    labelStrength->setText(ResourceEditor::HEIGHTMAP_EDITOR_STRENGTH_CAPTION.c_str());
    layoutStrength->addWidget(labelStrength);
    layoutStrength->addWidget(sliderWidgetStrength);

    QHBoxLayout* layoutAvgStrength = new QHBoxLayout();
    QLabel* labelAvgStrength = new QLabel();
    labelAvgStrength->setText(ResourceEditor::HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_CAPTION.c_str());
    layoutAvgStrength->addWidget(labelAvgStrength);
    layoutAvgStrength->addWidget(sliderWidgetAverageStrength);

    QGridLayout* layoutDrawTypes = new QGridLayout();
    layoutDrawTypes->addWidget(radioAbsolute, 0, 0);
    layoutDrawTypes->addWidget(radioRelative, 0, 1);
    layoutDrawTypes->addWidget(radioAverage, 1, 0);
    layoutDrawTypes->addWidget(radioAbsDrop, 1, 1);
    layoutDrawTypes->addWidget(radioDropper, 2, 0);
    layoutDrawTypes->addWidget(radioCopyPaste, 2, 1);

    QHBoxLayout* layoutHeight = new QHBoxLayout();
    QLabel* labelHeightDesc = new QLabel(this);
    QSpacerItem* spacerHeight = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Maximum);
    editHeight->setMinimum(0);
    editHeight->setMaximum(9999);
    editHeight->installEventFilter(this);

    layoutHeight->addWidget(labelHeightDesc);
    layoutHeight->addWidget(editHeight);
    layoutHeight->addSpacerItem(spacerHeight);

    QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->addLayout(layoutBrushSize);
    layout->addLayout(layoutBrushImage);
    layout->addLayout(layoutStrength);
    layout->addLayout(layoutAvgStrength);
    layout->addLayout(layoutDrawTypes);
    layout->addLayout(layoutHeight);
    layout->addSpacerItem(spacer);

    setLayout(layout);

    SetWidgetsState(false);
    BlockAllSignals(true);

    sliderWidgetBrushSize->Init(false, DEF_BRUSH_MAX_SIZE, DEF_BRUSH_MIN_SIZE, DEF_BRUSH_MIN_SIZE);
    sliderWidgetBrushSize->SetRangeBoundaries(ResourceEditor::BRUSH_MIN_BOUNDARY, ResourceEditor::BRUSH_MAX_BOUNDARY);
    sliderWidgetStrength->Init(true, DEF_STRENGTH_MAX_VALUE, 0, 0);
    sliderWidgetStrength->SetRangeBoundaries(STRENGTH_MIN_BOUNDARY, STRENGTH_MAX_BOUNDARY);
    sliderWidgetAverageStrength->Init(false, DEF_AVERAGE_STRENGTH_MAX_VALUE,
                                      DEF_AVERAGE_STRENGTH_MIN_VALUE, DEF_AVERAGE_STRENGTH_MIN_VALUE);
    sliderWidgetAverageStrength->SetRangeBoundaries(AVG_STRENGTH_MIN_BOUNDARY, AVG_STRENGTH_MAX_BOUNDARY);

    layoutBrushImage->setContentsMargins(0, 0, 0, 0);
    layoutDrawTypes->setContentsMargins(0, 0, 0, 0);
    layoutHeight->setContentsMargins(0, 0, 0, 0);

    labelBrushImageDesc->setText(ResourceEditor::HEIGHTMAP_EDITOR_LABEL_BRUSH_IMAGE.c_str());
    labelBrushImageDesc->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    labelHeightDesc->setText(ResourceEditor::HEIGHTMAP_EDITOR_LABEL_DROPPER_HEIGHT.c_str());
    labelHeightDesc->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    radioCopyPaste->setText(ResourceEditor::HEIGHTMAP_EDITOR_RADIO_COPY_PASTE.c_str());
    radioAbsDrop->setText(ResourceEditor::HEIGHTMAP_EDITOR_RADIO_ABS_DROP.c_str());
    radioAbsolute->setText(ResourceEditor::HEIGHTMAP_EDITOR_RADIO_ABSOLUTE.c_str());
    radioAverage->setText(ResourceEditor::HEIGHTMAP_EDITOR_RADIO_AVERAGE.c_str());
    radioDropper->setText(ResourceEditor::HEIGHTMAP_EDITOR_RADIO_DROPPER.c_str());
    radioRelative->setText(ResourceEditor::HEIGHTMAP_EDITOR_RADIO_RELATIVE.c_str());

    InitBrushImages();
}

void HeightmapEditorPanel::ConnectToSignals()
{
    connect(SceneSignals::Instance(), SIGNAL(DropperHeightChanged(SceneEditor2*, double)),
            this, SLOT(SetDropperHeight(SceneEditor2*, double)));
    connect(SceneSignals::Instance(), SIGNAL(LandscapeEditorToggled(SceneEditor2*)),
            this, SLOT(EditorToggled(SceneEditor2*)));

    connect(sliderWidgetBrushSize, SIGNAL(ValueChanged(int)), this, SLOT(SetBrushSize(int)));
    connect(sliderWidgetStrength, SIGNAL(ValueChanged(int)), this, SLOT(SetStrength(int)));
    connect(sliderWidgetAverageStrength, SIGNAL(ValueChanged(int)), this, SLOT(SetAverageStrength(int)));
    connect(radioAbsolute, SIGNAL(clicked()), this, SLOT(SetAbsoluteDrawing()));
    connect(radioRelative, SIGNAL(clicked()), this, SLOT(SetRelativeDrawing()));
    connect(radioAverage, SIGNAL(clicked()), this, SLOT(SetAverageDrawing()));
    connect(radioAbsDrop, SIGNAL(clicked()), this, SLOT(SetAbsDropDrawing()));
    connect(radioDropper, SIGNAL(clicked()), this, SLOT(SetDropper()));
    connect(radioCopyPaste, SIGNAL(clicked()), this, SLOT(SetHeightmapCopyPaste()));
    connect(comboBrushImage, SIGNAL(currentIndexChanged(int)), this, SLOT(SetToolImage(int)));
    connect(editHeight, SIGNAL(editingFinished()), this, SLOT(HeightUpdatedManually()));
}

void HeightmapEditorPanel::StoreState()
{
    DAVA::KeyedArchive* customProperties = GetOrCreateCustomProperties(GetActiveScene())->GetArchive();

    customProperties->SetInt32(ResourceEditor::HEIGHTMAP_EDITOR_BRUSH_SIZE_MIN,
                               sliderWidgetBrushSize->GetRangeMin());
    customProperties->SetInt32(ResourceEditor::HEIGHTMAP_EDITOR_BRUSH_SIZE_MAX,
                               sliderWidgetBrushSize->GetRangeMax());
    customProperties->SetInt32(ResourceEditor::HEIGHTMAP_EDITOR_STRENGTH_MAX,
                               sliderWidgetStrength->GetRangeMax());
    customProperties->SetInt32(ResourceEditor::HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_MIN,
                               sliderWidgetAverageStrength->GetRangeMin());
    customProperties->SetInt32(ResourceEditor::HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_MAX,
                               sliderWidgetAverageStrength->GetRangeMax());
}

void HeightmapEditorPanel::RestoreState()
{
    SceneEditor2* sceneEditor = GetActiveScene();

    bool enabled = sceneEditor->heightmapEditorSystem->IsLandscapeEditingEnabled();
    DAVA::int32 brushSize = BrushSizeSystemToUI(sceneEditor->heightmapEditorSystem->GetBrushSize());
    DAVA::int32 strength = StrengthSystemToUI(sceneEditor->heightmapEditorSystem->GetStrength());
    DAVA::int32 averageStrength = AverageStrengthSystemToUI(sceneEditor->heightmapEditorSystem->GetAverageStrength());
    DAVA::int32 toolImage = sceneEditor->heightmapEditorSystem->GetToolImageIndex();
    HeightmapEditorSystem::eHeightmapDrawType drawingType = sceneEditor->heightmapEditorSystem->GetDrawingType();
    DAVA::float32 height = sceneEditor->heightmapEditorSystem->GetDropperHeight();

    DAVA::int32 brushRangeMin = DEF_BRUSH_MIN_SIZE;
    DAVA::int32 brushRangeMax = DEF_BRUSH_MAX_SIZE;
    DAVA::int32 strRangeMax = DEF_STRENGTH_MAX_VALUE;
    DAVA::int32 avStrRangeMin = DEF_AVERAGE_STRENGTH_MIN_VALUE;
    DAVA::int32 avStrRangeMax = DEF_AVERAGE_STRENGTH_MAX_VALUE;

    DAVA::KeyedArchive* customProperties = GetCustomPropertiesArchieve(sceneEditor);
    if (customProperties)
    {
        brushRangeMin = customProperties->GetInt32(ResourceEditor::HEIGHTMAP_EDITOR_BRUSH_SIZE_MIN,
                                                   DEF_BRUSH_MIN_SIZE);
        brushRangeMax = customProperties->GetInt32(ResourceEditor::HEIGHTMAP_EDITOR_BRUSH_SIZE_MAX,
                                                   DEF_BRUSH_MAX_SIZE);
        strRangeMax = customProperties->GetInt32(ResourceEditor::HEIGHTMAP_EDITOR_STRENGTH_MAX,
                                                 DEF_STRENGTH_MAX_VALUE);
        avStrRangeMin = customProperties->GetInt32(ResourceEditor::HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_MIN,
                                                   DEF_AVERAGE_STRENGTH_MIN_VALUE);
        avStrRangeMax = customProperties->GetInt32(ResourceEditor::HEIGHTMAP_EDITOR_AVERAGE_STRENGTH_MAX,
                                                   DEF_AVERAGE_STRENGTH_MAX_VALUE);
    }

    SetWidgetsState(enabled);

    BlockAllSignals(true);
    sliderWidgetBrushSize->SetRangeMin(brushRangeMin);
    sliderWidgetBrushSize->SetRangeMax(brushRangeMax);
    sliderWidgetBrushSize->SetValue(brushSize);
    sliderWidgetStrength->SetRangeMax(strRangeMax);
    sliderWidgetStrength->SetValue(strength);
    sliderWidgetAverageStrength->SetRangeMin(avStrRangeMin);
    sliderWidgetAverageStrength->SetRangeMax(avStrRangeMax);
    sliderWidgetAverageStrength->SetValue(averageStrength);
    comboBrushImage->setCurrentIndex(toolImage);
    editHeight->setValue(height);
    if (NULL != sceneEditor && NULL != sceneEditor->landscapeEditorDrawSystem)
    {
        editHeight->setMaximum(sceneEditor->landscapeEditorDrawSystem->GetLandscapeMaxHeight());
    }
    UpdateRadioState(drawingType);
    BlockAllSignals(!enabled);
}

void HeightmapEditorPanel::InitBrushImages()
{
    comboBrushImage->clear();

    QSize iconSize = comboBrushImage->iconSize();
    iconSize = iconSize.expandedTo(QSize(32, 32));
    comboBrushImage->setIconSize(iconSize);

    DAVA::FilePath toolsPath(ResourceEditor::HEIGHTMAP_EDITOR_TOOLS_PATH);

    DAVA::ScopedPtr<DAVA::FileList> fileList(new DAVA::FileList(toolsPath));
    for (DAVA::uint32 iFile = 0; iFile < fileList->GetCount(); ++iFile)
    {
        auto pathname = fileList->GetPathname(iFile);
        if (DAVA::TextureDescriptor::IsSourceTextureExtension(pathname.GetExtension()))
        {
            QIcon toolIcon(QPixmap::fromImage(ImageTools::FromDavaImage(pathname)));

            auto fullname = pathname.GetAbsolutePathname();
            comboBrushImage->addItem(toolIcon, pathname.GetBasename().c_str(), QVariant(QString::fromStdString(fullname)));
        }
    }
}

// these functions are designed to convert values from sliders in ui
// to the values suitable for heightmap editor system
DAVA::int32 HeightmapEditorPanel::BrushSizeUIToSystem(DAVA::int32 uiValue)
{
    // height map size is differ from the landscape texture size.
    // so to unify brush size necessary to additionally scale brush size by (texture size / height map size) coefficient
    DAVA::float32 coef = ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF / GetBrushScaleCoef();
    DAVA::int32 systemValue = static_cast<DAVA::int32>(uiValue * coef);

    return systemValue;
}

DAVA::int32 HeightmapEditorPanel::BrushSizeSystemToUI(DAVA::int32 systemValue)
{
    DAVA::float32 coef = ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF / GetBrushScaleCoef();
    DAVA::int32 uiValue = static_cast<DAVA::int32>(systemValue / coef);

    return uiValue;
}

DAVA::float32 HeightmapEditorPanel::StrengthUIToSystem(DAVA::int32 uiValue)
{
    return static_cast<DAVA::float32>(uiValue);
}

DAVA::int32 HeightmapEditorPanel::StrengthSystemToUI(DAVA::float32 systemValue)
{
    return static_cast<DAVA::int32>(systemValue);
}

DAVA::float32 HeightmapEditorPanel::AverageStrengthUIToSystem(DAVA::int32 uiValue)
{
    DAVA::float32 systemValue = static_cast<DAVA::float32>(uiValue) / DEF_AVERAGE_STRENGTH_MAX_VALUE;
    return systemValue;
}

DAVA::int32 HeightmapEditorPanel::AverageStrengthSystemToUI(DAVA::float32 systemValue)
{
    DAVA::int32 uiValue = static_cast<DAVA::int32>(systemValue * DEF_AVERAGE_STRENGTH_MAX_VALUE);
    return uiValue;
}
// end of convert functions ==========================

DAVA::float32 HeightmapEditorPanel::GetBrushScaleCoef()
{
    SceneEditor2* sceneEditor = GetActiveScene();

    HeightmapProxy* heightmapProxy = sceneEditor->landscapeEditorDrawSystem->GetHeightmapProxy();
    if (!heightmapProxy)
    {
        return ResourceEditor::HEIGHTMAP_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
    }

    DAVA::float32 heightmapSize = heightmapProxy->Size();
    DAVA::float32 textureSize = sceneEditor->landscapeEditorDrawSystem->GetTextureSize(DAVA::Landscape::TEXTURE_COLOR);

    return textureSize / heightmapSize;
}

void HeightmapEditorPanel::UpdateRadioState(HeightmapEditorSystem::eHeightmapDrawType type)
{
    radioAbsolute->setChecked(false);
    radioRelative->setChecked(false);
    radioAverage->setChecked(false);
    radioAbsDrop->setChecked(false);
    radioDropper->setChecked(false);
    radioCopyPaste->setChecked(false);

    switch (type)
    {
    case HeightmapEditorSystem::HEIGHTMAP_DRAW_ABSOLUTE:
        radioAbsolute->setChecked(true);
        break;

    case HeightmapEditorSystem::HEIGHTMAP_DRAW_RELATIVE:
        radioRelative->setChecked(true);
        break;

    case HeightmapEditorSystem::HEIGHTMAP_DRAW_AVERAGE:
        radioAverage->setChecked(true);
        break;

    case HeightmapEditorSystem::HEIGHTMAP_DRAW_ABSOLUTE_DROPPER:
        radioAbsDrop->setChecked(true);
        break;

    case HeightmapEditorSystem::HEIGHTMAP_DROPPER:
        radioDropper->setChecked(true);
        break;

    case HeightmapEditorSystem::HEIGHTMAP_COPY_PASTE:
        radioCopyPaste->setChecked(true);
        break;

    default:
        break;
    }
}

void HeightmapEditorPanel::SetBrushSize(int brushSize)
{
    GetActiveScene()->heightmapEditorSystem->SetBrushSize(BrushSizeUIToSystem(brushSize));
}

void HeightmapEditorPanel::SetToolImage(int toolImage)
{
    QString s = comboBrushImage->itemData(toolImage).toString();

    if (!s.isEmpty())
    {
        DAVA::FilePath fp(s.toStdString());
        GetActiveScene()->heightmapEditorSystem->SetToolImage(fp, toolImage);
    }
}

void HeightmapEditorPanel::SetRelativeDrawing()
{
    SetDrawingType(HeightmapEditorSystem::HEIGHTMAP_DRAW_RELATIVE);
}

void HeightmapEditorPanel::SetAverageDrawing()
{
    SetDrawingType(HeightmapEditorSystem::HEIGHTMAP_DRAW_AVERAGE);
}

void HeightmapEditorPanel::SetAbsoluteDrawing()
{
    SetDrawingType(HeightmapEditorSystem::HEIGHTMAP_DRAW_ABSOLUTE);
}

void HeightmapEditorPanel::SetAbsDropDrawing()
{
    SetDrawingType(HeightmapEditorSystem::HEIGHTMAP_DRAW_ABSOLUTE_DROPPER);
}

void HeightmapEditorPanel::SetDropper()
{
    SetDrawingType(HeightmapEditorSystem::HEIGHTMAP_DROPPER);
}

void HeightmapEditorPanel::SetHeightmapCopyPaste()
{
    SetDrawingType(HeightmapEditorSystem::HEIGHTMAP_COPY_PASTE);
}

void HeightmapEditorPanel::SetStrength(int strength)
{
    GetActiveScene()->heightmapEditorSystem->SetStrength(StrengthUIToSystem(strength));
}

void HeightmapEditorPanel::SetAverageStrength(int averageStrength)
{
    GetActiveScene()->heightmapEditorSystem->SetAverageStrength(AverageStrengthUIToSystem(averageStrength));
}

void HeightmapEditorPanel::SetDrawingType(HeightmapEditorSystem::eHeightmapDrawType type)
{
    BlockAllSignals(true);
    UpdateRadioState(type);
    BlockAllSignals(false);

    GetActiveScene()->heightmapEditorSystem->SetDrawingType(type);
}

void HeightmapEditorPanel::SetDropperHeight(SceneEditor2* scene, double height)
{
    if (scene == GetActiveScene())
    {
        editHeight->setValue(height);
    }
}

void HeightmapEditorPanel::HeightUpdatedManually()
{
    SceneEditor2* sceneEditor = GetActiveScene();
    sceneEditor->heightmapEditorSystem->SetDropperHeight(editHeight->value());
}

void HeightmapEditorPanel::OnEditorEnabled()
{
    SetToolImage(comboBrushImage->currentIndex());
}

void HeightmapEditorPanel::ConnectToShortcuts()
{
    LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();

    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL), SIGNAL(activated()),
            this, SLOT(IncreaseBrushSize()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL), SIGNAL(activated()),
            this, SLOT(DecreaseBrushSize()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE), SIGNAL(activated()),
            this, SLOT(IncreaseBrushSizeLarge()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE), SIGNAL(activated()),
            this, SLOT(DecreaseBrushSizeLarge()));

    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
            this, SLOT(IncreaseStrength()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
            this, SLOT(DecreaseStrength()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
            this, SLOT(IncreaseStrengthLarge()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
            this, SLOT(DecreaseStrengthLarge()));

    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
            this, SLOT(IncreaseAvgStrength()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
            this, SLOT(DecreaseAvgStrength()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
            this, SLOT(IncreaseAvgStrengthLarge()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
            this, SLOT(DecreaseAvgStrengthLarge()));

    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT), SIGNAL(activated()),
            this, SLOT(NextTool()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV), SIGNAL(activated()),
            this, SLOT(PrevTool()));

    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_COPY_PASTE), SIGNAL(activated()),
            this, SLOT(SetHeightmapCopyPaste()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_ABSOLUTE), SIGNAL(activated()),
            this, SLOT(SetAbsoluteDrawing()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_RELATIVE), SIGNAL(activated()),
            this, SLOT(SetRelativeDrawing()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_AVERAGE), SIGNAL(activated()),
            this, SLOT(SetAverageDrawing()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_ABS_DROP), SIGNAL(activated()),
            this, SLOT(SetAbsDropDrawing()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_DROPPER), SIGNAL(activated()),
            this, SLOT(SetDropper()));

    shortcutManager->SetHeightMapEditorShortcutsEnabled(true);

    shortcutManager->SetBrushSizeShortcutsEnabled(true);
    shortcutManager->SetStrengthShortcutsEnabled(true);
    shortcutManager->SetAvgStrengthShortcutsEnabled(true);
    shortcutManager->SetBrushImageSwitchingShortcutsEnabled(true);
}

void HeightmapEditorPanel::DisconnectFromShortcuts()
{
    LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();

    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_SMALL), SIGNAL(activated()),
               this, SLOT(IncreaseBrushSize()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_SMALL), SIGNAL(activated()),
               this, SLOT(DecreaseBrushSize()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_INCREASE_LARGE), SIGNAL(activated()),
               this, SLOT(IncreaseBrushSizeLarge()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_SIZE_DECREASE_LARGE), SIGNAL(activated()),
               this, SLOT(DecreaseBrushSizeLarge()));

    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
               this, SLOT(IncreaseStrength()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
               this, SLOT(DecreaseStrength()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
               this, SLOT(IncreaseStrengthLarge()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
               this, SLOT(DecreaseStrengthLarge()));

    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_SMALL), SIGNAL(activated()),
               this, SLOT(IncreaseAvgStrength()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_SMALL), SIGNAL(activated()),
               this, SLOT(DecreaseAvgStrength()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_INCREASE_LARGE), SIGNAL(activated()),
               this, SLOT(IncreaseAvgStrengthLarge()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_AVG_STRENGTH_DECREASE_LARGE), SIGNAL(activated()),
               this, SLOT(DecreaseAvgStrengthLarge()));

    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_NEXT), SIGNAL(activated()),
               this, SLOT(NextTool()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_BRUSH_IMAGE_PREV), SIGNAL(activated()),
               this, SLOT(PrevTool()));

    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_COPY_PASTE), SIGNAL(activated()),
               this, SLOT(SetHeightmapCopyPaste()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_ABSOLUTE), SIGNAL(activated()),
               this, SLOT(SetAbsoluteDrawing()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_RELATIVE), SIGNAL(activated()),
               this, SLOT(SetRelativeDrawing()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_AVERAGE), SIGNAL(activated()),
               this, SLOT(SetAverageDrawing()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_ABS_DROP), SIGNAL(activated()),
               this, SLOT(SetAbsDropDrawing()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_SET_DROPPER), SIGNAL(activated()),
               this, SLOT(SetDropper()));

    shortcutManager->SetHeightMapEditorShortcutsEnabled(false);
    shortcutManager->SetBrushSizeShortcutsEnabled(false);
    shortcutManager->SetStrengthShortcutsEnabled(false);
    shortcutManager->SetAvgStrengthShortcutsEnabled(false);
    shortcutManager->SetBrushImageSwitchingShortcutsEnabled(false);
}

bool HeightmapEditorPanel::eventFilter(QObject* o, QEvent* e)
{
    if (o == editHeight && e->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Plus || keyEvent->key() == Qt::Key_Minus)
        {
            e->ignore();
            return true;
        }
    }
    return LandscapeEditorBasePanel::eventFilter(o, e);
}

void HeightmapEditorPanel::IncreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void HeightmapEditorPanel::DecreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void HeightmapEditorPanel::IncreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void HeightmapEditorPanel::DecreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void HeightmapEditorPanel::IncreaseStrength()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void HeightmapEditorPanel::DecreaseStrength()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void HeightmapEditorPanel::IncreaseStrengthLarge()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void HeightmapEditorPanel::DecreaseStrengthLarge()
{
    sliderWidgetStrength->SetValue(sliderWidgetStrength->GetValue()
                                   - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void HeightmapEditorPanel::IncreaseAvgStrength()
{
    sliderWidgetAverageStrength->SetValue(sliderWidgetAverageStrength->GetValue()
                                          + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void HeightmapEditorPanel::DecreaseAvgStrength()
{
    sliderWidgetAverageStrength->SetValue(sliderWidgetAverageStrength->GetValue()
                                          - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void HeightmapEditorPanel::IncreaseAvgStrengthLarge()
{
    sliderWidgetAverageStrength->SetValue(sliderWidgetAverageStrength->GetValue()
                                          + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void HeightmapEditorPanel::DecreaseAvgStrengthLarge()
{
    sliderWidgetAverageStrength->SetValue(sliderWidgetAverageStrength->GetValue()
                                          - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void HeightmapEditorPanel::PrevTool()
{
    DAVA::int32 curIndex = comboBrushImage->currentIndex();
    if (curIndex)
    {
        comboBrushImage->setCurrentIndex(curIndex - 1);
    }
}

void HeightmapEditorPanel::NextTool()
{
    DAVA::int32 curIndex = comboBrushImage->currentIndex();
    if (curIndex < comboBrushImage->count() - 1)
    {
        comboBrushImage->setCurrentIndex(curIndex + 1);
    }
}
