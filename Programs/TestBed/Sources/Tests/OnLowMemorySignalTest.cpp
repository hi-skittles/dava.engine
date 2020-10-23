#include "OnLowMemorySignalTest.h"

#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <UI/Focus/UIFocusComponent.h>
#include <UI/Text/UITextComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>

OnLowMemorySignalTest::OnLowMemorySignalTest(TestBed& app)
    : BaseScreen(app, "OnLowMemorySignalTest")
{
}

void OnLowMemorySignalTest::LoadResources()
{
    using namespace DAVA;

    BaseScreen::LoadResources();

    numberOfCallbackCalls = 0;
    numberOfAllocatedMbytes = 0;

    RefPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));
    const float32 fontSize = 20.f;

    auto SetText = [&font, fontSize](UIControl* c, const String& s) {
        UITextComponent* t = c->GetOrCreateComponent<UITextComponent>();
        t->SetFont(font);
        t->SetText(s);
        t->SetFontSize(fontSize);
        t->SetColorInheritType(UIControlBackground::eColorInheritType::COLOR_IGNORE_PARENT);
        t->SetColor(Color::White);
        t->SetAlign(eAlign::ALIGN_LEFT);
    };

    UIControl* chunkSizeTxt = new UIControl({ 50.f, 50.f, 150.f, 60.f });
    SetText(chunkSizeTxt, "Chunk size: ");
    AddControl(chunkSizeTxt);

    UITextField* chunkSize = new UITextField({ 200.f, 50.f, 100.f, 60.f });
    chunkSize->SetName(FastName("chunkSize"));
    chunkSize->GetOrCreateComponent<UIDebugRenderComponent>();
    chunkSize->SetFont(font.Get());
    chunkSize->SetFontSize(fontSize);
    chunkSize->SetTextColor(Color::White);
    chunkSize->SetText(std::to_wstring(minChunkSize));
    chunkSize->GetOrCreateComponent<UIFocusComponent>();
    AddControl(chunkSize);

    UIControl* startTest = new UIControl({ 350.f, 50.f, 100.f, 60.f });
    startTest->SetName(FastName("startTest"));
    startTest->AddEvent(EVENT_TOUCH_UP_INSIDE, Message([](BaseObject*, void* x, void*) { static_cast<OnLowMemorySignalTest*>(x)->ToggleTest(); }, this));
    startTest->GetOrCreateComponent<UIDebugRenderComponent>();
    SetText(startTest, "Test");
    startTest->GetComponent<UITextComponent>()->SetAlign(eAlign::ALIGN_HCENTER);
    DAVA::UIControlBackground* bg = startTest->GetOrCreateComponent<DAVA::UIControlBackground>();
    bg->SetDrawType(DAVA::UIControlBackground::DRAW_FILL);
    AddControl(startTest);

    UIControl* callbackCount = new UIControl({ 50.f, 200.f, 300.f, 60.f });
    callbackCount->SetName("callbackCount");
    SetText(callbackCount, "0");
    AddControl(callbackCount);

    UIControl* mbytesAllocated = new UIControl({ 360.f, 200.f, 300.f, 60.f });
    mbytesAllocated->SetName("mbytesAllocated");
    SetText(mbytesAllocated, "0");
    AddControl(mbytesAllocated);

    Engine::Instance()->lowMemory.Connect(this, &OnLowMemorySignalTest::OnLowMemory);
    Engine::Instance()->update.Connect(this, &OnLowMemorySignalTest::OnUpdate);
}

void OnLowMemorySignalTest::UnloadResources()
{
    DAVA::Engine::Instance()->update.Disconnect(this);
    DAVA::Engine::Instance()->lowMemory.Disconnect(this);

    CleanUp();

    BaseScreen::UnloadResources();
}

void OnLowMemorySignalTest::OnUpdate(DAVA::float32)
{
    using namespace DAVA;

    const size_t mbyte = 1024 * 1024;

    Color c = Color::Black;

    if (isTestRunning)
    {
        auto x = []() { return static_cast<float32>(std::rand() % 256) / 255.f; };

        int chunkSizeMbytes = GetChunkSize();

        c = Color(x(), x(), x(), 1.f);
        memoryChunks.emplace_back(Vector<uint8>(chunkSizeMbytes * mbyte));
        numberOfAllocatedMbytes += chunkSizeMbytes;
    }

    FindByName("startTest")->GetOrCreateComponent<UIControlBackground>()->SetColor(c);

    UIControl* callbackCount = FindByName("callbackCount");
    callbackCount->GetComponent<UITextComponent>()->SetText("Callback called times: " + std::to_string(numberOfCallbackCalls));

    UIControl* mbytesAllocated = FindByName("mbytesAllocated");
    mbytesAllocated->GetComponent<UITextComponent>()->SetText("MBytes allocated: ~" + std::to_string(numberOfAllocatedMbytes));
}

void OnLowMemorySignalTest::ToggleTest()
{
    using namespace DAVA;

    isTestRunning = !isTestRunning;

    UIDebugRenderComponent* bg = FindByName("startTest")->GetComponent<UIDebugRenderComponent>();

    bg->SetDrawColor(isTestRunning ? Color::Green : Color::Red);
}

void OnLowMemorySignalTest::OnLowMemory()
{
    ++numberOfCallbackCalls;

    if (isTestRunning)
    {
        ToggleTest();
    }
}

void OnLowMemorySignalTest::CleanUp()
{
    isTestRunning = false;

    memoryChunks.clear();
}

int OnLowMemorySignalTest::GetChunkSize()
{
    DAVA::UITextField* tf = static_cast<DAVA::UITextField*>(FindByName("chunkSize"));

    int chunkSizeMbytes = minChunkSize;

    try
    {
        chunkSizeMbytes = std::stoi(tf->GetText().c_str());
    }
    catch (const std::exception&)
    {
        DVASSERT(false, DAVA::Format("Invalid value. Value should be in the range [%i; %i]", minChunkSize, maxChunkSize).c_str());
    }

    if (chunkSizeMbytes > maxChunkSize)
    {
        chunkSizeMbytes = maxChunkSize;
    }

    if (chunkSizeMbytes < minChunkSize)
    {
        chunkSizeMbytes = minChunkSize;
    }

    tf->SetText(std::to_wstring(chunkSizeMbytes));

    return chunkSizeMbytes;
}
