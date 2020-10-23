#ifndef __RESOURCEEDITORQT__SLIDERWIDGET__
#define __RESOURCEEDITORQT__SLIDERWIDGET__

#include <QWidget>

class QSlider;
class QLineEdit;
class QSpinBox;
class QLabel;

class SliderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SliderWidget(QWidget* parent = 0);
    ~SliderWidget();

    void Init(bool symmetric, int max, int min, int value);

    void SetRange(int min, int max);

    void SetRangeMax(int max);
    int GetRangeMax();

    void SetRangeMin(int min);
    int GetRangeMin();

    void SetSymmetric(bool symmetric);
    bool IsSymmetric();

    void SetValue(int value);
    int GetValue();

    void SetRangeChangingBlocked(bool blocked);
    bool IsRangeChangingBlocked();

    void SetRangeVisible(bool visible);
    bool IsRangeVisible();

    void SetCurValueVisible(bool visible);
    bool IsCurValueVisible();

    void SetRangeBoundaries(int min, int max);

protected:
    virtual bool eventFilter(QObject* obj, QEvent* ev);

signals:
    void ValueChanged(int newValue);

private slots:
    void SliderValueChanged(int newValue);
    void RangeChanged(int newMinValue, int newMaxValue);
    void SpinValueChanged(int newValue);

    void OnValueReady(const QWidget* widget, int value);

private:
    static const int DEF_LOWEST_VALUE;
    static const int DEF_HIGHEST_VALUE;

    QLabel* labelMinValue;
    QLabel* labelMaxValue;
    QSpinBox* spinCurValue;
    QSlider* sliderValue;

    bool isSymmetric;
    bool isRangeChangingBlocked;
    bool isRangeVisible;

    int minValue;
    int maxValue;
    int currentValue;

    int rangeBoundMin;
    int rangeBoundMax;

    void EmitValueChanged();
    void ConnectToSignals();
    void UpdateControls();

    void InitUI();
};

#endif /* defined(__RESOURCEEDITORQT__SLIDERWIDGET__) */
