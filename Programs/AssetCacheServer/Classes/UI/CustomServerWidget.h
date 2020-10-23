#pragma once

#include <QWidget>
#include "ApplicationSettings.h"
#include "ui_CustomServerWidget.h"

class CustomServerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CustomServerWidget(QWidget* parent = nullptr);
    explicit CustomServerWidget(const RemoteServerParams& newServer, QWidget* parent = nullptr);

    RemoteServerParams GetServerData() const;

    bool IsChecked() const;
    void SetChecked(bool checked);

signals:
    void ServerChecked(bool checked);
    void ParametersChanged();
    void RemoveLater();

private slots:
    void OnChecked(int val);

private:
    std::unique_ptr<Ui::CustomServerWidget> ui;
};
