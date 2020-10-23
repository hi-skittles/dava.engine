#pragma once

#include <QWidget>
#include "ApplicationSettings.h"
#include "ui_SharedServerWidget.h"

class SharedServerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SharedServerWidget(QWidget* parent = nullptr);
    explicit SharedServerWidget(const SharedServer& newServer, QWidget* parent = nullptr);

    void Update(const SharedServer& updatedData);

    ServerID GetServerID() const;
    PoolID GetPoolID() const;

    bool IsChecked() const;
    void SetChecked(bool checked);

signals:
    void ServerChecked(bool checked);
    void RemoveLater();

private slots:
    void OnChecked(int val);

private:
    PoolID poolID = NullPoolID;
    ServerID serverID = NullServerID;
    std::unique_ptr<Ui::SharedServerWidget> ui;
};

inline PoolID SharedServerWidget::GetPoolID() const
{
    return poolID;
}

inline ServerID SharedServerWidget::GetServerID() const
{
    return serverID;
}
