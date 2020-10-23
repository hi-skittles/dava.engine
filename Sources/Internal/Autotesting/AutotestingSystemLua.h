#ifndef __DAVAENGINE_AUTOTESTING_SYSTEM_LUA_H__
#define __DAVAENGINE_AUTOTESTING_SYSTEM_LUA_H__

#include "DAVAConfig.h"

#ifdef __DAVAENGINE_AUTOTESTING__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Math/Vector.h"
#include "Base/Singleton.h"

#include "UI/UIControl.h"
#include "UI/UIList.h"

#include "FileSystem/LocalizationSystem.h"
#include "Autotesting/AutotestingSystem.h"

struct lua_State;

namespace DAVA
{
    
#if !defined(SWIG)
class AutotestingSystemLuaDelegate
{
public:
    virtual ~AutotestingSystemLuaDelegate()
    {
    }

    virtual bool LoadWrappedLuaObjects(lua_State* luaState) = 0;
};
#endif //SWIG

class AutotestingSystemLua : public Singleton<AutotestingSystemLua>
{
public:
    AutotestingSystemLua();
    ~AutotestingSystemLua();
    
#if !defined(SWIG)
    void SetDelegate(AutotestingSystemLuaDelegate* _delegate);

    void InitFromFile(const FilePath& luaFilePath);

    void StartTest();

    void Update(float32 timeElapsed);

    static int Print(lua_State* L);
    static int RequireModule(lua_State* L);

    static void StackDump(lua_State* L);
    const char* Pushnexttemplate(lua_State* L, const char* path);
    const FilePath Findfile(lua_State* L, const char* name, const char* pname);
#endif //SWIG

    // autotesting system api
    void OnError(const String& errorMessage);
    void OnTestFinished();
    void OnTestSkipped();

    size_t GetUsedMemory() const;

    float32 GetTimeElapsed();
    int64 GetMsSinceEpoch();

    // Test organization API
    void OnTestStart(const String& testName);

    void OnStepStart(const String& stepName);

    void Log(const String& level, const String& message);

    // autotesting api
    UIControl* GetScreen();
    UIControl* FindControl(const String& path) const;
    UIControl* FindControl(const String& path, UIControl* screen) const;
    UIControl* FindControlOnPopUp(const String& path) const;
    UIControl* FindControl(UIControl* srcControl, const String& controlName) const;
    UIControl* FindControl(UIControl* srcControl, int32 index) const;
    UIControl* FindControl(UIList* srcList, int32 index) const;

    bool IsCenterInside(UIControl* parent, UIControl* child) const;
    bool IsSelected(UIControl* control) const;

    bool IsListHorisontal(UIControl* control);
    float32 GetListScrollPosition(UIControl* control);
    float32 GetMaxListOffsetSize(UIControl* control);

    Vector2 GetContainerScrollPosition(UIControl* control);
    Vector2 GetMaxContainerOffsetSize(UIControl* control);

    void TouchDown(const Vector2& point, int32 touchId);
    void TouchMove(const Vector2& point, int32 touchId);
    void TouchUp(int32 touchId);

    void LeftMouseClickDown(const Vector2& point);
    void LeftMouseClickUp(const Vector2& point);

    void MouseWheel(const Vector2& point, float32 x, float32 y);

    void ScrollToControl(const String& path) const;

    // Keyboard action
    void KeyPress(int32 keyChar);

    void ProcessInput(const UIEvent& input);
    void ClickSystemBack();
    void EmulatePressKey(DAVA::uint32 key);

    // helpers
    bool SetText(const String& path, const String& text); // lua uses ansi strings
    bool CheckText(UIControl* control, const String& expectedText);
    bool CheckMsgText(UIControl* control, const String& key);
    String GetTaggedClass(UIControl* control, const String& tag);
    String GetText(UIControl* control);
    String GetRichText(UIControl* control);
    uint32 GetTextColor(UIControl* control);

    // multiplayer api
    int32 GetServerQueueState(const String& serverName);
    bool SetServerQueueState(const String& serverName, int32 state);

    String ReadState(const String& device, const String& param);
    void WriteState(const String& device, const String& param, const String& state);

    void InitializeDevice();

    String GetDeviceName();
    String GetPlatform();

    bool IsPhoneScreen();

    // DB storing
    bool SaveKeyedArchiveToDevice(const String& archiveName, KeyedArchive* archive);

    String GetTestParameter(const String& device);

    String MakeScreenshot();
    bool GetIsScreenShotSaving() const;

    bool RunScript(const String& luaScript);
    bool RunScriptFromFile(const FilePath& luaFilePath);

protected:
#if !defined(SWIG)
    inline void ParsePath(const String& path, Vector<String>& parsedPath) const;

    bool LoadScript(const String& luaScript);
    bool LoadScriptFromFile(const FilePath& luaFilePath);
    bool RunScript();

    bool LoadWrappedLuaObjects();

    AutotestingSystemLuaDelegate* delegate;
    lua_State* luaState; //TODO: multiple lua states
    
#endif //SWIG
private:
    void* memoryPool;
    void* memorySpace;
    int resumeTestFunctionRef;
};
};

#endif //__DAVAENGINE_AUTOTESTING__

#endif //__DAVAENGINE_AUTOTESTING_SYSTEM_LUA_H__
