#include "Autotesting/AutotestingSystemLua.h"

#ifdef __DAVAENGINE_AUTOTESTING__

#include "Autotesting/AutotestingSystem.h"
#include "Autotesting/AutotestingDB.h"
#include "DeviceManager/DeviceManager.h"
#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Utils/Utils.h"
#include "UI/RichContent/UIRichContentComponent.h"
#include "UI/ScrollHelper.h"
#include "UI/Text/UITextComponent.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIControlSystem.h"
#include "UI/UIListCell.h"
#include "UI/UIStaticText.h"
#include "UI/UITextField.h"
#include "UI/UITextFieldDelegate.h"
#include "UI/UISwitch.h"
#include "UI/UIScreen.h"
#include "UI/UIScrollView.h"
#include "Platform/DeviceInfo.h"
#include "Logger/Logger.h"
#include "Time/SystemTimer.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)
#include "MemoryManager/MemoryProfiler.h"
#endif

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "dlmalloc.h"
};

// directly include wrapped module here to compile only if __DAVAENGINE_AUTOTESTING__ is defined

extern "C" int luaopen_AutotestingSystem(lua_State* l);
extern "C" int luaopen_UIControl(lua_State* l);
extern "C" int luaopen_Rect(lua_State* l);
extern "C" int luaopen_Vector(lua_State* l);
extern "C" int luaopen_KeyedArchive(lua_State* l);
extern "C" int luaopen_Polygon2(lua_State* l);

#define LUA_OK 0

namespace DAVA
{
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
void* lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    if (0 == nsize)
    {
        MemoryManager::Instance()->Deallocate(ptr);
        return nullptr;
    }

    void* newPtr = MemoryManager::Instance()->Allocate(nsize, ALLOC_POOL_LUA);
    if (osize != 0 && newPtr != nullptr)
    {
        size_t n = std::min(osize, nsize);
        memcpy(newPtr, ptr, n);
        MemoryManager::Instance()->Deallocate(ptr);
    }
    return newPtr;
}
#else
static const int32 LUA_MEMORY_POOL_SIZE = 1024 * 1024 * 10;

void* lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
        mspace_free(ud, ptr);
        return nullptr;
    }
    else
    {
        void* mem = mspace_realloc(ud, ptr, nsize);
        DVASSERT(mem);
        return mem;
    }
}
#endif

AutotestingSystemLua::AutotestingSystemLua()
    : delegate(nullptr)
    , luaState(nullptr)
    , memoryPool(nullptr)
    , memorySpace(nullptr)
{
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    // Suppress warning about unused data member
    (void)memoryPool;
#endif
}

AutotestingSystemLua::~AutotestingSystemLua()
{
    if (!luaState)
    {
        return;
    }
    lua_close(luaState);
    luaState = nullptr;

#if !defined(DAVA_MEMORY_PROFILING_ENABLE)
    destroy_mspace(memorySpace);
    free(memoryPool);
#endif
}

void AutotestingSystemLua::SetDelegate(AutotestingSystemLuaDelegate* _delegate)
{
    delegate = _delegate;
}

