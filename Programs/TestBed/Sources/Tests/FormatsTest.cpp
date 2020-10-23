#include "Tests/FormatsTest.h"
#include "Render/PixelFormatDescriptor.h"
#include "UI/Render/UIDebugRenderComponent.h"

using namespace DAVA;

namespace FormatsTestInternal
{
void UpdateSupportState(const PixelFormatDescriptor& descriptor, UIStaticText* formatValue)
{
    if (descriptor.isHardwareSupported)
    {
        formatValue->SetText(L"supported");
        formatValue->SetTextColor({ 0, 1.0f, 0, 1.f });
    }
    else
    {
        formatValue->SetText(L"unsupported");
        formatValue->SetTextColor({ 1.f, 0, 0, 1.f });
    }
}

void UpdateFormatInfo(PixelFormat format, UIStaticText* formatName, UIStaticText* formatValue)
{
    if (format == FORMAT_INVALID)
    {
        formatName->SetText(L"Invalid");
        formatValue->SetText(L"Ignore. For developers only");
        formatValue->SetTextColor({ 1.0f, 0.5f, 0.f, 1.0f });
        return;
    }

    if (format == FORMAT_REMOVED_DXT_1N)
    {
        formatName->SetText(L"Removed: DXT_1N");
        formatValue->SetText(L"unsupported");
        formatValue->SetTextColor({ 1.0f, 0.5f, 0.f, 1.0f });
        return;
    }

    const PixelFormatDescriptor& descriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(format);
    formatName->SetText(UTF8Utils::EncodeToWideString(descriptor.name.c_str()));

    if (format == FORMAT_DXT1A)
    {
        UpdateSupportState(PixelFormatDescriptor::GetPixelFormatDescriptor(FORMAT_DXT1), formatValue);
    }
    else if (format == FORMAT_DXT5NM)
    {
        UpdateSupportState(PixelFormatDescriptor::GetPixelFormatDescriptor(FORMAT_DXT5), formatValue);
    }
    else
    {
        UpdateSupportState(descriptor, formatValue);
    }
}
}

FormatsTest::FormatsTest(TestBed& app)
    : BaseScreen(app, "Supported_texture_formats_test")
{
}

void FormatsTest::LoadResources()
{
    BaseScreen::LoadResources();

    DVASSERT(formatsGrid == nullptr);
    const Size2i screenSize = GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    formatsGrid = new UIList(Rect(0, 0, static_cast<DAVA::float32>(screenSize.dx), static_cast<DAVA::float32>(screenSize.dy - 30.f)), UIList::ORIENTATION_VERTICAL);
    formatsGrid->SetDelegate(this);
    AddControl(formatsGrid);
}

void FormatsTest::UnloadResources()
{
    BaseScreen::UnloadResources();
    RemoveAllControls();

    SafeRelease(formatsGrid);
}

float32 FormatsTest::CellHeight(UIList* list, int32 index)
{
    return 30.f;
}

int32 FormatsTest::ElementsCount(UIList* list)
{
    return FORMAT_PVR2_2; //FORMAT_COUNT in future
}

UIListCell* FormatsTest::CellAtIndex(UIList* list, int32 index)
{
    UIStaticText* formatName = nullptr;
    UIStaticText* formatValue = nullptr;

    static const String cellName = "TestButtonCell";
    UIListCell* cell = list->GetReusableCell(cellName); //try to get cell from the reusable cells store
    if (!cell)
    { //if cell of requested type isn't find in the store create new cell
        ScopedPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));
        DVASSERT(font);

        const float32 cellWidth = list->GetSize().x;
        const float32 cellHeight = CellHeight(list, index);
        cell = new UIListCell(Rect(0.f, 0.f, cellWidth, cellHeight), cellName);
        cell->GetOrCreateComponent<UIDebugRenderComponent>();

        const float32 formatNameWidth = Min(cellWidth / 2.0f, 200.0f);
        formatName = new UIStaticText(Rect(0., 0., formatNameWidth, cellHeight));
        formatName->SetFont(font);
        formatName->SetFontSize(16.f);
        formatName->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
        formatName->SetFittingOption(TextBlock::FITTING_REDUCE);
        cell->AddControl(formatName);

        const float32 formatValueWidth = cellWidth - formatNameWidth;
        formatValue = new UIStaticText(Rect(formatNameWidth, 0., formatValueWidth, cellHeight));
        formatValue->SetFont(font);
        formatValue->SetFontSize(16.f);
        formatValue->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
        cell->AddControl(formatValue);
    }

    FormatsTestInternal::UpdateFormatInfo(static_cast<PixelFormat>(index), formatName, formatValue);
    return cell;
}

void FormatsTest::OnCellSelected(UIList* forList, UIListCell* selectedCell)
{
}
