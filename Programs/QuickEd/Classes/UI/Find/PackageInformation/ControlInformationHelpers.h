#pragma once

#include <Base/BaseTypes.h>

class ControlInformation;

namespace ControlInformationHelpers
{
DAVA::String GetPathToControl(const ControlInformation* provider);
};
