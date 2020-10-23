#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>

class ObjectPlacementModule : public DAVA::ClientModule
{
protected:
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;

    void PostInit() override;

private:
    void OnPlaceOnLandscape();
    void OnSnapToLandscape();
    void OnPlaceAndAlign();

    DAVA::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(ObjectPlacementModule, DAVA::ClientModule);
};
