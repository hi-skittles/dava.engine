#pragma once

namespace DAVA
{
namespace TArc
{
class PropertiesHolder;
class ContextAccessor;
} // namespace TArc
} // namespace DAVA

void ConvertSettingsIfNeeded(const DAVA::TArc::PropertiesHolder& rootNode, DAVA::TArc::ContextAccessor* accessor);
