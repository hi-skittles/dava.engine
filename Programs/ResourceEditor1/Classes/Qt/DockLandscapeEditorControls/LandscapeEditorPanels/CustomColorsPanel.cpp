#include "CustomColorsPanel.h"
#include "Scene/SceneSignals.h"
#include "Scene/SceneEditor2.h"
#include "Tools/SliderWidget/SliderWidget.h"
#include "Deprecated/EditorConfig.h"
#include "Constants.h"
#include "Main/QtUtils.h"
#include "Qt/DockLandscapeEditorControls/LandscapeEditorShortcutManager.h"
#include "QtTools/FileDialogs/FileDialog.h"
#include "Tools/PathDescriptor/PathDescriptor.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"

#include <QLayout>
#include <QComboBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QLabel>

CustomColorsPanel::CustomColorsPanel(QWidget* parent)
    : LandscapeEditorBasePanel(parent)
{
    InitUI();
    ConnectToSignals();
    InitColors();
}

bool CustomColorsPanel::GetEditorEnabled()
{
    return GetActiveScene()->customColorsSystem->IsLandscapeEditingEnabled();
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
    customProperties->SetInt32(ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MIN, sliderWidgetBrushSize->GetRangeMin());
    customProperties->SetInt32(ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MAX, sliderWidgetBrushSize->GetRangeMax());
}

void CustomColorsPanel::RestoreState()
{
    SceneEditor2* sceneEditor = GetActiveScene();

    bool enabled = sceneEditor->customColorsSystem->IsLandscapeEditingEnabled();
    DAVA::int32 brushSize = BrushSizeSystemToUI(sceneEditor->customColorsSystem->GetBrushSize());
    DAVA::int32 colorIndex = sceneEditor->customColorsSystem->GetColor();

    DAVA::int32 brushRangeMin = DEF_BRUSH_MIN_SIZE;
    DAVA::int32 brushRangeMax = DEF_BRUSH_MAX_SIZE;

    DAVA::KeyedArchive* customProperties = GetCustomPropertiesArchieve(sceneEditor);
    if (customProperties)
    {
        brushRangeMin = customProperties->GetInt32(ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MIN, DEF_BRUSH_MIN_SIZE);
        brushRangeMax = customProperties->GetInt32(ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_MAX, DEF_BRUSH_MAX_SIZE);
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
    labelBrushSize->setText(ResourceEditor::CUSTOM_COLORS_BRUSH_SIZE_CAPTION.c_str());
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
    sliderWidgetBrushSize->SetRangeBoundaries(ResourceEditor::BRUSH_MIN_BOUNDARY, ResourceEditor::BRUSH_MAX_BOUNDARY);
    buttonSaveTexture->setText(ResourceEditor::CUSTOM_COLORS_SAVE_CAPTION.c_str());
    buttonLoadTexture->setText(ResourceEditor::CUSTOM_COLORS_LOAD_CAPTION.c_str());
}

void CustomColorsPanel::ConnectToSignals()
{
    projectDataWrapper = REGlobal::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<ProjectManagerData>());
    projectDataWrapper.SetListener(this);

    connect(SceneSignals::Instance(), SIGNAL(LandscapeEditorToggled(SceneEditor2*)),
            this, SLOT(EditorToggled(SceneEditor2*)));

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

    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);

    const EditorConfig* config = data->GetEditorConfig();

    DAVA::Vector<DAVA::Color> customColors = config->GetColorPropertyValues(ResourceEditor::CUSTOM_COLORS_PROPERTY_COLORS);
    DAVA::Vector<DAVA::String> customColorsDescription = config->GetComboPropertyValues(ResourceEditor::CUSTOM_COLORS_PROPERTY_DESCRIPTION);
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
    DAVA::int32 systemValue = uiValue * ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
    return systemValue;
}

DAVA::int32 CustomColorsPanel::BrushSizeSystemToUI(DAVA::int32 systemValue)
{
    DAVA::int32 uiValue = systemValue / ResourceEditor::LANDSCAPE_BRUSH_SIZE_UI_TO_SYSTEM_COEF;
    return uiValue;
}
// end of convert functions ==========================

void CustomColorsPanel::OnDataChanged(const DAVA::TArc::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data);
    InitColors();
}

void CustomColorsPanel::SetBrushSize(int brushSize)
{
    GetActiveScene()->customColorsSystem->SetBrushSize(BrushSizeUIToSystem(brushSize));
}

void CustomColorsPanel::SetColor(int color)
{
    GetActiveScene()->customColorsSystem->SetColor(color);
}

bool CustomColorsPanel::SaveTexture()
{
    SceneEditor2* sceneEditor = GetActiveScene();

    DAVA::FilePath selectedPathname = sceneEditor->customColorsSystem->GetCurrentSaveFileName();
    DVASSERT(!selectedPathname.IsEmpty());
    selectedPathname = selectedPathname.GetDirectory();

    const QString text = "Custom colors texture was not saved. Do you want to save it?";
    QString filePath;
    for (;;)
    {
        filePath = FileDialog::getSaveFileName(nullptr, QString(ResourceEditor::CUSTOM_COLORS_SAVE_CAPTION.c_str()),
                                               QString(selectedPathname.GetAbsolutePathname().c_str()),
                                               PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter);

        if (filePath.isEmpty())
        {
            QMessageBox::StandardButton result = QMessageBox::question(NULL, "Save changes", text);
            if (result == QMessageBox::Yes)
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

    sceneEditor->customColorsSystem->SaveTexture(selectedPathname);
    return true;
}

void CustomColorsPanel::LoadTexture()
{
    SceneEditor2* sceneEditor = GetActiveScene();

    DAVA::FilePath currentPath = sceneEditor->customColorsSystem->GetCurrentSaveFileName();

    if (!DAVA::FileSystem::Instance()->Exists(currentPath))
    {
        currentPath = sceneEditor->GetScenePath().GetDirectory();
    }

    DAVA::FilePath selectedPathname = GetOpenFileName(ResourceEditor::CUSTOM_COLORS_LOAD_CAPTION, currentPath,
                                                      PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter.toStdString());
    if (!selectedPathname.IsEmpty())
    {
        sceneEditor->customColorsSystem->LoadTexture(selectedPathname, true);
    }
}

void CustomColorsPanel::ConnectToShortcuts()
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

    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
            this, SLOT(NextTexture()));
    connect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
            this, SLOT(PrevTexture()));

    shortcutManager->SetBrushSizeShortcutsEnabled(true);
    shortcutManager->SetTextureSwitchingShortcutsEnabled(true);
}

void CustomColorsPanel::DisconnectFromShortcuts()
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

    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_NEXT), SIGNAL(activated()),
               this, SLOT(NextTexture()));
    disconnect(shortcutManager->GetShortcutByName(ResourceEditor::SHORTCUT_TEXTURE_PREV), SIGNAL(activated()),
               this, SLOT(PrevTexture()));

    shortcutManager->SetBrushSizeShortcutsEnabled(false);
    shortcutManager->SetTextureSwitchingShortcutsEnabled(false);
}

void CustomColorsPanel::IncreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void CustomColorsPanel::DecreaseBrushSize()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_SMALL);
}

void CustomColorsPanel::IncreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    + ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
}

void CustomColorsPanel::DecreaseBrushSizeLarge()
{
    sliderWidgetBrushSize->SetValue(sliderWidgetBrushSize->GetValue()
                                    - ResourceEditor::SLIDER_WIDGET_CHANGE_VALUE_STEP_LARGE);
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