void AutotestingSystemLua::InitFromFile(const FilePath& luaFilePath)
{
    if (luaState)
    {
        Logger::Debug("AutotestingSystemLua::Has initialised already.");
        return;
    }

    Logger::Debug("AutotestingSystemLua::InitFromFile luaFilePath=%s", luaFilePath.GetAbsolutePathname().c_str());

#if !defined(DAVA_MEMORY_PROFILING_ENABLE)
    memoryPool = malloc(LUA_MEMORY_POOL_SIZE);
    memset(memoryPool, 0, LUA_MEMORY_POOL_SIZE);
    memorySpace = create_mspace_with_base(memoryPool, LUA_MEMORY_POOL_SIZE, 0);
    mspace_set_footprint_limit(memorySpace, LUA_MEMORY_POOL_SIZE);
#endif
    luaState = lua_newstate(lua_allocator, memorySpace);
    luaL_openlibs(luaState);

    lua_pushcfunction(luaState, &AutotestingSystemLua::Print);
    lua_setglobal(luaState, "print");

    lua_pushcfunction(luaState, &AutotestingSystemLua::RequireModule);
    lua_setglobal(luaState, "require");

    if (!LoadWrappedLuaObjects())
    {
        AutotestingSystem::Instance()->ForceQuit("Load wrapped lua objects was failed.");
    }

    FilePath automationAPIStrPath = AutotestingSystem::Instance()->GetPathTo("/Scripts/autotesting_api.lua");
    if (!FileSystem::Instance()->Exists(automationAPIStrPath) || !RunScriptFromFile(automationAPIStrPath))
    {
        AutotestingSystem::Instance()->ForceQuit("Initialization of 'autotesting_api.lua' was failed.");
    }

    lua_getglobal(luaState, "SetPackagePath");
    lua_pushstring(luaState, AutotestingSystem::Instance()->GetPathTo("/").GetAbsolutePathname().c_str());
    if (lua_pcall(luaState, 1, 1, 0))
    {
        const char* err = lua_tostring(luaState, -1);
        AutotestingSystem::Instance()->ForceQuit(Format("AutotestingSystemLua::InitFromFile SetPackagePath failed: %s", err));
    }

    if (!luaFilePath.IsEmpty())
    {
        if (!LoadScriptFromFile(luaFilePath))
        {
            AutotestingSystem::Instance()->ForceQuit("Load of '" + luaFilePath.GetAbsolutePathname() + "' was failed failed");
        }
    }
    else //Empty 'luaFilePath' means we start record&play mode. In this mode we load all requirements beforehand.
    {
        for (const FilePath& path : FileSystem::Instance()->EnumerateFilesInDirectory(AutotestingSystem::Instance()->GetPathTo("/Actions/")))
        {
            RunScript(Format("require '%s'", path.GetBasename().c_str()));
            Logger::FrameworkDebug("Used memory after '%s': %d", path.GetBasename().c_str(), GetUsedMemory());
        }
    }

    lua_getglobal(luaState, "ResumeTest");
    resumeTestFunctionRef = luaL_ref(luaState, LUA_REGISTRYINDEX);
    AutotestingSystem::Instance()->OnInit();

    if (!luaFilePath.IsEmpty())
    {
        String baseName = FilePath(luaFilePath).GetBasename();
        lua_pushstring(luaState, baseName.c_str());
    }

    AutotestingSystem::Instance()->RunTests();
}

void AutotestingSystemLua::StartTest()
{
    RunScript();
}

int AutotestingSystemLua::Print(lua_State* L)
{
    const char* str = lua_tostring(L, -1);
    Logger::Debug("AutotestingSystemLua::Print: %s", str);
    lua_pop(L, 1);
    return 0;
}

const char* AutotestingSystemLua::Pushnexttemplate(lua_State* L, const char* path)
{
    const char* l;
    while (*path == *LUA_PATHSEP) path++; /* skip separators */
    if (*path == '\0')
        return nullptr; /* no more templates */
    l = strchr(path, *LUA_PATHSEP); /* find next separator */
    if (l == nullptr)
        l = path + strlen(path);
    lua_pushlstring(L, path, l - path); /* template */
    return l;
}

const FilePath AutotestingSystemLua::Findfile(lua_State* L, const char* name, const char* pname)
{
    const char* path;
    name = luaL_gsub(L, name, ".", LUA_DIRSEP);
    lua_getglobal(L, "package");
    lua_getfield(L, -1, pname);
    path = lua_tostring(L, -1);
    if (path == nullptr)
        luaL_error(L, LUA_QL("package.%s") " must be a string", pname);
    lua_pushliteral(L, ""); /* error accumulator */
    FilePath filename;
    while ((path = Pushnexttemplate(L, path)) != nullptr)
    {
        filename = luaL_gsub(L, lua_tostring(L, -1), LUA_PATH_MARK, name);
        lua_remove(L, -2); /* remove path template */
        if (FileSystem::Instance()->Exists(filename)) /* does file exist and is readable? */
            return filename; /* return that file name */
        lua_pushfstring(L, "\n\tno file " LUA_QS, filename.GetAbsolutePathname().c_str());
        lua_remove(L, -2); /* remove file name */
        lua_concat(L, 2); /* add entry to possible error message */
    }
    return name; /* not found */
}

int AutotestingSystemLua::RequireModule(lua_State* L)
{
    String module = lua_tostring(L, -1);
    lua_pop(L, 1);
    FilePath path = Instance()->Findfile(L, module.c_str(), "path");
    if (!Instance()->LoadScriptFromFile(path))
    {
        AutotestingSystem::Instance()->ForceQuit("AutotestingSystemLua::RequireModule: couldn't load module " + path.GetAbsolutePathname());
    }
    lua_pushstring(Instance()->luaState, path.GetBasename().c_str());
    if (!Instance()->RunScript())
    {
        AutotestingSystem::Instance()->ForceQuit("AutotestingSystemLua::RequireModule: couldn't run module " + path.GetBasename());
    }
    lua_pushcfunction(L, lua_tocfunction(Instance()->luaState, -1));
    lua_pushstring(L, path.GetBasename().c_str());
    return 2;
}

