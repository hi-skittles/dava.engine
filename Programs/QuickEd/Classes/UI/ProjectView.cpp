#include "ProjectView.h"
#include "ui_mainwindow.h"
#include "Modules/LegacySupportModule/Private/Project.h"

#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>

MainWindow::ProjectView::ProjectView(MainWindow* mainWindow_)
    : QObject(mainWindow_)
    , mainWindow(mainWindow_)
{
    InitPluginsToolBar();
    SetProjectActionsEnabled(false);
}

void MainWindow::ProjectView::SetLanguages(const QStringList& availableLangsCodes, const QString& currentLangCode)
{
    bool wasBlocked = comboboxLanguage->blockSignals(true); //performance fix
    comboboxLanguage->clear();

    if (!availableLangsCodes.isEmpty())
    {
        bool currentLangPresent = false;
        for (const QString& langCode : availableLangsCodes)
        {
            comboboxLanguage->addItem(ConvertLangCodeToString(langCode), langCode);
            if (langCode == currentLangCode)
            {
                currentLangPresent = true;
            }
        }
        DVASSERT(currentLangPresent);
        comboboxLanguage->setCurrentText(ConvertLangCodeToString(currentLangCode));
    }

    comboboxLanguage->blockSignals(wasBlocked);
}

void MainWindow::ProjectView::SetCurrentLanguage(const QString& currentLangCode)
{
    comboboxLanguage->setCurrentText(ConvertLangCodeToString(currentLangCode));
}

void MainWindow::ProjectView::SetProjectActionsEnabled(bool enabled)
{
    mainWindow->ui->toolBarGlobal->setEnabled(enabled);
}

void MainWindow::ProjectView::InitPluginsToolBar()
{
    InitLanguageBox();
    InitGlobalClasses();
    InitRtlBox();
    InitBiDiSupportBox();
}

void MainWindow::ProjectView::InitLanguageBox()
{
    comboboxLanguage = new QComboBox(mainWindow);
    comboboxLanguage->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    QLabel* label = new QLabel(tr("Language"));
    label->setBuddy(comboboxLanguage);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(label);
    layout->addWidget(comboboxLanguage);
    QWidget* wrapper = new QWidget();
    wrapper->setLayout(layout);
    mainWindow->ui->toolBarGlobal->addSeparator();
    mainWindow->ui->toolBarGlobal->addWidget(wrapper);

    void (QComboBox::*currentIndexChangedFn)(int) = &QComboBox::currentIndexChanged;
    connect(comboboxLanguage, currentIndexChangedFn, this, &MainWindow::ProjectView::OnCurrentLanguageChanged);
}

void MainWindow::ProjectView::InitRtlBox()
{
    QCheckBox* rtlBox = new QCheckBox(tr("Right-to-left"));
    rtlBox->setLayoutDirection(Qt::RightToLeft);
    mainWindow->ui->toolBarGlobal->addSeparator();
    mainWindow->ui->toolBarGlobal->addWidget(rtlBox);
    connect(rtlBox, &QCheckBox::stateChanged, this, &MainWindow::ProjectView::OnRtlChanged);
}

void MainWindow::ProjectView::InitBiDiSupportBox()
{
    QCheckBox* bidiSupportBox = new QCheckBox(tr("BiDi Support"));
    bidiSupportBox->setLayoutDirection(Qt::RightToLeft);
    mainWindow->ui->toolBarGlobal->addSeparator();
    mainWindow->ui->toolBarGlobal->addWidget(bidiSupportBox);
    connect(bidiSupportBox, &QCheckBox::stateChanged, this, &MainWindow::ProjectView::OnBiDiSupportChanged);
}

void MainWindow::ProjectView::InitGlobalClasses()
{
    QLineEdit* classesEdit = new QLineEdit();
    classesEdit->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    QLabel* label = new QLabel(tr("Global classes"));
    label->setBuddy(classesEdit);
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(label);
    layout->addWidget(classesEdit);
    QWidget* wrapper = new QWidget();
    wrapper->setLayout(layout);
    mainWindow->ui->toolBarGlobal->addSeparator();
    mainWindow->ui->toolBarGlobal->addWidget(wrapper);
    connect(classesEdit, &QLineEdit::textChanged, this, &MainWindow::ProjectView::OnGlobalClassesChanged);
}

void MainWindow::ProjectView::SetProjectPath(const QString& projectPath)
{
    mainWindow->SetProjectPath(projectPath);
}

void MainWindow::ProjectView::OnProjectChanged(Project* project)
{
    emit ProjectChanged(project);
}

void MainWindow::ProjectView::OnRtlChanged(int arg)
{
    emit RtlChanged(arg == Qt::Checked);
}

void MainWindow::ProjectView::OnBiDiSupportChanged(int arg)
{
    emit BiDiSupportChanged(arg == Qt::Checked);
}

void MainWindow::ProjectView::OnGlobalClassesChanged(const QString& str)
{
    emit GlobalStyleClassesChanged(str);
}

void MainWindow::ProjectView::OnCurrentLanguageChanged(int newLanguageIndex)
{
    QString langCode = comboboxLanguage->itemData(newLanguageIndex).toString();
    emit CurrentLanguageChanged(langCode);
}

QString MainWindow::ProjectView::ConvertLangCodeToString(const QString& langCode)
{
    QLocale locale(langCode);
    switch (locale.script())
    {
    case QLocale::SimplifiedChineseScript:
    {
        return "Chinese simpl.";
    }

    case QLocale::TraditionalChineseScript:
    {
        return "Chinese trad.";
    }

    default:
        return QLocale::languageToString(locale.language());
    }
}
