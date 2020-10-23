#include "Classes/DevFuncs/CrashProduceModule.h"

#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/Utils/ModuleCollection.h>

#include <Reflection/ReflectionRegistrator.h>

#include <QAction>
#include <thread>

void CrashProduceModule::PostInit()
{
    using namespace DAVA;
    UI* ui = GetUI();
    ActionPlacementInfo placementInfo(CreateMenuPoint(QList<QString>() << "DebugFunctions"
                                                                       << "Crash Produce"));

    QAction* assertAction = new QAction("Generate Assert", nullptr);
    connections.AddConnection(assertAction, &QAction::triggered, []()
                              {
                                  DVASSERT(false);
                              });

    ui->AddAction(DAVA::mainWindowKey, placementInfo, assertAction);

    QAction* sequencalsAssert = new QAction("Generate sequencal asserts", nullptr);
    connections.AddConnection(sequencalsAssert, &QAction::triggered, []()
                              {
                                  DVASSERT(false && "First assert");
                                  DVASSERT(false && "Second assert");
                              });

    ui->AddAction(DAVA::mainWindowKey, placementInfo, sequencalsAssert);

    QAction* threadAssertAction = new QAction("Generate Assert From Thread", nullptr);
    connections.AddConnection(threadAssertAction, &QAction::triggered, []()
                              {
                                  std::thread t([]() {
                                      DVASSERT(0, "Assert from foreign thread");
                                  });
                                  t.detach();
                              });
    ui->AddAction(DAVA::mainWindowKey, placementInfo, threadAssertAction);

    QAction* dumpAction = new QAction("Generate dump", nullptr);
    connections.AddConnection(dumpAction, &QAction::triggered, []()
                              {
                                  int* p = reinterpret_cast<int*>(static_cast<int>(0xDEADBEAD));
                                  int x = *p;
                                  DAVA::Logger::Info("Crash value %d", x);
                              });

    ui->AddAction(DAVA::mainWindowKey, placementInfo, dumpAction);
}

DAVA_VIRTUAL_REFLECTION_IMPL(CrashProduceModule)
{
    DAVA::ReflectionRegistrator<CrashProduceModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_TARC_MODULE(CrashProduceModule);