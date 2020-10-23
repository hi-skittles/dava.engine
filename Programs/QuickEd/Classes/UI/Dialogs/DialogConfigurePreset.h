#pragma once

#include <QDialog>
#include <memory>

class EditorFontSystem;
namespace DAVA
{
class Font;
}
namespace Ui
{
class DialogConfigurePreset;
}

class DialogConfigurePreset : public QDialog
{
    Q_OBJECT

public:
    explicit DialogConfigurePreset(EditorFontSystem* editorFontSystem, const QString& originalPresetName, QWidget* parent = nullptr);
    ~DialogConfigurePreset();
private slots:
    void initPreset();
    void OnDefaultFontChanged(const QString& arg);
    void OnDefaultFontSizeChanged(int size);
    void OnLocalizedFontChanged(const QString& arg);
    void OnLocalizedFontSizeChanged(int size);

    void OnCurrentLocaleChanged(const QString& arg);
    void OnResetLocale();
    void OnApplyToAllLocales();
    void OnOk();
    void OnCancel();

private:
    void UpdateDefaultFontWidgets();
    void UpdateLocalizedFontWidgets();
    void SetFont(const QString& font, const int fontSize, const QString& locale);

    std::unique_ptr<Ui::DialogConfigurePreset> ui;
    const QString originalPresetName;
    EditorFontSystem* editorFontSystem = nullptr;
};
