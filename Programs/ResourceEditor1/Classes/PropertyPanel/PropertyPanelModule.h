#pragma once

#include "Classes/Interfaces/PropertyPanelInterface.h"

#include <TArc/Core/ClientModule.h>

#include <Reflection/Reflection.h>
#include <Base/BaseTypes.h>

class PropertyPanelModule final : public DAVA::TArc::ClientModule, private Interfaces::PropertyPanelInterface
{
public:
    void PostInit() override;

protected:
    void RegisterExtension(const std::shared_ptr<DAVA::TArc::ExtensionChain>& extension) override;
    void UnregisterExtension(const std::shared_ptr<DAVA::TArc::ExtensionChain>& extension) override;

private:
    DAVA_VIRTUAL_REFLECTION(PropertyPanelModule, DAVA::TArc::ClientModule);
};