#pragma once

#include "Core/ConsoleTasks/ConsoleBaseTask.h"

struct ApplicationContext;
struct ConfigHolder;

class SelfTestTask : public ConsoleBaseTask
{
public:
    SelfTestTask();
    ~SelfTestTask() override;

private:
    QCommandLineOption CreateOption() const override;
    void Run(const QStringList& arguments) override;

    ApplicationContext* appContext = nullptr;
    ConfigHolder* configHolder = nullptr;
};

Q_DECLARE_METATYPE(SelfTestTask);
