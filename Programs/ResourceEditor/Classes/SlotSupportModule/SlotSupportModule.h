#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>

class SlotSupportModule : public DAVA::ClientModule
{
public:
    SlotSupportModule();

protected:
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;

    void OnInterfaceRegistered(const DAVA::Type* interfaceType) override;
    void OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType) override;

    void PostInit() override;

private:
    void ReloadConfig() const;
    void ParseConfig(const DAVA::Any& v) const;
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
    DAVA::QtConnections connections;
    DAVA_VIRTUAL_REFLECTION(SlotSupportModule, DAVA::ClientModule);
};
