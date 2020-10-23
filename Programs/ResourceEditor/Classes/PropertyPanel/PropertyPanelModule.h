#pragma once

#include <REPlatform/Global/PropertyPanelInterface.h>

#include <TArc/Core/ClientModule.h>

#include <Reflection/Reflection.h>
#include <Base/BaseTypes.h>

class PropertyPanelModule final : public DAVA::ClientModule, private DAVA::PropertyPanelInterface
{
public:
    void PostInit() override;

protected:
    void RegisterExtension(const std::shared_ptr<DAVA::ExtensionChain>& extension) override;
    void UnregisterExtension(const std::shared_ptr<DAVA::ExtensionChain>& extension) override;

private:
    DAVA_VIRTUAL_REFLECTION(PropertyPanelModule, DAVA::ClientModule);
};