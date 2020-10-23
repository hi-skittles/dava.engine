#include "Classes/Qt/TextureBrowser/TextureListDelegate.h"
#include "Classes/Qt/TextureBrowser/TextureListModel.h"
#include "Classes/Qt/TextureBrowser/TextureCache.h"
#include "Classes/Qt/TextureBrowser/TextureConvertor.h"
#include "Classes/Qt/TextureBrowser/TextureBrowser.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Scene/Utils/Preset.h>

#include <TArc/Core/Deprecated.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/SharedModules/ThemesModule/ThemesModule.h>
#include <TArc/Utils/Utils.h>

#include <QtHelpers/HelperFunctions.h>

#include <Base/BaseTypes.h>

#include <QPainter>
#include <QFileInfo>
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QToolTip>
#include <QAbstractItemView>
#include <QDesktopServices>
#include "REPlatform/Scene/Utils/Utils.h"

#define TEXTURE_PREVIEW_SIZE 80
#define TEXTURE_PREVIEW_SIZE_SMALL 24
#define BORDER_MARGIN 1
#define BORDER_COLOR QColor(0, 0, 0, 25)
#define SELECTION_BORDER_COLOR_MACRO QColor(0, 0, 0, 50)
#define INFO_TEXT_COLOR QColor(0, 0, 0, 100)
#define FORMAT_INFO_WIDTH 3
#define FORMAT_INFO_SPACING 1

namespace ActionIcon
{
QIcon loadPresetIcon;
QIcon savePresetIcon;
}

TextureListDelegate::TextureListDelegate(QObject* parent /* = 0 */)
    : QAbstractItemDelegate(parent)
    , nameFont("Arial", 10, QFont::Bold)
    , nameFontMetrics(nameFont)
    , drawRule(DRAW_PREVIEW_BIG)
{
    QObject::connect(TextureCache::Instance(), SIGNAL(ThumbnailLoaded(const DAVA::TextureDescriptor*, const TextureInfo&)), this, SLOT(textureReadyThumbnail(const DAVA::TextureDescriptor*, const TextureInfo&)));

    if (ActionIcon::loadPresetIcon.isNull())
    {
        ActionIcon::loadPresetIcon.addFile(QStringLiteral(":/QtIcons/openscene.png"), QSize(), QIcon::Normal, QIcon::Off);
    }
    if (ActionIcon::savePresetIcon.isNull())
    {
        ActionIcon::savePresetIcon.addFile(QStringLiteral(":/QtIcons/save_as.png"), QSize(), QIcon::Normal, QIcon::Off);
    }
};

void TextureListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    switch (drawRule)
    {
    case DRAW_PREVIEW_SMALL:
        drawPreviewSmall(painter, option, index);
        break;

    case DRAW_PREVIEW_BIG:
    default:
        drawPreviewBig(painter, option, index);
        break;
    }
}

QSize TextureListDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    switch (drawRule)
    {
    case DRAW_PREVIEW_SMALL:
        return QSize(option.rect.x(), TEXTURE_PREVIEW_SIZE_SMALL);

    case DRAW_PREVIEW_BIG:
    default:
        return QSize(option.rect.x(), TEXTURE_PREVIEW_SIZE);
    }
}

void TextureListDelegate::textureReadyThumbnail(const DAVA::TextureDescriptor* descriptor, const TextureInfo& images)
{
    if (nullptr != descriptor)
    {
        if (descriptorIndexes.contains(descriptor->pathname))
        {
            QModelIndex index = descriptorIndexes[descriptor->pathname];
            descriptorIndexes.remove(descriptor->pathname);

            // this will force item with given index to redraw
            emit sizeHintChanged(index);
        }
    }
}

void TextureListDelegate::setDrawRule(DrawRule rule)
{
    drawRule = rule;
}

