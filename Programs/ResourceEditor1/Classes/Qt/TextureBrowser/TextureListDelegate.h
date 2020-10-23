#ifndef __TEXTURE_LIST_DELEGATE_H__
#define __TEXTURE_LIST_DELEGATE_H__

#include <QAbstractItemDelegate>
#include <QMap>
#include <QFont>
#include <QFontMetrics>

#include "TextureInfo.h"
#include "DAVAEngine.h"

class QPainter;

class TextureListDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    enum DrawRule
    {
        DRAW_PREVIEW_BIG,
        DRAW_PREVIEW_SMALL
    };

    TextureListDelegate(QObject* parent = 0);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setDrawRule(DrawRule rule);

    bool helpEvent(QHelpEvent* event,
                   QAbstractItemView* view,
                   const QStyleOptionViewItem& option,
                   const QModelIndex& index) override;

signals:
    void textureDescriptorChanged(DAVA::TextureDescriptor* descriptor);
    void textureDescriptorReload(DAVA::TextureDescriptor* descriptor);

protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private slots:
    void textureReadyThumbnail(const DAVA::TextureDescriptor* descriptor, const TextureInfo& images);
    void onOpenTexturePath();
    void onReloadTexture();

    void onLoadPreset();
    void onSavePreset();

private:
    QFont nameFont;
    QFontMetrics nameFontMetrics;
    mutable QMap<DAVA::FilePath, QModelIndex> descriptorIndexes;
    DAVA::TextureDescriptor* lastSelectedTextureDescriptor = nullptr;

    DrawRule drawRule;

    void drawPreviewBig(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void drawPreviewSmall(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    int drawFormatInfo(QPainter* painter, QRect rect, const DAVA::Texture* texture, const DAVA::TextureDescriptor* descriptor) const;

    QString CreateInfoString(const QModelIndex& index) const;
};

#endif // __TEXTURE_LIST_DELEGATE_H__
