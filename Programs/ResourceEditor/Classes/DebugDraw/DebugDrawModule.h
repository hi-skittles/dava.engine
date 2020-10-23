#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/ReflectedPairsVector.h>

#include <Reflection/Reflection.h>

using CollisionTypesMap = DAVA::ReflectedPairsVector<DAVA::int32, DAVA::String>;

class DebugDrawModule : public DAVA::ClientModule
{
public:
    DebugDrawModule();

protected:
    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;
    void OnInterfaceRegistered(const DAVA::Type* interfaceType) override;
    void OnBeforeInterfaceUnregistered(const DAVA::Type* interfaceType) override;
    void PostInit() override;

private:
    DAVA::QtConnections connections;
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;

    void OnHangingObjects();
    void OnHangingObjectsHeight(double value);
    void OnSwitchWithDifferentLODs();

    bool IsDisabled() const;
    DAVA::int32 GetCollisionType() const;
    void SetCollisionType(DAVA::int32 type);

    const CollisionTypesMap& GetCollisionTypes() const;
    void SetCollisionTypes(const CollisionTypesMap& map);

    const CollisionTypesMap& GetCollisionTypesCrashed() const;
    void SetCollisionTypesCrashed(const CollisionTypesMap& map);

    DAVA_VIRTUAL_REFLECTION(DebugDrawModule, DAVA::ClientModule);
};
