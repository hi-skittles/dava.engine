#pragma once

#include "Base/Any.h"
#include "Debug/DVAssert.h"

class SceneEditor2;
class QWidget;
class GlobalOperations
{
public:
    virtual ~GlobalOperations() = default;

    enum ID
    {
        OpenScene, // args - scenePath: DAVA::String
        ShowMaterial, // args - material::DAVA::NMaterial*
        ReloadTexture, // args - empty
    };
    virtual void CallAction(ID id, DAVA::Any&& args) = 0;
    virtual QWidget* GetGlobalParentWidget() const = 0;

    virtual void ShowWaitDialog(const DAVA::String& tittle, const DAVA::String& message, DAVA::uint32 min = 0, DAVA::uint32 max = 100) = 0;
    virtual void HideWaitDialog() = 0;

    virtual void ForEachScene(const DAVA::Function<void(SceneEditor2*)>& functor) = 0;
};

class WaitDialogGuard
{
public:
    WaitDialogGuard(std::shared_ptr<GlobalOperations> globalOperations_, const DAVA::String& tittle,
                    const DAVA::String& message, DAVA::uint32 min, DAVA::uint32 max)
        : globalOperations(globalOperations_)
    {
        DVASSERT(globalOperations != nullptr);
        globalOperations->ShowWaitDialog(tittle, message, min, max);
    }

    ~WaitDialogGuard()
    {
        globalOperations->HideWaitDialog();
    }

private:
    std::shared_ptr<GlobalOperations> globalOperations;
};
