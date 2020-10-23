#ifndef MULTILINEEDITOR_H
#define MULTILINEEDITOR_H


#include <QWidget>
#include <QScopedPointer>

namespace Ui
{
class MultilineEditor;
} // namespace Ui

class MultilineEditor
: public QWidget
{
    Q_OBJECT

signals:
    void done();

public:
    explicit MultilineEditor(QWidget* parent = nullptr);
    ~MultilineEditor();

    void SetText(const QString& text);
    QString GetText() const;

    bool IsAccepted() const;

private:
    void closeEvent(QCloseEvent* e) override;

    QScopedPointer<Ui::MultilineEditor> ui;
    bool isAccepted;
};


#endif // MULTILINEEDITOR_H
