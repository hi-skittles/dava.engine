#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "ColladaErrorCodes.h"

eColladaErrorCodes ConvertDaeToSc2(const DAVA::FilePath& pathToFile);
eColladaErrorCodes ConvertDaeToAnimations(const DAVA::FilePath& pathToFile);
