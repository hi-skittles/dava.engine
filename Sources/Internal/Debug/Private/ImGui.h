#pragma once

#include "Base/BaseTypes.h"

#include <imgui/imgui.h>

namespace DAVA
{
struct InputEvent;
}

namespace ImGui
{
static const DAVA::uint32 screenWidth = 1024;
static const DAVA::uint32 screenHeight = 768;

void Initialize();
bool IsInitialized();
void OnFrameBegin();
void OnFrameEnd();
bool OnInput(const DAVA::InputEvent& input);
void Uninitialize();

struct Settings
{
    static DAVA::float32 scale;
    static DAVA::float32 pendingScale;
};
} // namespace ImGui
