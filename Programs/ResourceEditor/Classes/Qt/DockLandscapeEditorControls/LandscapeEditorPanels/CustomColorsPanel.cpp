#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorPanels/CustomColorsPanel.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorShortcutManager.h"
#include "Classes/Qt/Scene/SceneSignals.h"
#include "Classes/Qt/Tools/PathDescriptor/PathDescriptor.h"
#include "Classes/Qt/Tools/SliderWidget/SliderWidget.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <REPlatform/Global/Constants.h>
#include <REPlatform/Global/StringConstants.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/CustomColorsSystem.h>

#include <TArc/Core/Deprecated.h>

#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSpacerItem>

CustomColorsPanel::CustomColorsPanel(QWidget* parent)
    : LandscapeEditorBasePanel(parent)
{
    InitUI();
    ConnectToSignals();
    InitColors();
}

bool CustomColorsPanel::GetEditorEnabled()
{
    return GetActiveScene()->GetSystem<DAVA::CustomColorsSystem>()->IsLandscapeEditingEnabled();
}

void CustomColorsPanel::SetWidgetsState(bool enabled)
{
    comboColor->setEnabled(enabled);
    sliderWidgetBrushSize->setEnabled(enabled);
    buttonSaveTexture->setEnabled(enabled);
    buttonLoadTexture->setEnabled(enabled);
}

void CustomColorsPanel::BlockAllSignals(bool block)
{
    comboColor->blockSignals(block);
    sliderWidgetBrushSize->blockSignals(block);
    buttonSaveTexture->blockSignals(block);
    buttonLoadTexture->blockSignals(block);
}

void CustomColorsPanel::StoreState()
{
    DAVA::KeyedArchive* customProperties = GetOrCreateCustomProperties(GetActiveScene())->GetArchive();
    customProperties->SetInt32(DAVA::ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MIN, sliderWidgetBrushSize->GetRangeMin());
    customProperties->SetInt32(DAVA::ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MAX, sliderWidgetBrushSize->GetRangeMax());
}

void CustomColorsPanel::RestoreState()
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();

    DAVA::CustomColorsSystem* customColorSystem = sceneEditor->GetSystem<DAVA::CustomColorsSystem>();
    bool enabled = customColorSystem->IsLandscapeEditingEnabled();
    DAVA::int32 brushSize = BrushSizeSystemToUI(customColorSystem->GetBrushSize());
    DAVA::int32 colorIndex = customColorSystem->GetColor();

    DAVA::int32 brushRangeMin = DEF_BRUSH_MIN_SIZE;
    DAVA::int32 brushRangeMax = DEF_BRUSH_MAX_SIZE;

    DAVA::KeyedArchive* customProperties = GetCustomPropertiesArchieve(sceneEditor);
    if (customProperties)
    {
        brushRangeMin = customProperties->GetInt32(DAVA::ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MIN, DEF_BRUSH_MIN_SIZE);
        brushRangeMax = customProperties->GetInt32(DAVA::ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MAX, DEF_BRUSH_MAX_SIZE);
    }

    SetWidgetsState(enabled);

    BlockAllSignals(true);
    sliderWidgetBrushSize->SetRangeMin(brushRangeMin);
    sliderWidgetBrushSize->SetRangeMax(brushRangeMax);
    sliderWidgetBrushSize->SetValue(brushSize);
    comboColor->setCurrentIndex(colorIndex);
    BlockAllSignals(!enabled);
}