void AutotestingSystemLua::StackDump(lua_State* L)
{
    Logger::FrameworkDebug("*** Stack Dump ***");
    int i;
    int top = lua_gettop(L);

    for (i = 1; i <= top; i++) /* repeat for each level */
    {
        int t = lua_type(L, i);
        switch (t)
        {
        case LUA_TSTRING:
        { /* strings */
            Logger::FrameworkDebug("'%s'", lua_tostring(L, i));
            break;
        }
        case LUA_TBOOLEAN:
        { /* booleans */
            Logger::FrameworkDebug(lua_toboolean(L, i) ? "true" : "false");
            break;
        }
        case LUA_TNUMBER:
        { /* numbers */
            Logger::FrameworkDebug("%g", lua_tonumber(L, i));
            break;
        }
        default:
        { /* other values */
            Logger::FrameworkDebug("%s", lua_typename(L, t));
            break;
        }
        }
    }
    Logger::FrameworkDebug("*** Stack Dump END***"); /* end the listing */
}

// Multiplayer API
void AutotestingSystemLua::WriteState(const String& device, const String& param, const String& state)
{
    Logger::FrameworkDebug("AutotestingSystemLua::WriteState device=%s param=%s state=%s", device.c_str(), param.c_str(), state.c_str());
    AutotestingDB::Instance()->WriteState(device, param, state);
}

String AutotestingSystemLua::ReadState(const String& device, const String& param)
{
    Logger::FrameworkDebug("AutotestingSystemLua::ReadState device=%s param=%s", device.c_str(), param.c_str());
    return AutotestingDB::Instance()->ReadState(device, param);
}

void AutotestingSystemLua::InitializeDevice()
{
    AutotestingSystem::Instance()->InitializeDevice();
}

String AutotestingSystemLua::GetPlatform()
{
    return DeviceInfo::GetPlatformString();
}

String AutotestingSystemLua::GetDeviceName()
{
    String deviceName;
    if (DeviceInfo::GetPlatformString() == "Android")
    {
        deviceName = DeviceInfo::GetModel();
    }
    else
    {
        deviceName = UTF8Utils::EncodeToUTF8(DeviceInfo::GetName());
    }
    replace(deviceName.begin(), deviceName.end(), ' ', '_');
    replace(deviceName.begin(), deviceName.end(), '-', '_');
    return deviceName;
}

bool AutotestingSystemLua::IsPhoneScreen()
{
    const DisplayInfo& primaryDisplay = GetEngineContext()->deviceManager->GetPrimaryDisplay();
    float32 xInch = primaryDisplay.rect.dx / primaryDisplay.rawDpiX;
    float32 yInch = primaryDisplay.rect.dy / primaryDisplay.rawDpiY;
    float32 diag = sqrtf(xInch * xInch + yInch * yInch);
    return diag <= 6.5f;
}

String AutotestingSystemLua::GetTestParameter(const String& parameter)
{
    Logger::FrameworkDebug("AutotestingSystemLua::GetTestParameter parameter=%s", parameter.c_str());
    String result = AutotestingDB::Instance()->GetStringTestParameter(AutotestingSystem::Instance()->deviceName, parameter);
    Logger::FrameworkDebug("AutotestingSystemLua::GetTestParameter value=%s", result.c_str());
    return result;
}

void AutotestingSystemLua::Update(float32 timeElapsed)
{
    lua_rawgeti(luaState, LUA_REGISTRYINDEX, resumeTestFunctionRef);
    if (lua_pcall(luaState, 0, 1, 0))
    {
        const char* err = lua_tostring(luaState, -1);
        Logger::Error("AutotestingSystemLua::Update error: %s", err);
    }
}

float32 AutotestingSystemLua::GetTimeElapsed()
{
    return SystemTimer::GetRealFrameDelta();
}

DAVA::int64 AutotestingSystemLua::GetMsSinceEpoch()
{
    return SystemTimer::GetMs();
}

void AutotestingSystemLua::OnError(const String& errorMessage)
{
    AutotestingSystem::Instance()->OnError(errorMessage);
}

void AutotestingSystemLua::OnTestStart(const String& testDescription)
{
    Logger::FrameworkDebug("AutotestingSystemLua::OnTestStart %s", testDescription.c_str());
    AutotestingSystem::Instance()->OnTestStart(testDescription);
}

void AutotestingSystemLua::OnTestFinished()
{
    Logger::FrameworkDebug("AutotestingSystemLua::OnTestFinished");
    AutotestingSystem::Instance()->OnTestsFinished();
}

void AutotestingSystemLua::OnTestSkipped()
{
    Logger::FrameworkDebug("AutotestingSystemLua::OnTestSkipped");
    AutotestingSystem::Instance()->OnTestSkipped();
}