void TextureListDelegate::drawPreviewBig(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const TextureListModel* curModel = (TextureListModel*)index.model();
    DAVA::TextureDescriptor* curTextureDescriptor = curModel->getDescriptor(index);

    if (nullptr != curTextureDescriptor)
    {
        DAVA::Texture* curTexture = curModel->getTexture(index);

        QString textureName = QString::fromStdString(curTextureDescriptor->GetSourceTexturePathname().GetFilename());

        painter->save();
        painter->setClipRect(option.rect);

        // draw border
        QRect borderRect = option.rect;
        borderRect.adjust(BORDER_MARGIN, BORDER_MARGIN, -BORDER_MARGIN, -BORDER_MARGIN);
        painter->setPen(BORDER_COLOR);
        painter->drawRect(borderRect);

        // draw selected item
        if (option.state & QStyle::State_Selected)
        {
            painter->setBrush(option.palette.highlight());
            painter->setPen(SELECTION_BORDER_COLOR_MACRO);
            painter->drawRect(borderRect);
        }

        const QList<QImage>& images = TextureCache::Instance()->getThumbnail(curTextureDescriptor);
        if (images.size() > 0 &&
            !images[0].isNull())
        {
            QSize imageSize = images[0].rect().size();
            int imageX = option.rect.x() + (TEXTURE_PREVIEW_SIZE - imageSize.width()) / 2;
            int imageY = option.rect.y() + (TEXTURE_PREVIEW_SIZE - imageSize.height()) / 2;
            painter->drawImage(QRect(QPoint(imageX, imageY), imageSize), images[0]);
        }
        else
        {
            // there is no image for this texture in cache
            // so load it async
            TextureConvertor::Instance()->GetThumbnail(curTextureDescriptor);
            descriptorIndexes.insert(curTextureDescriptor->pathname, index);
        }

        // draw formats info
        int infoLen = drawFormatInfo(painter, borderRect, curTexture, curTextureDescriptor);

        if (curModel->isHighlited(index) && !option.state.testFlag(QStyle::State_Selected))
        {
            // draw highlight
            QColor color = DAVA::Deprecated::GetDataNode<DAVA::ThemesSettings>()->GetHighligtedItemTextColor();
            painter->setPen(color);
        }
        else
        {
            painter->setPen(option.palette.text().color());
        }

        // draw text info
        {
            QRectF textRect = option.rect;
            textRect.adjust(TEXTURE_PREVIEW_SIZE, option.decorationSize.height() / 2, -infoLen, 0);

            QFont origFont = painter->font();
            painter->setFont(nameFont);
            painter->drawText(textRect, textureName);

            painter->setFont(origFont);
            textRect.adjust(0, nameFontMetrics.height(), 0, 0);

            QString infoText = CreateInfoString(index);
            painter->drawText(textRect, infoText);
        }

        painter->restore();
    }
}

bool TextureListDelegate::helpEvent(QHelpEvent* event,
                                    QAbstractItemView* view,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index)
{
    if (nullptr == event || nullptr == view)
    {
        return false;
    }

    if (event->type() == QEvent::ToolTip)
    {
        auto tooltip = index.data(Qt::DisplayRole);
        if (tooltip.canConvert<QString>())
        {
            QString infoText = CreateInfoString(index);
            if (!infoText.isEmpty())
            {
                QToolTip::showText(event->globalPos(), infoText, view);
                return true;
            }
        }
    }

    return QAbstractItemDelegate::helpEvent(event, view, option, index);
}

