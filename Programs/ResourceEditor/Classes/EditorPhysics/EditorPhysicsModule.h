#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>

class EditorPhysicsModule : public DAVA::ClientModule
{
public:
    EditorPhysicsModule();

protected:
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;

    void PostInit() override;

    void OnInterfaceRegistered(const DAVA::Type* interfaceType) override;
    void OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType) override;

private:
    DAVA::QtConnections connections;
    std::unique_ptr<DAVA::FieldBinder> binder;
    DAVA_VIRTUAL_REFLECTION(EditorPhysicsModule, DAVA::ClientModule);
};