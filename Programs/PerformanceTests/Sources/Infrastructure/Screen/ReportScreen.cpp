#include "ReportScreen.h"

bool ReportScreen::IsFinished() const
{
    return false;
}

void ReportScreen::LoadResources()
{
    BaseScreen::LoadResources();

    CreateReportScreen();
}

void ReportScreen::CreateReportScreen()
{
    UIYamlLoader::LoadFonts("~res:/UI/Fonts/fonts.yaml");

    DefaultUIPackageBuilder builder;
    UIPackageLoader().LoadPackage(ControlHelpers::GetPathToUIYaml("ReportItem.yaml"), &builder);
    DAVA::UIControl* reportItem = builder.GetPackage()->GetControl("ReportItem");

    uint32 offsetY = 150;
    uint32 testNumber = 0;

    for (auto* test : testChain)
    {
        if (test->IsFinished())
        {
            const auto& framesInfo = test->GetFramesInfo();

            float32 minDelta = std::numeric_limits<float>::max();
            float32 maxDelta = std::numeric_limits<float>::min();
            float32 averageDelta = 0.0f;

            float32 testTime = 0.0f;
            float32 elapsedTime = 0.0f;

            uint32 framesCount = static_cast<uint32>(framesInfo.size());

            for (const auto& frameInfo : framesInfo)
            {
                if (frameInfo.delta > maxDelta)
                {
                    maxDelta = frameInfo.delta;
                }
                if (frameInfo.delta < minDelta)
                {
                    minDelta = frameInfo.delta;
                }

                averageDelta += frameInfo.delta;
            }

            averageDelta /= framesCount;

            testTime = test->GetOverallTestTime();
            elapsedTime = test->GetElapsedTime() / 1000.0f;

            ScopedPtr<UIControl> reportItemCopy(reportItem->Clone());
            reportItemCopy->SetPosition(Vector2(0.0f, 0.0f + testNumber * offsetY));

            UIStaticText* testName = reportItemCopy->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::TEST_NAME_PATH);
            testName->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%s", test->GetSceneName().c_str())));

            UIStaticText* minDeltaText = reportItemCopy->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::MIN_DELTA_PATH);
            minDeltaText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", minDelta)));

            UIStaticText* maxDeltaText = reportItemCopy->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::MAX_DELTA_PATH);
            maxDeltaText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", maxDelta)));

            UIStaticText* averageDeltaText = reportItemCopy->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::AVERAGE_DELTA_PATH);
            averageDeltaText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", averageDelta)));

            UIStaticText* testTimeText = reportItemCopy->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::TEST_TIME_PATH);
            testTimeText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", testTime)));

            UIStaticText* elapsedTimeText = reportItemCopy->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::ELAPSED_TIME_PATH);
            elapsedTimeText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", elapsedTime)));

            UIStaticText* framesRenderedText = reportItemCopy->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::FRAMES_RENDERED_PATH);
            framesRenderedText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%d", framesCount)));

            AddControl(reportItemCopy);

            testNumber++;
        }
    }
}