QString TextureListDelegate::CreateInfoString(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QString();
    }

    auto curModel = static_cast<const TextureListModel*>(index.model());
    auto curTextureDescriptor = curModel->getDescriptor(index);

    if (nullptr != curTextureDescriptor)
    {
        auto curTexture = curModel->getTexture(index);
        if (nullptr != curTexture)
        {
            QString infoText;
            char dimen[64];

            QSize textureDimension;
            if (curTexture->IsPinkPlaceholder())
            {
                DAVA::ImageInfo imgInfo = DAVA::ImageSystem::GetImageInfo(curTextureDescriptor->GetSourceTexturePathname());
                textureDimension = QSize(imgInfo.width, imgInfo.height);
            }
            else
            {
                textureDimension = QSize(curTexture->width, curTexture->height);
            }

            sprintf(dimen, "Size: %dx%d", textureDimension.width(), textureDimension.height());
            infoText += dimen;
            infoText += "\nData size: ";
            infoText += QString::fromStdString(DAVA::SizeInBytesToString(TextureCache::Instance()->getThumbnailSize(curTextureDescriptor)));

            DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
            if (data)
            {
                DAVA::FilePath dataSourcePath = data->GetDataSource3DPath();

                infoText += "\nPath: ";
                infoText += curTextureDescriptor->pathname.GetRelativePathname(dataSourcePath).c_str();
            }

            return infoText;
        }
    }

    return QString();
}

void TextureListDelegate::drawPreviewSmall(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const TextureListModel* curModel = (TextureListModel*)index.model();
    DAVA::TextureDescriptor* curTextureDescriptor = curModel->getDescriptor(index);
    DAVA::Texture* curTexture = curModel->getTexture(index);

    if (nullptr != curTextureDescriptor)
    {
        painter->save();
        painter->setClipRect(option.rect);

        // draw border
        QRect borderRect = option.rect;
        borderRect.adjust(BORDER_MARGIN, BORDER_MARGIN, -BORDER_MARGIN, -BORDER_MARGIN);
        painter->setPen(BORDER_COLOR);
        painter->drawRect(borderRect);

        // draw selected item
        if (option.state & QStyle::State_Selected)
        {
            painter->setBrush(option.palette.highlight());
            painter->setPen(SELECTION_BORDER_COLOR_MACRO);
            painter->drawRect(borderRect);
        }

        // draw preview
        QRect previewRect = borderRect;
        previewRect.adjust(BORDER_MARGIN, BORDER_MARGIN, -BORDER_MARGIN * 2, -BORDER_MARGIN * 2);
        previewRect.setWidth(previewRect.height());
        painter->setBrush(QBrush(QColor(100, 100, 100)));
        painter->drawRect(previewRect);

        // draw formats info
        int infoLen = drawFormatInfo(painter, borderRect, curTexture, curTextureDescriptor);

        // draw text
        QRectF textRect = option.rect;
        textRect.adjust(TEXTURE_PREVIEW_SIZE_SMALL, (option.rect.height() - option.fontMetrics.height()) / 2, -infoLen, 0);

        if (curModel->isHighlited(index) && !option.state.testFlag(QStyle::State_Selected))
        {
            // draw highlight
            QColor color = DAVA::Deprecated::GetDataNode<DAVA::ThemesSettings>()->GetHighligtedItemTextColor();
            painter->setPen(color);
        }
        else
        {
            painter->setPen(option.palette.text().color());
        }

        painter->drawText(textRect, curModel->data(index).toString());

        painter->restore();
    }
}

int TextureListDelegate::drawFormatInfo(QPainter* painter, QRect rect, const DAVA::Texture* texture, const DAVA::TextureDescriptor* descriptor) const
{
    int ret = 0;
    QRect r = rect;

    if (nullptr != descriptor && nullptr != texture)
    {
        r.adjust(FORMAT_INFO_SPACING, FORMAT_INFO_SPACING, -FORMAT_INFO_SPACING, -FORMAT_INFO_SPACING);
        r.setX(rect.x() + rect.width());
        r.setWidth(FORMAT_INFO_WIDTH);

        QColor gpuInfoColors[DAVA::GPU_DEVICE_COUNT];
        gpuInfoColors[DAVA::GPU_POWERVR_IOS] = TextureBrowser::gpuColor_PVR_ISO;
        gpuInfoColors[DAVA::GPU_POWERVR_ANDROID] = TextureBrowser::gpuColor_PVR_Android;
        gpuInfoColors[DAVA::GPU_TEGRA] = TextureBrowser::gpuColor_Tegra;
        gpuInfoColors[DAVA::GPU_MALI] = TextureBrowser::gpuColor_MALI;
        gpuInfoColors[DAVA::GPU_ADRENO] = TextureBrowser::gpuColor_Adreno;
        gpuInfoColors[DAVA::GPU_DX11] = TextureBrowser::gpuColor_DX11;

        // format lines
        for (int i = (DAVA::GPU_DEVICE_COUNT - 1); i >= 0; --i)
        {
            r.moveLeft(r.x() - FORMAT_INFO_WIDTH);

            if (descriptor->compression[i].format != DAVA::FORMAT_INVALID)
            {
                QColor c = gpuInfoColors[i];

                painter->setPen(Qt::NoPen);
                painter->setBrush(c);
                painter->drawRect(r);
            }

            r.moveLeft(r.x() - FORMAT_INFO_SPACING);
        }

        // error icon
        if (texture->width != texture->height)
        {
            r.moveLeft(r.x() - 16);
            DAVA::SharedIcon(":/QtIcons/error.png").paint(painter, r.x(), r.y(), 16, 16);
        }

        ret = rect.width() - (r.x() - rect.x());
    }

    return ret;
}

