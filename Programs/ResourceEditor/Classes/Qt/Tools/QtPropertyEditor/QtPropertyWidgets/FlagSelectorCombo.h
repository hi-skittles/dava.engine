#ifndef __PROPERTY_FLAG_SELECTOR_COMBO_H__
#define __PROPERTY_FLAG_SELECTOR_COMBO_H__


#include <QObject>
#include <QComboBox>
#include <QPointer>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QListView>

class EnumMap;
class QStandardItem;

class FlagSelectorCombo
: public QComboBox
{
    Q_OBJECT

private:
    enum ItemRoles
    {
        ValueRole = Qt::UserRole + 1,
    };

signals:
    void done(quint64 flags);

public:
    FlagSelectorCombo(QWidget* parent = NULL);
    ~FlagSelectorCombo();

    void AddFlagItem(const quint64 value, const QString& hint);
    void SetFlags(const quint64 flags);
    quint64 GetFlags() const;

private slots:
    void onItemChanged(QStandardItem* item);
    void updateText();

private:
    bool eventFilter(QObject* obj, QEvent* e);
    void paintEvent(QPaintEvent* event);

    QString text;
    quint64 extraMask;
};


#endif // __PROPERTY_FLAG_SELECTOR_COMBO_H__
