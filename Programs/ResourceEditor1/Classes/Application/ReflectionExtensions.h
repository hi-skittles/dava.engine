#pragma once

#include <TArc/Core/ClientModule.h>

class ReflectionExtensionsModule : public DAVA::TArc::ClientModule
{
public:
    ReflectionExtensionsModule() = default;

    void PostInit() override;

private:
    DAVA_VIRTUAL_REFLECTION(ReflectionExtensionsModule, DAVA::TArc::ClientModule);
};