void CustomColorsPanel::InitUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    comboColor = new QComboBox(this);
    sliderWidgetBrushSize = new SliderWidget(this);
    buttonSaveTexture = new QPushButton(this);
    buttonLoadTexture = new QPushButton(this);
    QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout* layoutBrushSize = new QHBoxLayout();
    QLabel* labelBrushSize = new QLabel();
    labelBrushSize->setText(DAVA::ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_CAPTION.c_str());
    layoutBrushSize->addWidget(labelBrushSize);
    layoutBrushSize->addWidget(sliderWidgetBrushSize);

    layout->addWidget(comboColor);
    layout->addLayout(layoutBrushSize);
    layout->addWidget(buttonLoadTexture);
    layout->addWidget(buttonSaveTexture);
    layout->addSpacerItem(spacer);

    setLayout(layout);

    SetWidgetsState(false);
    BlockAllSignals(true);

    sliderWidgetBrushSize->Init(false, DEF_BRUSH_MAX_SIZE, DEF_BRUSH_MIN_SIZE, DEF_BRUSH_MIN_SIZE);
    sliderWidgetBrushSize->SetRangeBoundaries(DAVA::ResourceEditor::BRUSH_MIN_BOUNDARY, DAVA::ResourceEditor::BRUSH_MAX_BOUNDARY);
    buttonSaveTexture->setText(DAVA::ResourceEditor::CUSTOM_COLORS_SAVE_CAPTION.c_str());
    buttonLoadTexture->setText(DAVA::ResourceEditor::CUSTOM_COLORS_LOAD_CAPTION.c_str());
}

void CustomColorsPanel::ConnectToSignals()
{
    projectDataWrapper = DAVA::Deprecated::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<DAVA::ProjectManagerData>());
    projectDataWrapper.SetListener(this);

    connect(SceneSignals::Instance(), &SceneSignals::LandscapeEditorToggled, this, &CustomColorsPanel::EditorToggled);

    connect(sliderWidgetBrushSize, SIGNAL(ValueChanged(int)), this, SLOT(SetBrushSize(int)));
    connect(comboColor, SIGNAL(currentIndexChanged(int)), this, SLOT(SetColor(int)));
    connect(buttonSaveTexture, SIGNAL(clicked()), this, SLOT(SaveTexture()));
    connect(buttonLoadTexture, SIGNAL(clicked()), this, SLOT(LoadTexture()));
}

void CustomColorsPanel::InitColors()
{
    comboColor->clear();

    QSize iconSize = comboColor->iconSize();
    iconSize = iconSize.expandedTo(QSize(100, 0));
    comboColor->setIconSize(iconSize);

    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data != nullptr);

    const DAVA::EditorConfig* config = data->GetEditorConfig();

    DAVA::Vector<DAVA::Color> customColors = config->GetColorPropertyValues(DAVA::ResourceEditor::CUSTOM_COLORS_PROPERTY_COLORS);
    DAVA::Vector<DAVA::String> customColorsDescription = config->GetComboPropertyValues(DAVA::ResourceEditor::CUSTOM_COLORS_PROPERTY_DESCRIPTION);
    for (size_t i = 0; i < customColors.size(); ++i)
    {
        QColor color = QColor::fromRgbF(customColors[i].r, customColors[i].g, customColors[i].b, customColors[i].a);

        QImage image(iconSize, QImage::Format_ARGB32);
        image.fill(color);

        QPixmap pixmap(iconSize);
        pixmap.convertFromImage(image, Qt::ColorOnly);

        QIcon icon(pixmap);
        DAVA::String description = (i >= customColorsDescription.size()) ? "" : customColorsDescription[i];
        comboColor->addItem(icon, description.c_str());
    }
}

// these functions are designed to convert values from sliders in ui
// to the values suitable for custom colors system
DAVA::int32 CustomColorsPanel::BrushSizeUIToSystem(DAVA::int32 uiValue)
{
    DAVA::int32 systemValue = uiValue * DAVA::ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
    return systemValue;
}

DAVA::int32 CustomColorsPanel::BrushSizeSystemToUI(DAVA::int32 systemValue)
{
    DAVA::int32 uiValue = systemValue / DAVA::ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
    return uiValue;
}
// end of convert functions ==========================

void CustomColorsPanel::OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data);
    InitColors();
}

void CustomColorsPanel::SetBrushSize(int brushSize)
{
    GetActiveScene()->GetSystem<DAVA::CustomColorsSystem>()->SetBrushSize(BrushSizeUIToSystem(brushSize));
}

void CustomColorsPanel::SetColor(int color)
{
    GetActiveScene()->GetSystem<DAVA::CustomColorsSystem>()->SetColor(color);
}

