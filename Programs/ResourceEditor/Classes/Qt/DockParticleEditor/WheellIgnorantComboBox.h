#pragma once

#include <QComboBox>

class QEvent;

class WheellIgnorantComboBox : public QComboBox
{
public:
    explicit WheellIgnorantComboBox(QWidget* parent = 0);

    bool event(QEvent* e) override;
};
