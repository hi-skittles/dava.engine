#pragma once

class GuideLabel;
class QWidget;

struct Guide
{
    void Show();
    void Raise();

    void Hide();

    QWidget* line = nullptr;
    GuideLabel* text = nullptr;

    bool inWork = false;
};