size_t AutotestingSystemLua::GetUsedMemory() const
{
    return lua_gc(luaState, LUA_GCCOUNT, 0) * 1024 + lua_gc(luaState, LUA_GCCOUNTB, 0);
}

void AutotestingSystemLua::OnStepStart(const String& stepName)
{
    AutotestingSystem::Instance()->stepIndex++;
    Logger::FrameworkDebug("AutotestingSystemLua::OnStepStart %s", stepName.c_str());
    AutotestingSystem::Instance()->OnStepStart(Format("%d %s", AutotestingSystem::Instance()->stepIndex, stepName.c_str()));
}

void AutotestingSystemLua::Log(const String& level, const String& message)
{
    AutotestingDB::Instance()->Log(level, message);
}

bool AutotestingSystemLua::SaveKeyedArchiveToDevice(const String& archiveName, KeyedArchive* archive)
{
    Logger::FrameworkDebug("AutotestingSystemLua::SaveKeyedArchiveToDevice");
    return AutotestingDB::Instance()->SaveKeyedArchiveToDevice(archiveName, archive);
}

String AutotestingSystemLua::MakeScreenshot()
{
    Logger::FrameworkDebug("AutotestingSystemLua::MakeScreenshot");
    AutotestingSystem::Instance()->MakeScreenShot();
    return AutotestingSystem::Instance()->GetScreenShotName();
}

bool AutotestingSystemLua::GetIsScreenShotSaving() const
{
    return AutotestingSystem::Instance()->GetIsScreenShotSaving();
}

UIControl* AutotestingSystemLua::GetScreen()
{
    return GetEngineContext()->uiControlSystem->GetScreen();
}

UIControl* AutotestingSystemLua::FindControlOnPopUp(const String& path) const
{
    return FindControl(path, GetEngineContext()->uiControlSystem->GetPopupContainer());
}

UIControl* AutotestingSystemLua::FindControl(const String& path) const
{
    return FindControl(path, GetEngineContext()->uiControlSystem->GetScreen());
}

UIControl* AutotestingSystemLua::FindControl(const String& path, UIControl* srcControl) const
{
    Vector<String> controlPath;
    ParsePath(path, controlPath);

    if (GetEngineContext()->uiControlSystem->GetLockInputCounter() > 0 || !srcControl || controlPath.empty())
    {
        return nullptr;
    }

    String zeroOrMoreLevelsWildcard = "**";

    if (controlPath[0] == zeroOrMoreLevelsWildcard)
    {
        return srcControl->FindByPath(path);
    }

    UIControl* control = FindControl(srcControl, controlPath[0]);
    for (uint32 i = 1; i < controlPath.size(); ++i)
    {
        if (!control)
        {
            return control;
        }
        control = FindControl(control, controlPath[i]);
    }
    return control;
}

UIControl* AutotestingSystemLua::FindControl(UIControl* srcControl, const String& controlName) const
{
    if (GetEngineContext()->uiControlSystem->GetLockInputCounter() > 0 || !srcControl)
    {
        return nullptr;
    }

    int32 index = atoi(controlName.c_str());
    // not number
    if (Format("%d", index) != controlName)
    {
        if (srcControl->GetName().c_str() == controlName)
            return srcControl;
        return srcControl->FindByName(controlName);
    }
    // number
    UIList* list = dynamic_cast<UIList*>(srcControl);
    if (list)
    {
        return FindControl(list, index);
    }
    return FindControl(srcControl, index);
}

UIControl* AutotestingSystemLua::FindControl(UIControl* srcControl, int32 index) const
{
    if (GetEngineContext()->uiControlSystem->GetLockInputCounter() > 0 || !srcControl)
    {
        return nullptr;
    }
    const auto& children = srcControl->GetChildren();
    int32 childIndex = 0;
    for (auto it = children.begin(); it != children.end(); ++it, ++childIndex)
    {
        if (childIndex == index)
        {
            return it->Get();
        }
    }
    return nullptr;
}

UIControl* AutotestingSystemLua::FindControl(UIList* srcList, int32 index) const
{
    if (GetEngineContext()->uiControlSystem->GetLockInputCounter() > 0 || !srcList)
    {
        return nullptr;
    }
    const auto& cells = srcList->GetVisibleCells();
    for (auto it = cells.begin(); it != cells.end(); ++it)
    {
        UIListCell* cell = dynamic_cast<UIListCell*>(it->Get());
        if (cell && cell->GetIndex() == index && IsCenterInside(srcList, cell))
        {
            return cell;
        }
    }
    return nullptr;
}

