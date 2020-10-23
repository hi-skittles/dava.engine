#pragma once

#include "Infrastructure/BaseScreen.h"

#include <Concurrency/Dispatcher.h>

namespace DAVA
{
class BaseObject;
class Color;
class Engine;
class Font;
class Thread;
class UIButton;
class UIStaticText;

struct Rect;
};

class TestBed;
class FileActivationTest : public BaseScreen
{
public:
    FileActivationTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;
    void Update(DAVA::float32 timeElapsed) override;

private:
    void OnFileActivated(DAVA::Vector<DAVA::String> filenames);

    void OnDumpFilenames(const DAVA::Any&);

    DAVA::String FormatFilenameList(const DAVA::Vector<DAVA::String>& filenames, size_t maxRows);

private:
    DAVA::Engine& engine;

    DAVA::UIStaticText* textStartup = nullptr;
    DAVA::UIStaticText* textFiles = nullptr;
    DAVA::UIStaticText* textLatest = nullptr;

    bool pendingUIUpdate = false;

    DAVA::Vector<DAVA::String> activationFilenames;
    DAVA::Vector<DAVA::String> latestActivationFilenames;
};
