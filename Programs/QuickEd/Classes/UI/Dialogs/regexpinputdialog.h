#ifndef REGEXPINPUTDIALOG_H
#define REGEXPINPUTDIALOG_H
     
#include <QDialog>

class QLabel;
class QLineEdit;
class QDialogButtonBox;
class QRegExpValidator;

class RegExpInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RegExpInputDialog(QWidget* parent = 0, int flags = ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);

    void setTitle(const QString& title);
    void setLabelText(const QString& label);
    void setText(const QString& text);
    void setRegExp(const QRegExp& regExp);

    QString getLabelText();
    QString getText();

    static QString getText(QWidget* parent, const QString& title, const QString& label,
                           const QString& text, const QRegExp& regExp, bool* ok,
                           int flags = ~Qt::WindowMinimizeButtonHint & ~Qt::WindowMaximizeButtonHint);

private slots:
    void checkValid(const QString& text);

private:
    QLabel* label;
    QLineEdit* text;
    QDialogButtonBox* buttonBox;
    QRegExp regExp;
    QRegExpValidator* validator;
};
     
#endif // REGEXPINPUTDIALOG_H
