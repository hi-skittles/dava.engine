#pragma once

namespace DAVA
{
class ContextAccessor;
}

class QString;

namespace DragNDropHelper
{
bool IsFileFromProject(DAVA::ContextAccessor* accessor, const QString& path);
bool IsExtensionSupported(const QString& path);
}