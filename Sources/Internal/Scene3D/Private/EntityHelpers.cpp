#include "Scene3D/Private/EntityHelpers.h"

namespace DAVA
{
String VisibleValueDescription(const Any& value)
{
    return value.Cast<bool>() == true ? "Visible" : "Invisible";
}
}