bool AutotestingSystemLua::IsCenterInside(UIControl* parent, UIControl* child) const
{
    if (!parent || !child)
    {
        return false;
    }
    const Rect& parentRect = parent->GetGeometricData().GetUnrotatedRect();
    const Rect& childRect = child->GetGeometricData().GetUnrotatedRect();
    // check if child center is inside parent rect
    return ((parentRect.x <= childRect.x + childRect.dx / 2) && (childRect.x + childRect.dx / 2 <= parentRect.x + parentRect.dx) &&
            (parentRect.y <= childRect.y + childRect.dy / 2) && (childRect.y + childRect.dy / 2 <= parentRect.y + parentRect.dy));
}

bool AutotestingSystemLua::SetText(const String& path, const String& text)
{
    UITextField* tf = dynamic_cast<UITextField*>(FindControl(path));
    if (tf)
    {
        tf->SetText(UTF8Utils::EncodeToWideString(text));
        return true;
    }
    return false;
}

void AutotestingSystemLua::KeyPress(int32 keyChar)
{
    UITextField* uiTextField = dynamic_cast<UITextField*>(GetEngineContext()->uiControlSystem->GetFocusedControl());
    if (!uiTextField)
    {
        return;
    }

    UIEvent keyPress;
    keyPress.keyChar = keyChar;
    keyPress.phase = UIEvent::Phase::CHAR;

    Logger::Info("AutotestingSystemLua::KeyPress %d phase=%d key=%c", keyPress.key, keyPress.phase, keyPress.keyChar);
    switch (keyPress.keyChar)
    {
    case '\b':
    {
        //TODO: act the same way on iPhone
        WideString str = L"";
        if (uiTextField->GetDelegate()->TextFieldKeyPressed(uiTextField, static_cast<int32>(uiTextField->GetText().length()), -1, str))
        {
            uiTextField->SetText(uiTextField->GetAppliedChanges(static_cast<int32>(uiTextField->GetText().length()), -1, str));
        }
        break;
    }
    case '\n':
    {
        uiTextField->GetDelegate()->TextFieldShouldReturn(uiTextField);
        break;
    }
    case 27: // ESCAPE
    {
        uiTextField->GetDelegate()->TextFieldShouldCancel(uiTextField);
        break;
    }
    default:
    {
        if (keyPress.keyChar == 0)
        {
            break;
        }
        Array<char32_t, 2> str32 = { keyPress.keyChar, 0 };
        String utf8string = UTF8Utils::EncodeToUTF8(str32.data());
        WideString wstr = UTF8Utils::EncodeToWideString(utf8string);
        if (uiTextField->GetDelegate()->TextFieldKeyPressed(uiTextField, static_cast<int32>(uiTextField->GetText().length()), 1, wstr))
        {
            uiTextField->SetText(uiTextField->GetAppliedChanges(static_cast<int32>(uiTextField->GetText().length()), 1, wstr));
        }
        break;
    }
    }
}

void AutotestingSystemLua::ClickSystemBack()
{
    Logger::FrameworkDebug("AutotestingSystemLua::ClickSystemBack");
    AutotestingSystem::Instance()->ClickSystemBack();
}

void AutotestingSystemLua::EmulatePressKey(DAVA::uint32 key)
{
    Logger::Debug("AutotestingSystemLua::EmulatePressKey");
    AutotestingSystem::Instance()->EmulatePressKey(key);
}

String AutotestingSystemLua::GetText(UIControl* control)
{
    UITextField* uiTextField = dynamic_cast<UITextField*>(control);
    if (uiTextField != nullptr)
    {
        return UTF8Utils::EncodeToUTF8(uiTextField->GetText());
    }
    UITextComponent* uiTextComponent = control->GetComponent<UITextComponent>();
    if (uiTextComponent != nullptr)
    {
        return uiTextComponent->GetText();
    }
    return "";
}

String AutotestingSystemLua::GetRichText(UIControl* control)
{
    UIRichContentComponent* uiRichContentComponent = control->GetComponent<UIRichContentComponent>();
    if (uiRichContentComponent != nullptr)
    {
        return uiRichContentComponent->GetText();
    }
    return "";
}

String AutotestingSystemLua::GetTaggedClass(UIControl* control, const String& tag)
{
    const FastName& value = control->GetTaggedClass(FastName(tag));
    if (value.IsValid())
    {
        return value.c_str();
    }
    return "";
}

