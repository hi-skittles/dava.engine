#include "Classes/Library/Private/ScenePreviewDialog.h"
#include "Classes/Library/Private/ControlsFactory.h"
#include "Classes/Library/Private/ScenePreviewControl.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Application/REGlobal.h"

ScenePreviewDialog::ScenePreviewDialog()
    : ExtendedDialog()
    , preview(nullptr)
    , errorMessage(nullptr)
    , clickableBackgound(nullptr)
{
    UpdateSize();
    DAVA::UIControlBackground* bg = GetOrCreateComponent<DAVA::UIControlBackground>();
    bg->color = DAVA::Color(2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f, 1.0f);

    clickableBackgound.reset(new DAVA::UIControl());
    clickableBackgound->SetInputEnabled(true, true);
    clickableBackgound->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &ScenePreviewDialog::OnClose));

    preview.reset(new ScenePreviewControl(DAVA::Rect(0, 0, ControlsFactory::PREVIEW_PANEL_HEIGHT, ControlsFactory::PREVIEW_PANEL_HEIGHT)));

    errorMessage.reset(new DAVA::UIStaticText(preview->GetRect()));
    DAVA::UIControlBackground* errorMessageBg = errorMessage->GetOrCreateComponent<DAVA::UIControlBackground>();
    errorMessageBg->SetAlign(DAVA::ALIGN_HCENTER | DAVA::ALIGN_VCENTER);
    errorMessage->SetMultiline(true);
    errorMessage->SetTextColor(ControlsFactory::GetColorError());
    errorMessage->SetFont(ControlsFactory::GetFont20());

    DAVA::ScopedPtr<DAVA::UIButton> button(ControlsFactory::CreateButton(DAVA::Rect(0, ControlsFactory::PREVIEW_PANEL_HEIGHT,
                                                                                    ControlsFactory::PREVIEW_PANEL_HEIGHT, ControlsFactory::BUTTON_HEIGHT),
                                                                         DAVA::LocalizedWideString("dialog.close")));
    button->AddEvent(DAVA::UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &ScenePreviewDialog::OnClose));
    draggableDialog->AddControl(button);
}

ScenePreviewDialog::~ScenePreviewDialog()
{
    if (IsShown())
    {
        Close();
    }
}

void ScenePreviewDialog::Show(const DAVA::FilePath& scenePathname)
{
    GeneralSettings* settings = REGlobal::GetGlobalContext()->GetData<GeneralSettings>();
    if (settings->previewEnabled == false)
        return;

    if (!GetParent())
    {
        DAVA::UIScreen* screen = DAVA::UIScreenManager::Instance()->GetScreen();
        clickableBackgound->SetSize(screen->GetSize());
        clickableBackgound->SetPosition(DAVA::Vector2(0, 0));
        screen->AddControl(clickableBackgound);
        screen->AddControl(this);

        screen->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, DAVA::Message(this, &ScenePreviewDialog::OnClose));
    }

    //show preview
    if (errorMessage->GetParent())
    {
        draggableDialog->RemoveControl(errorMessage);
    }

    DAVA::int32 error = preview->OpenScene(scenePathname);
    if (DAVA::SceneFileV2::ERROR_NO_ERROR == error)
    {
        if (!preview->GetParent())
        {
            draggableDialog->AddControl(preview);
        }
    }
    else
    {
        switch (error)
        {
        case DAVA::SceneFileV2::ERROR_FAILED_TO_CREATE_FILE:
        {
            errorMessage->SetText(DAVA::LocalizedWideString("library.errormessage.failedtocreeatefile"));
            break;
        }

        case DAVA::SceneFileV2::ERROR_FILE_WRITE_ERROR:
        {
            errorMessage->SetText(DAVA::LocalizedWideString("library.errormessage.filewriteerror"));
            break;
        }

        case DAVA::SceneFileV2::ERROR_VERSION_IS_TOO_OLD:
        {
            errorMessage->SetText(DAVA::LocalizedWideString("library.errormessage.versionistooold"));
            break;
        }

        case ScenePreviewControl::ERROR_CANNOT_OPEN_FILE:
        {
            errorMessage->SetText(DAVA::LocalizedWideString("library.errormessage.cannotopenfile"));
            break;
        }

        case ScenePreviewControl::ERROR_WRONG_EXTENSION:
        {
            errorMessage->SetText(DAVA::LocalizedWideString("library.errormessage.wrongextension"));
            break;
        }

        default:
            errorMessage->SetText(DAVA::LocalizedWideString("library.errormessage.unknownerror"));
            break;
        }

        draggableDialog->AddControl(errorMessage);
    }
    ExtendedDialog::Show();
}

void ScenePreviewDialog::OnClose(BaseObject*, void*, void*)
{
    Close();
}

void ScenePreviewDialog::Close()
{
    UIControl* backgourndParent = clickableBackgound->GetParent();
    if (backgourndParent != nullptr)
    {
        backgourndParent->RemoveControl(clickableBackgound);
    }

    preview->ReleaseScene();
    preview->RecreateScene();
    draggableDialog->RemoveControl(preview);
    ExtendedDialog::Close();
}

const DAVA::Rect ScenePreviewDialog::GetDialogRect() const
{
    DAVA::Rect screenRect = GetScreenRect();

    DAVA::float32 x = (screenRect.dx - ControlsFactory::PREVIEW_PANEL_HEIGHT);
    DAVA::float32 h = ControlsFactory::PREVIEW_PANEL_HEIGHT + ControlsFactory::BUTTON_HEIGHT;
    DAVA::float32 y = (screenRect.dy - h) / 2;

    return DAVA::Rect(x, y, ControlsFactory::PREVIEW_PANEL_HEIGHT, h);
}

void ScenePreviewDialog::UpdateSize()
{
    DAVA::Rect dialogRect = GetDialogRect();
    SetRect(dialogRect);

    dialogRect.x = dialogRect.y = 0;
    draggableDialog->SetRect(dialogRect);
}
