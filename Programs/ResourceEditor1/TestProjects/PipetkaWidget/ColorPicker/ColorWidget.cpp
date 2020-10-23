#include "ColorWidget.h"
#include "ui_ColorWidget.h"

#include "IColorEditor.h"
#include "HSVPaletteWidget.h"

ColorWidget::ColorWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ColorWidget())
{
    ui->setupUi(this);

    // TEST
    {
        ui->test_01->SetColorRange(QColor(255, 0, 0), QColor(0, 0, 0, 0));
        ui->test_01->SetRenderDimensions(true, false);
    }
    {
        const QColor c1 = QColor(0, 255, 0);
        const QColor c2 = QColor(0, 0, 255);
        ui->test_02->SetColorRange(c1, c2);
        ui->test_02->SetRenderDimensions(false, true);
        ui->test_02->SetBgPadding(5, 0, 5, 0);
        ui->test_02->setEditorDimensions(Qt::LeftEdge | Qt::RightEdge /* | Qt::TopEdge | Qt::BottomEdge*/);
        ui->test_02->setPrefferableArrows();

        connect(ui->test_02, SIGNAL(changing(const QColor&)), SLOT(onSliderColor(const QColor&)));
    }

    AddPalette("HSV", new HSVPaletteWidget());

    connect(ui->paletteCombo, SIGNAL(currentIndexChanged(int)), SLOT(onPaletteType()));
}

ColorWidget::~ColorWidget()
{
}

void ColorWidget::AddPalette(QString const& name, IColorEditor* _pal)
{
    QWidget* pal = dynamic_cast<QWidget*>(_pal);
    Q_ASSERT(pal);

    ui->paletteCombo->addItem(name, name);
    ui->paletteStack->addWidget(pal);
    paletteMap[name] = pal;
}

void ColorWidget::onPaletteType()
{
    const int idx = ui->paletteCombo->currentIndex();
    //if ( idx < 0 )
    //    return;

    const QString& key = ui->paletteCombo->itemData(idx).toString();
    ui->paletteStack->setCurrentWidget(paletteMap[key]);
}

void ColorWidget::onSliderColor(QColor const& c)
{
    const QSize picSize(ui->cmid->size());
    QPixmap pix(picSize);
    pix.fill(c);
    ui->cmid->setPixmap(pix);
}
