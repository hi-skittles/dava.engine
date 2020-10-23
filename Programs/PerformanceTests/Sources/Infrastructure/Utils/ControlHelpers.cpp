#include "Infrastructure/Utils/ControlHelpers.h"

using namespace DAVA;

FilePath ControlHelpers::GetPathToUIYaml(const String& yamlFileName)
{
    const FilePath path = Format("~res:/UI/win/%s", yamlFileName.c_str());
    return path;
}

const String ControlHelpers::ReportItem::TEST_NAME_PATH = "TestName";
const String ControlHelpers::ReportItem::MIN_DELTA_PATH = "MinDelta/MinDeltaValue";
const String ControlHelpers::ReportItem::MAX_DELTA_PATH = "MaxDelta/MaxDeltaValue";
const String ControlHelpers::ReportItem::AVERAGE_DELTA_PATH = "AverageDelta/AverageDeltaValue";
const String ControlHelpers::ReportItem::TEST_TIME_PATH = "TestTime/TestTimeValue";
const String ControlHelpers::ReportItem::ELAPSED_TIME_PATH = "ElapsedTime/ElapsedTimeValue";
const String ControlHelpers::ReportItem::FRAMES_RENDERED_PATH = "FramesRendered/FramesRenderedValue";
