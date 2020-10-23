#pragma once

#include <TArc/Core/BaseApplication.h>
#include <TArc/Core/FieldBinder.h>
#include <FileSystem/KeyedArchive.h>
#include <Base/RefPtr.h>

#include <QString>

namespace DAVA
{
class Core;
class SceneValidator;
class EditorConfig;
class SettingsManager;
}

class REApplication : public DAVA::BaseApplication
{
public:
    REApplication(DAVA::Vector<DAVA::String>&& cmdLine);
    // destructor will never call on some platforms,
    // so release all resources in Cleanup method.
    ~REApplication() = default;

protected:
    EngineInitInfo GetInitInfo() const override;
    void CreateModules(DAVA::Core* tarcCore) const override;

private:
    void CreateGUIModules(DAVA::Core* tarcCore) const;
    void CreateConsoleModules(DAVA::Core* tarcCore) const;
    void Init(const DAVA::EngineContext* engineContext) override;
    void Init(DAVA::Core* tarcCore) override;
    void Cleanup() override;

    void RegisterEditorAnyCasts() override;
    bool AllowMultipleInstances() const override;
    QString GetInstanceKey() const override;

    DAVA::KeyedArchive* CreateOptions() const;

    bool isConsoleMode = false;
    DAVA::Vector<DAVA::String> cmdLine;
    mutable std::unique_ptr<DAVA::FieldBinder> renderBackEndListener;

private:
    // singletons. In future we probably will try to move them into special module, or completely decompose
    mutable DAVA::RefPtr<DAVA::KeyedArchive> appOptions;
};
