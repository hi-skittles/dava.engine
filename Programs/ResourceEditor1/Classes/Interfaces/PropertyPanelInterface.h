#pragma once

#include <TArc/Controls/PropertyPanel/PropertyModelExtensions.h>

namespace Interfaces
{
class PropertyPanelInterface
{
public:
    virtual void RegisterExtension(const std::shared_ptr<DAVA::TArc::ExtensionChain>& extension) = 0;
    virtual void UnregisterExtension(const std::shared_ptr<DAVA::TArc::ExtensionChain>& extension) = 0;
};
}