bool TextureListDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    switch (event->type())
    {
    case QEvent::MouseButtonRelease:
    {
        if (!index.isValid())
        {
            break;
        }

        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::RightButton)
        {
            break;
        }

        const TextureListModel* curModel = qobject_cast<const TextureListModel*>(index.model());
        DVASSERT(curModel);
        lastSelectedTextureDescriptor = curModel->getDescriptor(index);
        if (lastSelectedTextureDescriptor == nullptr)
        {
            break;
        }

        QMenu menu;
        QAction* openTextureFolder = menu.addAction("Open texture folder");
        QObject::connect(openTextureFolder, &QAction::triggered, this, &TextureListDelegate::onOpenTexturePath);

        QAction* reloadTexture = menu.addAction("Reload texture");
        QObject::connect(reloadTexture, &QAction::triggered, this, &TextureListDelegate::onReloadTexture);

        QAction* savePresetAction = menu.addAction(ActionIcon::savePresetIcon, "Save Preset");
        QObject::connect(savePresetAction, &QAction::triggered, this, &TextureListDelegate::onSavePreset);

        QAction* loadPresetAction = menu.addAction(ActionIcon::loadPresetIcon, "Load Preset");
        QObject::connect(loadPresetAction, &QAction::triggered, this, &TextureListDelegate::onLoadPreset);

        menu.exec(QCursor::pos());
    }
    break;

    default:
        break;
    }

    return QAbstractItemDelegate::editorEvent(event, model, option, index);
}

void TextureListDelegate::onReloadTexture()
{
    if (nullptr == lastSelectedTextureDescriptor)
    {
        return;
    }

    emit textureDescriptorReload(lastSelectedTextureDescriptor);
}

void TextureListDelegate::onOpenTexturePath()
{
    if (nullptr == lastSelectedTextureDescriptor)
    {
        return;
    }

    const QString pathname = lastSelectedTextureDescriptor->pathname.GetAbsolutePathname().c_str();
    QFileInfo fileInfo(pathname);
    QtHelpers::ShowInOSFileManager(fileInfo.absoluteFilePath());

    lastSelectedTextureDescriptor = nullptr;
}

void TextureListDelegate::onLoadPreset()
{
    if (nullptr == lastSelectedTextureDescriptor)
    {
        return;
    }

    bool loaded = DAVA::Preset::DialogLoadPresetForTexture(lastSelectedTextureDescriptor);
    if (loaded)
    {
        emit textureDescriptorChanged(lastSelectedTextureDescriptor);
    }

    lastSelectedTextureDescriptor = nullptr;
}

void TextureListDelegate::onSavePreset()
{
    if (nullptr == lastSelectedTextureDescriptor)
    {
        return;
    }

    DAVA::Preset::DialogSavePresetForTexture(lastSelectedTextureDescriptor);

    lastSelectedTextureDescriptor = nullptr;
}
