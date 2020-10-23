#pragma once

#include <QWidget>
#include "ApplicationSettings.h"
#include "ui_SharedPoolWidget.h"

class SharedPoolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SharedPoolWidget(QWidget* parent = nullptr);
    explicit SharedPoolWidget(const SharedPool& pool, QWidget* parent = nullptr);

    void Update(const SharedPool& pool);

    PoolID GetPoolID() const;

    bool IsChecked() const;
    void SetChecked(bool checked);

signals:
    void PoolChecked(bool checked);
    void RemoveLater();

private slots:
    void OnChecked(int val);

private:
    PoolID poolID = NullPoolID;
    std::unique_ptr<Ui::SharedPoolWidget> ui;
};

inline PoolID SharedPoolWidget::GetPoolID() const
{
    return poolID;
}
