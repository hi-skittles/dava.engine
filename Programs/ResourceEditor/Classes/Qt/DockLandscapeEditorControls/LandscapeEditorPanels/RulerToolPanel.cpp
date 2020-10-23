#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorPanels/RulerToolPanel.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorShortcutManager.h"

#include "Classes/Qt/Scene/SceneSignals.h"
#include "Classes/Qt/Tools/SliderWidget/SliderWidget.h"

#include <REPlatform/Global/Constants.h>
#include <REPlatform/Global/StringConstants.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/RulerToolSystem.h>

#include <QFormLayout>
#include <QLabel>
#include <QLayout>

RulerToolPanel::RulerToolPanel(QWidget* parent)
    : LandscapeEditorBasePanel(parent)
    , labelLength(nullptr)
    , labelPreview(nullptr)
{
    InitUI();
    ConnectToSignals();
}

RulerToolPanel::~RulerToolPanel()
{
}

bool RulerToolPanel::GetEditorEnabled()
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();
    if (!sceneEditor)
    {
        return false;
    }

    return sceneEditor->GetSystem<DAVA::RulerToolSystem>()->IsLandscapeEditingEnabled();
}

void RulerToolPanel::SetWidgetsState(bool enabled)
{
    labelLength->setEnabled(enabled);
    labelPreview->setEnabled(enabled);
}

void RulerToolPanel::BlockAllSignals(bool block)
{
}

void RulerToolPanel::InitUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    labelLength = new QLabel(this);
    labelPreview = new QLabel(this);

    QLabel* labelLengthDesc = new QLabel(this);
    QLabel* labelPreviewDesc = new QLabel(this);
    QFrame* frame = new QFrame(this);
    QFormLayout* frameLayout = new QFormLayout(frame);
    QSpacerItem* spacer = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);

    frameLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    frameLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frame->setLayout(frameLayout);
    frameLayout->addRow(labelLengthDesc, labelLength);
    frameLayout->addRow(labelPreviewDesc, labelPreview);

    layout->addWidget(frame);
    layout->addSpacerItem(spacer);

    setLayout(layout);

    SetWidgetsState(false);
    BlockAllSignals(true);

    labelLength->setNum(0);
    labelPreview->setNum(0);
    labelLengthDesc->setText(DAVA::ResourceEditor::RULER_TOOL_LENGTH_CAPTION.c_str());
    labelPreviewDesc->setText(DAVA::ResourceEditor::RULER_TOOL_PREVIEW_LENGTH_CAPTION.c_str());
}

void RulerToolPanel::ConnectToSignals()
{
    connect(SceneSignals::Instance(), &SceneSignals::LandscapeEditorToggled, this, &RulerToolPanel::EditorToggled);

    connect(SceneSignals::Instance(), SIGNAL(RulerToolLengthChanged(DAVA::SceneEditor2*, double, double)),
            this, SLOT(UpdateLengths(DAVA::SceneEditor2*, double, double)));
}

void RulerToolPanel::StoreState()
{
}

void RulerToolPanel::RestoreState()
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();
    DAVA::RulerToolSystem* system = sceneEditor->GetSystem<DAVA::RulerToolSystem>();

    bool enabled = system->IsLandscapeEditingEnabled();

    SetWidgetsState(enabled);

    BlockAllSignals(true);
    UpdateLengths(sceneEditor, system->GetLength(), system->GetPreviewLength());
    BlockAllSignals(!enabled);
}

void RulerToolPanel::UpdateLengths(DAVA::SceneEditor2* scene, double length, double previewLength)
{
    DAVA::SceneEditor2* sceneEditor = GetActiveScene();
    if (scene != sceneEditor)
    {
        return;
    }

    if (length < 0.0)
    {
        length = 0.0;
    }
    if (previewLength < 0.0)
    {
        previewLength = 0.0;
    }

    labelLength->setText(QString::number(length));
    labelPreview->setText(QString::number(previewLength));
}

void RulerToolPanel::ConnectToShortcuts()
{
    LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();
    shortcutManager->SetBrushSizeShortcutsEnabled(true);
}

void RulerToolPanel::DisconnectFromShortcuts()
{
    LandscapeEditorShortcutManager* shortcutManager = LandscapeEditorShortcutManager::Instance();
    shortcutManager->SetBrushSizeShortcutsEnabled(false);
}
