#include "MultilineEditor.h"


#include "ui_MultilineEditor.h"

MultilineEditor::MultilineEditor(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::MultilineEditor())
    , isAccepted(false)
{
    ui->setupUi(this);

    connect(ui->ok, &QPushButton::clicked, this,
            [this]()
            {
                isAccepted = true;
            });

    connect(ui->ok, &QPushButton::clicked, this, &MultilineEditor::close);
    connect(ui->cancel, &QPushButton::clicked, this, &QWidget::close);
}

MultilineEditor::~MultilineEditor()
{
}

void MultilineEditor::SetText(const QString& text)
{
    ui->editor->setPlainText(text);
}

QString MultilineEditor::GetText() const
{
    return ui->editor->toPlainText();
}

bool MultilineEditor::IsAccepted() const
{
    return isAccepted;
}

void MultilineEditor::closeEvent(QCloseEvent* e)
{
    emit done();
    QWidget::closeEvent(e);
}
