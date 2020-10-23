#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>

class EditorPhysicsModule : public DAVA::TArc::ClientModule
{
public:
    EditorPhysicsModule();

protected:
    void OnContextCreated(DAVA::TArc::DataContext* context) override;
    void OnContextDeleted(DAVA::TArc::DataContext* context) override;

    void PostInit() override;

    void OnInterfaceRegistered(const DAVA::Type* interfaceType) override;
    void OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType) override;

private:
    DAVA::TArc::QtConnections connections;
    std::unique_ptr<DAVA::TArc::FieldBinder> binder;
    DAVA_VIRTUAL_REFLECTION(EditorPhysicsModule, DAVA::TArc::ClientModule);
};