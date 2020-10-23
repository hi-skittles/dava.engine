#include "MitsubaExporterTools.h"

namespace mitsuba
{
const DAVA::String kBeginTag = "<";
const DAVA::String kBeginCloseTag = "</";
const DAVA::String kEndTag = ">";
const DAVA::String kSelfEndTag = "/>";
const DAVA::String kSpace = " ";
const DAVA::String kEqual = "=";
const DAVA::String kQuote = "\"";
const DAVA::String kIdent = "    ";
const DAVA::String kName = "name";
const DAVA::String kValue = "value";
const DAVA::String kType = "type";
const DAVA::String kInteger = "integer";
const DAVA::String kString = "string";
const DAVA::String kFloat = "float";
const DAVA::String kId = "id";
const DAVA::String kBumpmap = "bumpmap";
const DAVA::String kDiffuse = "diffuse";
const DAVA::String kDielectric = "dielectric";
const DAVA::String kRoughDielectric = "roughdielectric";
const DAVA::String kRoughConductor = "roughconductor";

std::ostream* currentOutput = nullptr;
DAVA::uint32 outputIdentation = 0;
}
