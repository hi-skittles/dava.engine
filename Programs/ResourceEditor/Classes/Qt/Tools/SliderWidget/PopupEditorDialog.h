#ifndef __RESOURCEEDITORQT__POPUPEDITORDIALOG__
#define __RESOURCEEDITORQT__POPUPEDITORDIALOG__

#include <QDialog>

class QLineEdit;

class PopupEditorDialog : public QDialog
{
    Q_OBJECT

public:
    PopupEditorDialog(int initialValue,
                      int rangeMin, int rangeMax,
                      const QWidget* widget = 0,
                      QWidget* parent = 0);
    ~PopupEditorDialog();

signals:
    void ValueReady(const QWidget* widget, int value);

protected slots:
    void OnReturnPressed();
    void OnEditingFinished();

protected:
    virtual void showEvent(QShowEvent* event);

private:
    QLineEdit* editValue;
    const QWidget* widget;
    int value;
};

#endif /* defined(__RESOURCEEDITORQT__POPUPEDITORDIALOG__) */
