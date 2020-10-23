#include "PathValidator.h"
#include <QMessageBox>

PathValidator::PathValidator(const QStringList& value)
    : RegExpValidator("")
    , referencePathList(value)
{
    QString regExpr("^$|");
    foreach (QString path, referencePathList)
    {
        regExpr += "^" + path + ".*|";
    }
    SetRegularExpression(regExpr);
}

void PathValidator::ErrorNotifyInternal(const QVariant& v) const
{
    QMessageBox::warning(NULL, "Wrong file selected", PrepareErrorMessage(v).c_str(), QMessageBox::Ok);
}

DAVA::String PathValidator::PrepareErrorMessage(const QVariant& v) const
{
    QString referencePaths;
    foreach (QString path, referencePathList)
    {
        referencePaths += path + " ";
    }
    return DAVA::Format("\"%s\" is wrong. It's allowed to select only from %s", v.toString().toStdString().c_str(), referencePaths.toStdString().c_str());
}
