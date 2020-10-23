#pragma once

#include "UIScreens/BaseScreen.h"
#include <Base/Array.h>
#include <Base/BaseTypes.h>
#include <Base/RefPtr.h>
#include <Base/Token.h>
#include <FileSystem/FilePath.h>

namespace DAVA
{
class ProgramOptions;
}

class UIViewScreen : public BaseScreen
{
public:
    UIViewScreen(DAVA::Window* window, DAVA::ProgramOptions* options);

    void LoadResources() override;
    void UnloadResources() override;

private:
    void SetupUI();

    void PrintError(const DAVA::String& errorMessage);

    DAVA::Window* window = nullptr;
    DAVA::ProgramOptions* options = nullptr;

    enum PlaceholderSide : DAVA::int32
    {
        LEFT = 0,
        RIGHT,
        TOP,
        BOTTOM,
        COUNT
    };
    DAVA::Array<DAVA::RefPtr<DAVA::UIControl>, PlaceholderSide::COUNT> hiddenPlaceholders;
    DAVA::Token visibleFrameChangedToken;

    DAVA::FilePath projectPath;
};
