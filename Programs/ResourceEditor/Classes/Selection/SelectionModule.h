#pragma once

#include "TArc/Core/ClientModule.h"
#include "TArc/Utils/QtConnections.h"

#include "Reflection/Reflection.h"

class SelectionModule : public DAVA::ClientModule
{
protected:
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;

    void PostInit() override;

private:
    void CreateModuleActions(DAVA::UI* ui);
    void SelectionByMouseChanged();

    DAVA::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(SelectionModule, DAVA::ClientModule);
};
