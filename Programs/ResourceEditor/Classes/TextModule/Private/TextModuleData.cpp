#include "Classes/TextModule/Private/TextModuleData.h"
#include "Classes/TextModule/Private/EditorTextSystem.h"

const char* TextModuleData::drawingEnabledPropertyName = "drawingEnabledPropertyName";

void TextModuleData::SetDrawingEnabled(bool enabled)
{
    DVASSERT(editorTextSystem);
    if (enabled)
    {
        editorTextSystem->EnableSystem();
    }
    else
    {
        editorTextSystem->DisableSystem();
    }
}

bool TextModuleData::IsDrawingEnabled() const
{
    DVASSERT(editorTextSystem);
    return editorTextSystem->IsSystemEnabled();
}
