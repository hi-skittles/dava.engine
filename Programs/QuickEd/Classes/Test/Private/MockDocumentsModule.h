#pragma once

#include <TArc/Core/ControllerModule.h>
#include <TArc/Core/OperationRegistrator.h>

namespace TestHelpers
{
class MockDocumentsModule : public DAVA::ControllerModule
{
    void PostInit() override;
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;

    void OnRenderSystemInitialized(DAVA::Window* w) override;
    bool CanWindowBeClosedSilently(const DAVA::WindowKey& key, DAVA::String& requestWindowText) override;
    bool SaveOnWindowClose(const DAVA::WindowKey& key) override;
    void RestoreOnWindowClose(const DAVA::WindowKey& key) override;

    void CloseAllDocuments();
    void CreateDummyContext();

    DAVA_VIRTUAL_REFLECTION(MockDocumentsModule, DAVA::ControllerModule);
};

DECLARE_OPERATION_ID(CreateDummyContextOperation);

class MockData : public DAVA::TArcDataNode
{
public:
    bool canClose = true;

    DAVA_VIRTUAL_REFLECTION(MockData, DAVA::TArcDataNode);
};
} //namespace TestHelpers
