#ifndef __QT_IMAGE_SPLITTER_SIZE_DIALOG_H__
#define __QT_IMAGE_SPLITTER_SIZE_DIALOG_H__

#include <QWidget>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QLabel>

#include "DAVAEngine.h"

class SizeDialog : public QDialog
{
public:
    explicit SizeDialog(QWidget* parent = 0);

    ~SizeDialog();

    DAVA::Vector2 GetSize() const;

private:
    QVBoxLayout* verticalLayout;
    QLabel* messageLbl;
    QHBoxLayout* horLayout;
    QLabel* widthLbl;
    QSpinBox* widthSpinBox;
    QLabel* heightLbl;
    QSpinBox* heightSpinBox;
    QDialogButtonBox* buttonBox;
};

#endif /* defined(__QT_IMAGE_SPLITTER_SIZE_DIALOG_H__) */
