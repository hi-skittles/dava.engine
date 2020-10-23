#ifndef COLORWIDGET_H
#define COLORWIDGET_H

#include <QWidget>
#include <QScopedPointer>
#include <QPointer>
#include <QMap>

namespace Ui
{
class ColorWidget;
};

class IColorEditor;

class ColorWidget
: public QWidget
{
    Q_OBJECT

private:
    typedef QMap<QString, QWidget*> PaletteMap;

public:
    explicit ColorWidget(QWidget* parent = 0);
    ~ColorWidget();

    void AddPalette(const QString& name, IColorEditor* pal);

private slots:
    void onPaletteType();
    void onSliderColor(const QColor& c);

private:
    QScopedPointer<Ui::ColorWidget> ui;
    PaletteMap paletteMap;
};


#endif // COLORWIDGET_H
