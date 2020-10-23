#include "RulerToolPanel.h"
#include "../../Scene/SceneEditor2.h"
#include "../../Scene/SceneSignals.h"
#include "../../Tools/SliderWidget/SliderWidget.h"
#include "../LandscapeEditorShortcutManager.h"
#include "Constants.h"

#include <QLayout>
#include <QFormLayout>
#include <QLabel>

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
    SceneEditor2* sceneEditor = GetActiveScene();
    if (!sceneEditor)
    {
        return false;
    }

    return sceneEditor->rulerToolSystem->IsLandscapeEditingEnabled();
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
    labelLengthDesc->setText(ResourceEditor::RULER_TOOL_LENGTH_CAPTION.c_str());
    labelPreviewDesc->setText(ResourceEditor::RULER_TOOL_PREVIEW_LENGTH_CAPTION.c_str());
}

void RulerToolPanel::ConnectToSignals()
{
    connect(SceneSignals::Instance(), SIGNAL(LandscapeEditorToggled(SceneEditor2*)),
            this, SLOT(EditorToggled(SceneEditor2*)));

    connect(SceneSignals::Instance(), SIGNAL(RulerToolLengthChanged(SceneEditor2*, double, double)),
            this, SLOT(UpdateLengths(SceneEditor2*, double, double)));
}

void RulerToolPanel::StoreState()
{
}

void RulerToolPanel::RestoreState()
{
    SceneEditor2* sceneEditor = GetActiveScene();

    bool enabled = sceneEditor->rulerToolSystem->IsLandscapeEditingEnabled();

    SetWidgetsState(enabled);

    BlockAllSignals(true);
    UpdateLengths(sceneEditor,
                  sceneEditor->rulerToolSystem->GetLength(),
                  sceneEditor->rulerToolSystem->GetPreviewLength());
    BlockAllSignals(!enabled);
}

void RulerToolPanel::UpdateLengths(SceneEditor2* scene, double length, double previewLength)
{
    SceneEditor2* sceneEditor = GetActiveScene();
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
