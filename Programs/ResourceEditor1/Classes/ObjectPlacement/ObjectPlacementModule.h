#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtConnections.h>

#include <Reflection/Reflection.h>

class ObjectPlacementModule : public DAVA::TArc::ClientModule
{
protected:
    void OnContextCreated(DAVA::TArc::DataContext* context) override;
    void OnContextDeleted(DAVA::TArc::DataContext* context) override;

    void PostInit() override;

private:
    void OnPlaceOnLandscape();
    void OnSnapToLandscape();
    void OnPlaceAndAlign();

    DAVA::TArc::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(ObjectPlacementModule, DAVA::TArc::ClientModule);
};
