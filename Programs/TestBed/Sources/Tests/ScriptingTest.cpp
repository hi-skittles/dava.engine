#include "Tests/ScriptingTest.h"
#include "Base/Type.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scripting/LuaScript.h"
#include "UI/Events/UIEventBindingComponent.h"
#include "Utils/StringUtils.h"

using namespace DAVA;

class SubObj : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SubObj)
    {
        ReflectionRegistrator<SubObj>::Begin()
        .Field("a", &SubObj::a)
        .Field("b", &SubObj::b)
        .Field("c", &SubObj::c)
        .End();
    }

public:
    int32 a = 10;
    WideString b = L"WideString";
    Color c = Color::Black;
};

class DemoBase : public ReflectionBase
{
public:
    int32 a = 99;
    String b = "String";
    Color c = Color::White;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(DemoBase)
    {
        ReflectionRegistrator<DemoBase>::Begin()
        .Field("a", &DemoBase::a)
        .Field("b", &DemoBase::b)
        .Field("c", &DemoBase::c)
        .End();
    }
};

class DemoObj : public DemoBase
{
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(DemoObj, DemoBase)
    {
        ReflectionRegistrator<DemoObj>::Begin()
        .Field("d", &DemoObj::d)
        .Field("v", &DemoObj::v)
        .Method("foo", &DemoObj::foo)
        .Method("foo2", &DemoObj::foo2)
        .Method("bar", &DemoObj::bar)
        .Method("bar2", &DemoObj::bar2)
        .End();
    }

public:
    SubObj d;
    Vector<int32> v;
    void foo()
    {
        Logger::Debug("Invoke DemoObj::foo()");
    }
    void foo2(int32 a1, String a2, bool a3, Color a4)
    {
        Logger::Debug("Invoke DemoObj::foo(%d, %s, %s, Color(%1.1f, %1.1f, %1.1f, %1.1f)",
                      a1, a2.c_str(), a3 ? "true" : "false", a4.r, a4.g, a4.b, a4.a);
    }
    int32 bar(int32 arg)
    {
        Logger::Debug("Invoke DemoObj::bar(%d)", arg);
        return arg;
    }
    bool bar2(bool arg)
    {
        Logger::Debug("Invoke DemoObj::bar(%s)", arg ? "true" : "false");
        return arg;
    }
};

DemoObj demoObj;
Reflection objRef;
LuaScript* script = nullptr;

static const String demo_script = R"script(
function main(int, str, ref)
    ref.a = int
    ref.b = str
    return ref.a, ref.b, ref.c
end
)script";

ScriptingTest::ScriptingTest(TestBed& app)
    : BaseScreen(app, "ScriptingTest")
{
}

void ScriptingTest::LoadResources()
{
    BaseScreen::LoadResources();

    DAVA::DefaultUIPackageBuilder pkgBuilder;
    DAVA::UIPackageLoader().LoadPackage("~res:/TestBed/UI/ScriptingTest.yaml", &pkgBuilder);
    UIControl* dialog = pkgBuilder.GetPackage()->GetControl("MainFrame");
    AddControl(dialog);

    scriptText = static_cast<UITextField*>(dialog->FindByName("ScriptText"));
    intArgText = static_cast<UITextField*>(dialog->FindByName("IntArgText"));
    strArgText = static_cast<UITextField*>(dialog->FindByName("StrArgText"));
    outputText = static_cast<UIStaticText*>(dialog->FindByName("OutputText"));
    timeText = static_cast<UIStaticText*>(dialog->FindByName("TimeText"));

    scriptText->SetUtf8Text(demo_script);
    intArgText->SetUtf8Text("42");
    strArgText->SetUtf8Text("demoStr");
    outputText->SetUtf8Text("");

    auto actions = dialog->GetOrCreateComponent<UIEventBindingComponent>();
    actions->BindAction(FastName("LOAD_SCRIPT"), [&](const DAVA::Any&) {
        String scriptBody = scriptText->GetUtf8Text();
        Run([&]() -> int32 {
            return script->ExecString(scriptBody);
        });
    });
    actions->BindAction(FastName("RUN_MAIN"), [&](const DAVA::Any&) {
        int32 intArg = atoi(intArgText->GetUtf8Text().c_str());
        String strArg = strArgText->GetUtf8Text();
        Run([&]() -> int32 {
            return script->ExecFunction("main", intArg, strArg, objRef);
        });
    });
    actions->BindAction(FastName("RUN_MAIN_NOARGS"), [&](const DAVA::Any&) {
        Run([&]() -> int32 {
            return script->ExecFunction("main");
        });
    });
    actions->BindAction(FastName("RUN_10000"), [&](const DAVA::Any&) {
        int32 intArg = atoi(intArgText->GetUtf8Text().c_str());
        String strArg = strArgText->GetUtf8Text();
        Run([&]() -> int32 {
            for (int32 i = 0; i < 10000; ++i)
            {
                int32 nresults = script->ExecFunction("main", intArg, strArg, objRef);
                script->Pop(nresults);
            }
            return 0;
        });
    });
    actions->BindAction(FastName("RUN_10000_NOARGS"), [&](const DAVA::Any&) {
        Run([&]() -> int32 {
            for (int32 i = 0; i < 10000; ++i)
            {
                int32 nresults = script->ExecFunction("main");
                script->Pop(nresults);
            }
            return 0;
        });
    });
    actions->BindAction(FastName("RESET_SCRIPT"), [&](const DAVA::Any&) {
        CreateScript();
    });
    actions->BindAction(FastName("DUMP_STACK"), [&](const DAVA::Any&) {
        if (script)
        {
            script->DumpStackToLog(Logger::LEVEL_DEBUG);
        }
    });

    demoObj.v.assign({ 1, 2, 3, 4, 5 });
    demoObj.c = Color::White;
    objRef = Reflection::Create(&demoObj);
    CreateScript();
}