uint32 AutotestingSystemLua::GetTextColor(UIControl* control)
{
    UIStaticText* uiStaticText = dynamic_cast<UIStaticText*>(control);
    if (uiStaticText != nullptr)
    {
        return uiStaticText->GetTextColor().GetRGBA();
    }
    UITextField* uiTextField = dynamic_cast<UITextField*>(control);
    if (uiTextField != nullptr)
    {
        return uiTextField->GetTextColor().GetRGBA();
    }
    return 0;
}

bool AutotestingSystemLua::IsSelected(UIControl* control) const
{
    Logger::Debug("AutotestingSystemLua::IsSelected Check is control %s selected", control->GetName().c_str());
    UISwitch* switchControl = dynamic_cast<UISwitch*>(control);
    if (switchControl)
    {
        return switchControl->GetIsLeftSelected();
    }
    AutotestingSystem::Instance()->OnError(Format("AutotestingSystemLua::IsSelected Couldn't get parameter for '%s'", control->GetName().c_str()));
    return false;
}

bool AutotestingSystemLua::IsListHorisontal(UIControl* control)
{
    UIList* list = dynamic_cast<UIList*>(control);
    if (!list)
    {
        AutotestingSystem::Instance()->OnError("AutotestingSystemLua::Can't get UIList obj.");
    }
    return list->GetOrientation() == UIList::ORIENTATION_HORIZONTAL;
}

float32 AutotestingSystemLua::GetListScrollPosition(UIControl* control)
{
    UIList* list = dynamic_cast<UIList*>(control);
    if (!list)
    {
        AutotestingSystem::Instance()->OnError("AutotestingSystemLua::Can't get UIList obj.");
    }
    float32 position = list->GetScrollPosition();
    if (position < 0)
    {
        position *= -1;
    }
    return position;
}

float32 AutotestingSystemLua::GetMaxListOffsetSize(UIControl* control)
{
    UIList* list = dynamic_cast<UIList*>(control);
    if (!list)
    {
        AutotestingSystem::Instance()->OnError("AutotestingSystemLua::Can't get UIList obj.");
    }
    float32 size;
    float32 areaSize = list->TotalAreaSize(nullptr);
    Vector2 visibleSize = control->GetSize();
    if (list->GetOrientation() == UIList::ORIENTATION_HORIZONTAL)
    {
        size = areaSize - visibleSize.x;
    }
    else
    {
        size = areaSize - visibleSize.y;
    }
    return size;
}

Vector2 AutotestingSystemLua::GetContainerScrollPosition(UIControl* control)
{
    UIScrollView* scrollView = dynamic_cast<UIScrollView*>(control);
    if (!scrollView)
    {
        AutotestingSystem::Instance()->OnError("AutotestingSystemLua::Can't get UIScrollView obj.");
    }
    Vector2 position = scrollView->GetScrollPosition();
    return Vector2(position.x * (-1), position.y * (-1));
}

Vector2 AutotestingSystemLua::GetMaxContainerOffsetSize(UIControl* control)
{
    UIScrollView* scrollView = dynamic_cast<UIScrollView*>(control);
    if (!scrollView)
    {
        AutotestingSystem::Instance()->OnError("AutotestingSystemLua::Can't get UIScrollView obj.");
    }
    ScrollHelper* horizontalScroll = scrollView->GetHorizontalScroll();
    ScrollHelper* verticalScroll = scrollView->GetVerticalScroll();
    Vector2 totalAreaSize(horizontalScroll->GetElementSize(), verticalScroll->GetElementSize());
    Vector2 visibleAreaSize(horizontalScroll->GetViewSize(), verticalScroll->GetViewSize());
    return Vector2(totalAreaSize.x - visibleAreaSize.x, totalAreaSize.y - visibleAreaSize.y);
}

bool AutotestingSystemLua::CheckText(UIControl* control, const String& expectedText)
{
    UIStaticText* uiStaticText = dynamic_cast<UIStaticText*>(control);
    if (uiStaticText)
    {
        String actualText = UTF8Utils::EncodeToUTF8(uiStaticText->GetText());
        return (actualText == expectedText);
    }
    UITextField* uiTextField = dynamic_cast<UITextField*>(control);
    if (uiTextField)
    {
        String actualText = UTF8Utils::EncodeToUTF8(uiTextField->GetText());
        return (actualText == expectedText);
    }
    return false;
}

bool AutotestingSystemLua::CheckMsgText(UIControl* control, const String& key)
{
    WideString expectedText = UTF8Utils::EncodeToWideString(key);

    UIStaticText* uiStaticText = dynamic_cast<UIStaticText*>(control);
    if (uiStaticText)
    {
        WideString actualText = uiStaticText->GetText();
        return (actualText == expectedText);
    }
    UITextField* uiTextField = dynamic_cast<UITextField*>(control);
    if (uiTextField)
    {
        WideString actualText = uiTextField->GetText();
        return (actualText == expectedText);
    }
    return false;
}

