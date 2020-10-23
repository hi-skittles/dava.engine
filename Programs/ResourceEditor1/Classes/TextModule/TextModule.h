#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>

class TextModule : public DAVA::TArc::ClientModule
{
public:
    TextModule();

protected:
    void OnContextCreated(DAVA::TArc::DataContext* context) override;
    void OnContextDeleted(DAVA::TArc::DataContext* context) override;

    void PostInit() override;

private:
    void ChangeDrawingState();

    DAVA::TArc::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(TextModule, DAVA::TArc::ClientModule);
};
