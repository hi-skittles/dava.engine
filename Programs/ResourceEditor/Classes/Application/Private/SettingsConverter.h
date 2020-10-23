#pragma once

namespace DAVA
{
class PropertiesHolder;
class ContextAccessor;
} // namespace DAVA

void ConvertSettingsIfNeeded(const DAVA::PropertiesHolder& rootNode, DAVA::ContextAccessor* accessor);