void AutotestingSystemLua::TouchDown(const Vector2& point, int32 touchId)
{
    UIEvent touchDown;
    touchDown.phase = UIEvent::Phase::BEGAN;
    touchDown.touchId = touchId;
    touchDown.timestamp = SystemTimer::GetMs() / 1000.0;
    touchDown.physPoint = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(point);
    touchDown.point = point;
    ProcessInput(touchDown);
}

void AutotestingSystemLua::TouchMove(const Vector2& point, int32 touchId)
{
    UIEvent touchMove;
    touchMove.touchId = touchId;
    touchMove.timestamp = SystemTimer::GetMs() / 1000.0;
    touchMove.physPoint = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(point);
    touchMove.point = point;

    if (AutotestingSystem::Instance()->IsTouchDown(touchId))
    {
        touchMove.phase = UIEvent::Phase::DRAG;
        ProcessInput(touchMove);
    }
    else
    {
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
        Logger::Warning("AutotestingSystemLua::TouchMove point=(%f, %f) ignored no touch down found", point.x, point.y);
#else
        touchMove.phase = UIEvent::Phase::MOVE;
        ProcessInput(touchMove);
#endif
    }
}

void AutotestingSystemLua::TouchUp(int32 touchId)
{
    UIEvent touchUp;
    if (!AutotestingSystem::Instance()->FindTouch(touchId, touchUp))
    {
        AutotestingSystem::Instance()->OnError("TouchAction::TouchUp touch down not found");
    }
    touchUp.phase = UIEvent::Phase::ENDED;
    touchUp.touchId = touchId;
    touchUp.timestamp = SystemTimer::GetMs() / 1000.0;

    ProcessInput(touchUp);
}

void AutotestingSystemLua::LeftMouseClickDown(const Vector2& point)
{
    UIEvent clickDown;
    clickDown.phase = UIEvent::Phase::BEGAN;
    clickDown.device = eInputDevices::MOUSE;
    clickDown.mouseButton = eMouseButtons::LEFT;
    clickDown.timestamp = SystemTimer::GetMs() / 1000.0;
    clickDown.physPoint = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(point);
    clickDown.point = point;
    ProcessInput(clickDown);
}

void AutotestingSystemLua::LeftMouseClickUp(const Vector2& point)
{
    UIEvent clickUp;
    if (!AutotestingSystem::Instance()->FindTouch(static_cast<int32>(eMouseButtons::LEFT), clickUp))
    {
        AutotestingSystem::Instance()->OnError("ClickAction::LeftMouseClickUp click down not found");
    }
    clickUp.phase = UIEvent::Phase::ENDED;
    clickUp.device = eInputDevices::MOUSE;
    clickUp.mouseButton = eMouseButtons::LEFT;
    clickUp.timestamp = SystemTimer::GetMs() / 1000.0;
    clickUp.physPoint = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(point);
    clickUp.point = point;
    ProcessInput(clickUp);
}

void AutotestingSystemLua::MouseWheel(const Vector2& point, float32 x, float32 y)
{
    UIEvent wheel;
    wheel.wheelDelta.x = x;
    wheel.wheelDelta.y = y;
    wheel.phase = UIEvent::Phase::WHEEL;
    wheel.device = eInputDevices::MOUSE;
    wheel.timestamp = SystemTimer::GetMs() / 1000.0;
    wheel.physPoint = GetEngineContext()->uiControlSystem->vcs->ConvertVirtualToInput(point);
    wheel.point = point;
    ProcessInput(wheel);
}

void AutotestingSystemLua::ScrollToControl(const String& path) const
{
    UIControl* control = FindControl(path);
    if (control != nullptr)
    {
        UIControlHelpers::ScrollToControl(control);
    }
}

void AutotestingSystemLua::ProcessInput(const UIEvent& input)
{
    UIEvent ev = input;
    GetEngineContext()->uiControlSystem->OnInput(&ev);

    AutotestingSystem::Instance()->OnInput(input);
}

inline void AutotestingSystemLua::ParsePath(const String& path, Vector<String>& parsedPath) const
{
    Split(path, "/", parsedPath);
}

bool AutotestingSystemLua::LoadWrappedLuaObjects()
{
    if (!luaState)
    {
        return false; //TODO: report error?
    }

    luaopen_AutotestingSystem(luaState); // load the wrappered module
    luaopen_UIControl(luaState); // load the wrappered module
    luaopen_Rect(luaState); // load the wrappered module
    luaopen_Vector(luaState); // load the wrappered module
    luaopen_KeyedArchive(luaState); // load the wrappered module
    luaopen_Polygon2(luaState); // load the wrappered module

    if (!delegate)
    {
        return false;
    }
    //TODO: check if modules really loaded
    return delegate->LoadWrappedLuaObjects(luaState);
}

