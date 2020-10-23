#include "Classes/Application/REGlobal.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Deprecated/EditorConfig.h"
#include <Classes/Qt/Tools/ExportSceneDialog/ExportSceneDialog.h>
#include "Classes/Qt/Tools/Widgets/FilePathBrowser.h"

#include <TArc/DataProcessing/DataContext.h>

#include <Base/GlobalEnum.h>
#include <Base/BaseTypes.h>
#include <Debug/DVAssert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

ExportSceneDialog::ExportSceneDialog(QWidget* parent)
    : QDialog(parent)
{
    SetupUI();
    InitializeValues();
}

ExportSceneDialog::~ExportSceneDialog() = default;

void ExportSceneDialog::SetupUI()
{
    setModal(true);

    static const int UI_WIDTH = 160;
    static const int UI_HEIGHT = 20;

    setWindowTitle("Export scene");

    QGridLayout* dialogLayout = new QGridLayout();
    dialogLayout->setColumnStretch(0, 1);
    dialogLayout->setColumnStretch(1, 1);

    DVASSERT(projectPathBrowser == nullptr); // to prevent several calls of this functions
    projectPathBrowser = new FilePathBrowser(this);
    projectPathBrowser->SetType(FilePathBrowser::Folder);
    projectPathBrowser->AllowInvalidPath(true);
    projectPathBrowser->setMinimumWidth(UI_WIDTH * 3);
    dialogLayout->addWidget(projectPathBrowser, 0, 0, 1, 0);

    { //GPU
        QVBoxLayout* gpuLayout = new QVBoxLayout();

        QLabel* gpuLabel = new QLabel(this);
        gpuLabel->setText("Select GPU:");
        gpuLabel->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        gpuLayout->addWidget(gpuLabel);

        for (DAVA::int32 gpu = DAVA::GPU_POWERVR_IOS; gpu < DAVA::GPU_FAMILY_COUNT; ++gpu)
        {
            QString gpuText = GlobalEnumMap<DAVA::eGPUFamily>::Instance()->ToString(static_cast<DAVA::eGPUFamily>(gpu));

            gpuSelector[gpu] = new QCheckBox(gpuText, this);
            gpuSelector[gpu]->setMinimumSize(UI_WIDTH, UI_HEIGHT);

            gpuLayout->addWidget(gpuSelector[gpu]);
            connect(gpuSelector[gpu], &QCheckBox::toggled, this, &ExportSceneDialog::SetExportEnabled);
        }

        gpuLayout->addStretch();

        dialogLayout->addLayout(gpuLayout, 1, 0);
    }

    { // options
        QVBoxLayout* optionsLayout = new QVBoxLayout();

        QLabel* qualityLabel = new QLabel(this);
        qualityLabel->setText("Select Quality:");
        qualityLabel->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        optionsLayout->addWidget(qualityLabel);

        qualitySelector = new QComboBox(this);
        qualitySelector->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        optionsLayout->addWidget(qualitySelector);

        const auto& qualityMap = GlobalEnumMap<DAVA::TextureConverter::eConvertQuality>::Instance();
        for (size_t i = 0; i < qualityMap->GetCount(); ++i)
        {
            int value;
            bool ok = qualityMap->GetValue(i, value);
            if (ok)
            {
                qualitySelector->addItem(qualityMap->ToString(value), value);
            }
        }

        QLabel* tagLabel = new QLabel(this);
        tagLabel->setText("Select Tag:");
        tagLabel->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        optionsLayout->addWidget(tagLabel);

        tagSelector = new QComboBox(this);
        tagSelector->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        optionsLayout->addWidget(tagSelector);

        tagSelector->addItem("No tags", "");

        ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
        DVASSERT(data != nullptr);
        const EditorConfig* editorConfig = data->GetEditorConfig();
        if (editorConfig->HasProperty("Tags"))
        {
            const DAVA::Vector<DAVA::String>& projectTags = editorConfig->GetComboPropertyValues("Tags");
            for (const DAVA::String& tag : projectTags)
            {
                QString qTag = QString::fromStdString(tag);
                tagSelector->addItem(qTag, qTag);
            }
        }

        optimizeOnExport = new QCheckBox("Optimize on export", this);
        optimizeOnExport->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        optionsLayout->addWidget(optimizeOnExport);

        useHDtextures = new QCheckBox("Use HD Textures", this);
        useHDtextures->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        optionsLayout->addWidget(useHDtextures);

        optionsLayout->addStretch();
        dialogLayout->addLayout(optionsLayout, 1, 1);
    }

    { //buttons
        QPushButton* cancelButton = new QPushButton("Cancel", this);
        cancelButton->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        cancelButton->setFixedHeight(UI_HEIGHT);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        exportButton = new QPushButton("Export", this);
        exportButton->setMinimumSize(UI_WIDTH, UI_HEIGHT);
        exportButton->setFixedHeight(UI_HEIGHT);
        connect(exportButton, &QPushButton::clicked, this, &QDialog::accept);

        dialogLayout->addWidget(exportButton, 2, 0);
        dialogLayout->addWidget(cancelButton, 2, 1);
    }

    setLayout(dialogLayout);
    setFixedHeight(sizeHint().height());
}