void ScriptingTest::UnloadResources()
{
    SafeDelete(script);

    scriptText.Set(nullptr);
    intArgText.Set(nullptr);
    strArgText.Set(nullptr);
    outputText.Set(nullptr);
    timeText.Set(nullptr);

    BaseScreen::UnloadResources();
}

void ScriptingTest::Update(DAVA::float32 timeElapsed)
{
}

void ScriptingTest::CreateScript()
{
    SafeDelete(script);
    script = new LuaScript();
    script->SetGlobalVariable("GlobRef", objRef);
}

void ScriptingTest::Run(Function<int32()> func)
{
    try
    {
        uint64 begin = SystemTimer::GetUs();
        int32 nresults = func();
        uint64 time = SystemTimer::GetUs() - begin;

        String output = Format("Run main() time: %llu us\n", time);
        for (int32 i = 1; i <= nresults; ++i)
        {
            Any val = script->GetResult(i);
            output += Format("%d) %s\n", i, AnyToString(val).c_str());
        }
        script->Pop(nresults);
        outputText->SetUtf8Text(output);
        timeText->SetUtf8Text(Format("Time: %llu us", time));
    }
    catch (const LuaException& e)
    {
        String error = Format("LuaException: %s", e.what());
        Logger::Error(error.c_str());
        outputText->SetUtf8Text(error);
        timeText->SetUtf8Text("Error");
    }
}

String ScriptingTest::AnyToString(const Any& any)
{
#define IFFORMAT(t, frm) if (any.CanGet<t>()) { return Format(frm, any.Get<t>()); }
#define IFFORMATEX(t, frm, exp) if (any.CanGet<t>()) { return Format(frm, (exp)); }

    if (any.IsEmpty())
        return "Any <empty>";
    else
        IFFORMATEX(bool, "%s", any.Get<bool>() ? "true" : "false")
    else IFFORMAT(int8, "%d")
    else IFFORMAT(int16, "%d")
    else IFFORMAT(int32, "%d")
    else IFFORMAT(int64, "%d")
    else IFFORMAT(char8, "%d")
    else IFFORMAT(char16, "%d")
    else IFFORMAT(float32, "%f")
    else IFFORMAT(float64, "%f")
    else IFFORMAT(const char*, "%s")
    else IFFORMATEX(String, "%s", any.Get<String>().c_str())
    else IFFORMATEX(WideString, "%s", UTF8Utils::EncodeToUTF8(any.Get<WideString>()).c_str())
    else IFFORMATEX(Reflection, "Any <Reflection <%s>>", any.Get<Reflection>().IsValid() ? any.Get<Reflection>().GetValueType()->GetName() : "non valid")
    else IFFORMATEX(Any, "Any <%s>", AnyToString(any.Get<Any>()).c_str())
    else IFFORMATEX(AnyFn, "Any <AnyFn <%s>>", any.Get<AnyFn>().IsValid() ? "..." : "non valid")
    else return Format("Any <%s>", any.GetType()->GetName());

#undef IFFORMAT
#undef IFFORMATEX
}
