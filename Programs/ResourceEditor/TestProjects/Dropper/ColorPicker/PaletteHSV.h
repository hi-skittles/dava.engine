#ifndef PALETTEHSV_H
#define PALETTEHSV_H

#include "../Widgets/AbstractSlider.h"

class PaletteHSV
: public AbstractSlider
{
    Q_OBJECT

public:
    explicit PaletteHSV(QWidget* parent);
    ~PaletteHSV();

    int GetHue() const;
    int GetSat() const;

    void SetColor(int hue, int sat);
    void SetColor(const QColor& c);

protected:
    void DrawBackground(QPainter* p) const override;
    void DrawForeground(QPainter* p) const override;
    QRect PosArea() const override;

    void resizeEvent(QResizeEvent* e) override;

private:
    void DrawCursor(QPainter* p) const;

    QSize cursorSize;
    mutable QPixmap bgCache;
};

#endif // PALETTEHSV_H