bool CustomColorsPanel::SaveTexture()
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();

    DAVA::FilePath selectedPathname = sceneEditor->GetSystem<DAVA::CustomColorsSystem>()->GetCurrentSaveFileName();
    DVASSERT(!selectedPathname.IsEmpty());
    selectedPathname = selectedPathname.GetDirectory();

    const QString text = "Custom colors texture was not saved. Do you want to save it?";
    QString filePath;
    for (;;)
    {
        DAVA::FileDialogParams fileDlgParams;
        fileDlgParams.dir = QString(selectedPathname.GetAbsolutePathname().c_str());
        fileDlgParams.title = QString(DAVA::ResourceEditor::CUSTOM_COLORS_SAVE_CAPTION.c_str());
        fileDlgParams.filters = PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter;
        filePath = DAVA::Deprecated::GetUI()->GetSaveFileName(DAVA::mainWindowKey, fileDlgParams);

        if (filePath.isEmpty())
        {
            DAVA::ModalMessageParams questParams;
            questParams.title = "Save changes";
            questParams.icon = DAVA::ModalMessageParams::Question;
            questParams.message = text;
            questParams.buttons = DAVA::ModalMessageParams::Yes | DAVA::ModalMessageParams::No;
            questParams.defaultButton = DAVA::ModalMessageParams::NoButton;

            if (DAVA::Deprecated::GetUI()->ShowModalMessage(DAVA::mainWindowKey, questParams) == DAVA::ModalMessageParams::Yes)
            {
                continue;
            }
        }
        break;
    }

    selectedPathname = DAVA::FilePath(filePath.toStdString());

    if (selectedPathname.IsEmpty())
    {
        return false;
    }

    sceneEditor->GetSystem<DAVA::CustomColorsSystem>()->SaveTexture(selectedPathname);
    return true;
}

void CustomColorsPanel::LoadTexture()
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();

    DAVA::FilePath currentPath = sceneEditor->GetSystem<DAVA::CustomColorsSystem>()->GetCurrentSaveFileName();

    if (!DAVA::FileSystem::Instance()->Exists(currentPath))
    {
        currentPath = sceneEditor->GetScenePath().GetDirectory();
    }

    DAVA::FileDialogParams fileDlgParams;
    fileDlgParams.dir = QString(currentPath.GetAbsolutePathname().c_str());
    fileDlgParams.title = QString(DAVA::ResourceEditor::CUSTOM_COLORS_LOAD_CAPTION.c_str());
    fileDlgParams.filters = PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter;
    QString filePath = DAVA::Deprecated::GetUI()->GetOpenFileName(DAVA::mainWindowKey, fileDlgParams);

    DAVA::FilePath selectedPathname(filePath.toStdString());
    if (!selectedPathname.IsEmpty())
    {
        sceneEditor->GetSystem<DAVA::CustomColorsSystem>()->LoadTexture(selectedPathname, true);
    }
}

void CustomColorsPanel::ConnectToShortcuts()
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

    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
            this, SLOT(NextTexture()));
    connect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
            this, SLOT(PrevTexture()));

    shortcutManager->SetBrushSizeShortcutsEnabled(true);
    shortcutManager->SetTextureSwitchingShortcutsEnabled(true);
}

void CustomColorsPanel::DisconnectFromShortcuts()
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

    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
               this, SLOT(NextTexture()));
    disconnect(shortcutManager->GetShortcutByName(DAVA::ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
               this, SLOT(PrevTexture()));

    shortcutManager->SetBrushSizeShortcutsEnabled(false);
    shortcutManager->SetTextureSwitchingShortcutsEnabled(false);
}

void CustomColorsPanel::IncreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void CustomColorsPanel::DecreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void CustomColorsPanel::IncreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void CustomColorsPanel::DecreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - DAVA::ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void CustomColorsPanel::PrevTexture()
{
    DAVA::int32 curIndex = comboColor->currentIndex();
    if (curIndex)
    {
        comboColor->setCurrentIndex(curIndex - 1);
    }
}

void CustomColorsPanel::NextTexture()
{
    DAVA::int32 curIndex = comboColor->currentIndex();
    if (curIndex < comboColor->count() - 1)
    {
        comboColor->setCurrentIndex(curIndex + 1);
    }
}
