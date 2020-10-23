#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>
#include <Base/ScopedPtr.h>

namespace DAVA
{
class FilePath;
}

class UserNodeModule : public DAVA::ClientModule
{
public:
    static DAVA::FilePath GetBotSpawnPath();
    UserNodeModule();

protected:
    void PostInit() override;
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;

private:
    void ChangeDrawingState();

    void OnHUDVisibilityChanged(const DAVA::Any& hudVisibilityValue);

    std::unique_ptr<DAVA::FieldBinder> fieldBinder;

    DAVA::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(UserNodeModule, DAVA::ClientModule);
};
