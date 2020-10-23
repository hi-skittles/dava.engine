#pragma once

#include "Classes/Library/Private/ExtendedDialog.h"
#include "Classes/Library/Private/ScenePreviewControl.h"

#include "DAVAEngine.h"

class ScenePreviewDialog : public ExtendedDialog
{
public:
    ScenePreviewDialog();
    virtual ~ScenePreviewDialog();

    void Show(const DAVA::FilePath& scenePathname);
    void Close() override;

protected:
    const DAVA::Rect GetDialogRect() const override;
    void UpdateSize() override;

    void OnClose(BaseObject*, void*, void*);

    DAVA::ScopedPtr<ScenePreviewControl> preview;
    DAVA::ScopedPtr<DAVA::UIStaticText> errorMessage;
    DAVA::ScopedPtr<DAVA::UIControl> clickableBackgound;
};
