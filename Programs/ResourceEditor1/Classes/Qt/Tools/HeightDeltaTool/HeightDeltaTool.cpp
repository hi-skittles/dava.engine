#include "Classes/Qt/Tools/HeightDeltaTool/HeightDeltaTool.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Qt/Main/mainwindow.h"
#include "Classes/Qt/Tools/HeightDeltaTool/PaintHeightDelta.h"
#include "Classes/Qt/Tools/PathDescriptor/PathDescriptor.h"

#include "ui_HeightDeltaTool.h"

#include <QtTools/FileDialogs/FileDialog.h>

#include <Render/Image/ImageSystem.h>
#include <Render/Image/ImageFormatInterface.h>

#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>

namespace HeightDeltaToolDetails
{
SceneEditor2* GetActiveScene()
{
    SceneData* data = REGlobal::GetActiveDataNode<SceneData>();
    if (data != nullptr)
    {
        return data->GetScene().Get();
    }
    return nullptr;
}
}

HeightDeltaTool::HeightDeltaTool(QWidget* p)
    : QWidget(p)
    , ui(new Ui::HeightDeltaTool())
{
    ui->setupUi(this);

    connect(ui->cancel, &QAbstractButton::clicked, this, &HeightDeltaTool::close);
    connect(ui->run, &QAbstractButton::clicked, this, &HeightDeltaTool::OnRun);
    connect(ui->angle, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &HeightDeltaTool::OnValueChanged);

    OnValueChanged();
}

HeightDeltaTool::~HeightDeltaTool() = default;

double HeightDeltaTool::GetThresholdInMeters(double unitSize)
{
    double angle = ui->angle->value();
    DAVA::float32 radAngle = DAVA::DegToRad((DAVA::float32)angle);

    double tangens = tan(radAngle);

    double delta = DAVA::Abs(unitSize * (DAVA::float32)tangens);
    return delta;
}

void HeightDeltaTool::OnRun()
{
    DVASSERT(!outputFilePath.isEmpty());

    SceneEditor2* scene = HeightDeltaToolDetails::GetActiveScene();
    DVASSERT(scene);
    DAVA::Landscape* landscapeRO = FindLandscape(scene);
    DVASSERT(landscapeRO);

    const DAVA::AABBox3& bbox = landscapeRO->GetBoundingBox();
    DAVA::Heightmap* heightmap = landscapeRO->GetHeightmap();
    DVASSERT(heightmap);

    DAVA::int32 heightmapSize = heightmap->Size();
    const double unitSize = (bbox.max.x - bbox.min.x) / heightmapSize;

    const double threshold = GetThresholdInMeters(unitSize);

    GeneralSettings* settings = REGlobal::GetGlobalContext()->GetData<GeneralSettings>();

    DAVA::Vector<DAVA::Color> colors;
    colors.resize(2);
    colors[0] = settings->heightMaskColor0;
    colors[1] = settings->heightMaskColor1;

    PaintHeightDelta::Execute(outputFilePath.toStdString(), (DAVA::float32)threshold, heightmap,
                              heightmapSize, heightmapSize, bbox.max.z - bbox.min.z, colors);
    QMessageBox::information(this, "Mask is ready", outputFilePath);

    QWidget::close();
}

void HeightDeltaTool::OnValueChanged(double /*v*/)
{
    ui->run->setEnabled(false);

    SceneEditor2* scene = HeightDeltaToolDetails::GetActiveScene();
    DVASSERT(scene != nullptr);
    DAVA::Landscape* landscape = FindLandscape(scene);
    if (landscape == nullptr)
    {
        ui->outputPath->setText(tr("Landscape not found"));
        return;
    }

    if (landscape->GetHeightmap() == nullptr)
    {
        ui->outputPath->setText(tr("Heightmap was not assigned"));
        return;
    }

    ui->run->setEnabled(true);

    DAVA::FilePath heightMapPath = landscape->GetHeightmapPathname();
    heightMapPath.ReplaceExtension("");

    QString angle = QString::number(ui->angle->value()).replace('.', '-').replace(',', '-');

    outputFilePath = QString("%1_delta_%2.png").arg(heightMapPath.GetAbsolutePathname().c_str()).arg(angle);
    ui->outputPath->setText(QFileInfo(outputFilePath).fileName());
}