bool AutotestingSystemLua::LoadScript(const String& luaScript)
{
    if (!luaState)
    {
        return false;
    }
    if (luaL_loadstring(luaState, luaScript.c_str()) != 0)
    {
        Logger::Error("AutotestingSystemLua::LoadScript Error: unable to load %s", luaScript.c_str());
        return false;
    }
    return true;
}

bool AutotestingSystemLua::LoadScriptFromFile(const FilePath& luaFilePath)
{
    Logger::FrameworkDebug("AutotestingSystemLua::LoadScriptFromFile: %s", luaFilePath.GetAbsolutePathname().c_str());
    File* file = File::Create(luaFilePath, File::OPEN | File::READ);
    if (!file)
    {
        Logger::Error("AutotestingSystemLua::LoadScriptFromFile: couldn't open %s", luaFilePath.GetAbsolutePathname().c_str());
        return false;
    }
    char* data = new char[static_cast<size_t>(file->GetSize())];
    file->Read(data, static_cast<uint32>(file->GetSize()));
    uint32 fileSize = static_cast<uint32>(file->GetSize());
    file->Release();
    bool result = luaL_loadbuffer(luaState, data, fileSize, luaFilePath.GetAbsolutePathname().c_str()) == LUA_OK;
    delete[] data;
    if (!result)
    {
        Logger::Error("AutotestingSystemLua::LoadScriptFromFile: couldn't load buffer %s", luaFilePath.GetAbsolutePathname().c_str());
        Logger::Error("%s", lua_tostring(luaState, -1));
        return false;
    }
    return true;
}

bool AutotestingSystemLua::RunScriptFromFile(const FilePath& luaFilePath)
{
    Logger::FrameworkDebug("AutotestingSystemLua::RunScriptFromFile %s", luaFilePath.GetAbsolutePathname().c_str());
    if (LoadScriptFromFile(luaFilePath))
    {
        lua_pushstring(luaState, luaFilePath.GetBasename().c_str());
        return RunScript();
    }
    return false;
}

bool AutotestingSystemLua::RunScript(const String& luaScript)
{
    if (!LoadScript(luaScript))
    {
        Logger::Error("AutotestingSystemLua::RunScript couldnt't load script %s", luaScript.c_str());
        return false;
    }
    if (lua_pcall(luaState, 0, 1, 0))
    {
        const char* err = lua_tostring(luaState, -1);
        Logger::Error("AutotestingSystemLua::RunScript error: %s", err);
        return false;
    }
    return true;
}

bool AutotestingSystemLua::RunScript()
{
    if (lua_pcall(luaState, 1, 1, 0))
    {
        const char* err = lua_tostring(luaState, -1);
        Logger::Debug("AutotestingSystemLua::RunScript error: %s", err);
        return false;
    }
    return true;
}
int32 AutotestingSystemLua::GetServerQueueState(const String& cluster)
{
    int32 queueState = 0;
    if (AutotestingSystem::Instance()->isDB)
    {
        RefPtr<MongodbUpdateObject> dbUpdateObject(new MongodbUpdateObject);
        KeyedArchive* clustersQueue = AutotestingDB::Instance()->FindOrInsertBuildArchive(dbUpdateObject.Get(), "clusters_queue");
        String serverName = Format("%s", cluster.c_str());

        if (!clustersQueue->IsKeyExists(serverName))
        {
            clustersQueue->SetInt32(serverName, 0);
        }
        else
        {
            queueState = clustersQueue->GetInt32(serverName);
        }
    }
    return queueState;
}

bool AutotestingSystemLua::SetServerQueueState(const String& cluster, int32 state)
{
    if (!AutotestingSystem::Instance()->isDB)
    {
        return true;
    }
    RefPtr<MongodbUpdateObject> dbUpdateObject(new MongodbUpdateObject);
    KeyedArchive* clustersQueue = AutotestingDB::Instance()->FindOrInsertBuildArchive(dbUpdateObject.Get(), "clusters_queue");
    String serverName = Format("%s", cluster.c_str());
    bool isSet = false;
    if (!clustersQueue->IsKeyExists(serverName) || clustersQueue->GetInt32(serverName) != state)
    {
        clustersQueue->SetInt32(serverName, state);
        isSet = AutotestingDB::Instance()->SaveToDB(dbUpdateObject.Get());
    }
    return isSet;
}
};

#endif //__DAVAENGINE_AUTOTESTING__
