#ifndef __CONTROL_HELPERS_H__
#define __CONTROL_HELPERS_H__

#include "DAVAEngine.h"

namespace DAVA
{
namespace ControlHelpers
{
struct ReportItem
{
    static const String TEST_NAME_PATH;
    static const String MIN_DELTA_PATH;
    static const String MAX_DELTA_PATH;
    static const String AVERAGE_DELTA_PATH;
    static const String TEST_TIME_PATH;
    static const String ELAPSED_TIME_PATH;
    static const String FRAMES_RENDERED_PATH;
};

FilePath GetPathToUIYaml(const String& yamlFileName);
};
};

#endif