void ExportSceneDialog::InitializeValues()
{
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);
    DAVA::String path = data->GetProjectPath().GetAbsolutePathname();
    projectPathBrowser->SetPath(QString::fromStdString(path + "Data/3d/"));

    for (DAVA::int32 gpu = DAVA::GPU_POWERVR_IOS; gpu < DAVA::GPU_FAMILY_COUNT; ++gpu)
    {
        gpuSelector[gpu]->setCheckState(Qt::Unchecked);
    }

    GeneralSettings* settings = REGlobal::GetGlobalContext()->GetData<GeneralSettings>();
    qualitySelector->setCurrentIndex(static_cast<int>(settings->compressionQuality));
    optimizeOnExport->setCheckState(Qt::Checked);
    useHDtextures->setCheckState(Qt::Unchecked);

    SetExportEnabled();
}

void ExportSceneDialog::SetExportEnabled()
{
    bool gpuChecked = false;
    for (QCheckBox* gpuBox : gpuSelector)
    {
        if (gpuBox->isChecked())
        {
            gpuChecked = true;
            break;
        }
    }

    exportButton->setEnabled(gpuChecked);
}

DAVA::FilePath ExportSceneDialog::GetDataFolder() const
{
    DAVA::FilePath path = projectPathBrowser->GetPath().toStdString();
    path.MakeDirectoryPathname();
    return path;
}

DAVA::Vector<DAVA::eGPUFamily> ExportSceneDialog::GetGPUs() const
{
    DAVA::Vector<DAVA::eGPUFamily> gpus;

    for (DAVA::int32 gpu = DAVA::GPU_POWERVR_IOS; gpu < DAVA::GPU_FAMILY_COUNT; ++gpu)
    {
        if (gpuSelector[gpu]->checkState() == Qt::Checked)
        {
            gpus.push_back(static_cast<DAVA::eGPUFamily>(gpu));
        }
    }

    return gpus;
}

DAVA::TextureConverter::eConvertQuality ExportSceneDialog::GetQuality() const
{
    return static_cast<DAVA::TextureConverter::eConvertQuality>(qualitySelector->currentIndex());
}

bool ExportSceneDialog::GetOptimizeOnExport() const
{
    return optimizeOnExport->checkState() == Qt::Checked;
}

bool ExportSceneDialog::GetUseHDTextures() const
{
    return useHDtextures->checkState() == Qt::Checked;
}

DAVA::String ExportSceneDialog::GetFilenamesTag() const
{
    return tagSelector->currentData(Qt::UserRole).toString().toStdString();
}
