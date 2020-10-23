#pragma once

#include "TArc/Core/ClientModule.h"
#include "TArc/Core/FieldBinder.h"
#include "TArc/Utils/QtConnections.h"

#include "Base/Any.h"
#include "Reflection/Reflection.h"

#include <memory>

class ShadersModule : public DAVA::TArc::ClientModule
{
protected:
    void PostInit() override;

private:
    void ReloadShaders();
    void OnProjectChanged(const DAVA::Any& projectFieldValue);

    std::unique_ptr<DAVA::TArc::FieldBinder> fieldBinder;
    DAVA::TArc::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(ShadersModule, DAVA::TArc::ClientModule);
};
