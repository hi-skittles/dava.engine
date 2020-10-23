#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>

class TextModule : public DAVA::ClientModule
{
public:
    TextModule();

protected:
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;

    void PostInit() override;

private:
    void ChangeDrawingState();

    DAVA::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(TextModule, DAVA::ClientModule);
};
