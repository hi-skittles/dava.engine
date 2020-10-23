#pragma once

#include <TextureCompression/TextureConverter.h>
#include <Render/RenderBase.h>

#include <QDialog>

class FilePathBrowser;
class QCheckBox;
class QComboBox;

class ExportSceneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportSceneDialog(QWidget* parent = 0);
    ~ExportSceneDialog() override;

    DAVA::FilePath GetDataFolder() const;
    DAVA::Vector<DAVA::eGPUFamily> GetGPUs() const;
    DAVA::TextureConverter::eConvertQuality GetQuality() const;
    bool GetOptimizeOnExport() const;
    bool GetUseHDTextures() const;
    DAVA::String GetFilenamesTag() const;

private slots:
    void SetExportEnabled();

private:
    void SetupUI();
    void InitializeValues();

    FilePathBrowser* projectPathBrowser = nullptr;

    DAVA::Array<QCheckBox*, DAVA::eGPUFamily::GPU_FAMILY_COUNT> gpuSelector;
    QComboBox* qualitySelector = nullptr;
    QComboBox* tagSelector = nullptr;

    QCheckBox* optimizeOnExport = nullptr;
    QCheckBox* useHDtextures = nullptr;

    QPushButton* exportButton = nullptr;
};
