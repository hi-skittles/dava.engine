#include "TileTexturePreviewWidget.h"
#include "TileTexturePreviewWidgetItemDelegate.h"

#include "Classes/Qt/TextureBrowser/TextureConvertor.h"

#include <REPlatform/Global/StringConstants.h>
#include <REPlatform/Scene/Utils/ImageTools.h>

#include <TArc/Controls/ColorPicker/ColorPickerDialog.h>
#include <TArc/Core/Deprecated.h>
#include <TArc/Utils/Utils.h>

#include <QHeaderView>
#include <QLabel>
#include <QEvent>

TileTexturePreviewWidget::TileTexturePreviewWidget(QWidget* parent)
    : QTreeWidget(parent)
    , selectedTexture(0)
    , mode(MODES_COUNT)
    , validator(NULL)
{
    colors.reserve(DEF_TILE_TEXTURES_COUNT);
    images.reserve(DEF_TILE_TEXTURES_COUNT);
    labels.reserve(DEF_TILE_TEXTURES_COUNT);

    SetMode(MODE_WITH_COLORS);
    ConnectToSignals();

    validator = new QRegExpValidator();
    validator->setRegExp(QRegExp(TileTexturePreviewWidgetItemDelegate::TILE_COLOR_VALIDATE_REGEXP, Qt::CaseInsensitive));
}

TileTexturePreviewWidget::~TileTexturePreviewWidget()
{
    Clear();

    if (itemDelegate != nullptr)
    {
        setItemDelegate(nullptr);
        DAVA::SafeDelete(itemDelegate);
    }

    DAVA::SafeDelete(validator);
}

void TileTexturePreviewWidget::Clear()
{
    clear();

    for (auto& image : images)
    {
        SafeRelease(image);
    }

    colors.clear();
    images.clear();
    labels.clear();
}

void TileTexturePreviewWidget::AddTexture(DAVA::Image* previewTexture, const DAVA::Color& color /*  = Color::White */)
{
    DVASSERT(previewTexture->GetPixelFormat() == DAVA::FORMAT_RGBA8888);

    bool blocked = signalsBlocked();
    blockSignals(true);

    images.push_back(SafeRetain(previewTexture));

    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setCheckState(0, Qt::Unchecked);
    addTopLevelItem(item);

    if (mode == MODE_WITHOUT_COLORS)
    {
        item->setFlags((item->flags() | Qt::ItemIsUserCheckable) & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable));
    }
    else
    {
        item->setFlags((item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable) & ~(Qt::ItemIsSelectable));

        QLabel* label = new QLabel();
        label->setMinimumWidth(26);
        label->setFrameShape(QFrame::Box);
        label->setAutoFillBackground(true);
        setItemWidget(item, COLOR_PREVIEW_COLUMN, label);
        label->setMinimumHeight(TEXTURE_PREVIEW_HEIGHT);
        labels.push_back(label);
        label->installEventFilter(this);
        label->setToolTip(DAVA::ResourceEditor::TILE_TEXTURE_PREVIEW_CHANGE_COLOR_TOOLTIP.c_str());
        label->setCursor(Qt::PointingHandCursor);

        colors.push_back(color);

        UpdateColor(static_cast<DAVA::uint32>(images.size() - 1));

        if (itemDelegate == nullptr)
        {
            itemDelegate = new TileTexturePreviewWidgetItemDelegate();
            setItemDelegate(itemDelegate);
        }
    }

    UpdateImage(static_cast<DAVA::uint32>(images.size() - 1));
    UpdateSelection();

    blockSignals(blocked);
}

void TileTexturePreviewWidget::InitWithColors()
{
    bool blocked = signalsBlocked();
    blockSignals(true);

    Clear();

    setHeaderHidden(true);
    setColumnCount(2);
    setRootIsDecorated(false);
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    setIconSize(QSize(TEXTURE_PREVIEW_WIDTH_SMALL, TEXTURE_PREVIEW_HEIGHT));

    blockSignals(blocked);
}

void TileTexturePreviewWidget::InitWithoutColors()
{
    bool blocked = signalsBlocked();
    blockSignals(true);

    Clear();

    setHeaderHidden(true);
    setColumnCount(1);
    setRootIsDecorated(false);
    setIconSize(QSize(TEXTURE_PREVIEW_WIDTH, TEXTURE_PREVIEW_HEIGHT));

    blockSignals(blocked);
}

void TileTexturePreviewWidget::ConnectToSignals()
{
    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(OnCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(OnItemChanged(QTreeWidgetItem*, int)));
}

DAVA::uint32 TileTexturePreviewWidget::GetSelectedTexture()
{
    return selectedTexture;
}

void TileTexturePreviewWidget::SetSelectedTexture(DAVA::uint32 number)
{
    if (number < static_cast<DAVA::uint32>(images.size()))
    {
        selectedTexture = number;
        UpdateSelection();

        emit SelectionChanged(selectedTexture);
    }
}

void TileTexturePreviewWidget::UpdateImage(DAVA::uint32 number)
{
    DVASSERT(number < static_cast<DAVA::uint32>(images.size()));

    QTreeWidgetItem* item = topLevelItem(number);

    DAVA::Image* image;
    if (mode == MODE_WITH_COLORS)
    {
        image = MultiplyImageWithColor(images[number], colors[number]);
    }
    else
    {
        image = SafeRetain(images[number]);
    }

    QImage qimg = DAVA::ImageTools::FromDavaImage(image);
    DAVA::SafeRelease(image);

    QSize size = QSize(TEXTURE_PREVIEW_WIDTH, TEXTURE_PREVIEW_HEIGHT);
    if (mode == MODE_WITH_COLORS)
    {
        size.setWidth(TEXTURE_PREVIEW_WIDTH_SMALL);
    }

    QImage previewImage = qimg.copy(0, 0, size.width(), size.height());
    item->setIcon(0, QIcon(QPixmap::fromImage(previewImage)));
}

void TileTexturePreviewWidget::UpdateColor(DAVA::uint32 number)
{
    DVASSERT(number < static_cast<DAVA::uint32>(images.size()));

    bool blocked = blockSignals(true);

    QTreeWidgetItem* item = topLevelItem(number);
    QColor color = DAVA::ColorToQColor(colors[number]);
    color.setAlpha(255);

    QPalette palette = labels[number]->palette();
    palette.setColor(labels[number]->backgroundRole(), color);
    labels[number]->setPalette(palette);

    QString str;
    str.sprintf("#%02x%02x%02x", color.red(), color.green(), color.blue());
    item->setText(0, str);

    UpdateImage(number);

    blockSignals(blocked);
}

void TileTexturePreviewWidget::UpdateSelection()
{
    for (DAVA::int32 i = 0; i < static_cast<DAVA::int32>(images.size()); ++i)
    {
        QTreeWidgetItem* item = topLevelItem(i);
        item->setCheckState(0, (i == selectedTexture ? Qt::Checked : Qt::Unchecked));
    }
}

void TileTexturePreviewWidget::OnCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    QModelIndex index = currentIndex();
    selectedTexture = index.row();
    UpdateSelection();

    emit SelectionChanged(selectedTexture);
}

void TileTexturePreviewWidget::OnItemChanged(QTreeWidgetItem* item, int column)
{
    DAVA::int32 index = indexOfTopLevelItem(item);

    if (mode == MODE_WITH_COLORS)
    {
        DAVA::int32 len = 0;
        QString str = item->text(0);
        QValidator::State state = validator->validate(str, len);

        if (state == QValidator::Acceptable)
        {
            QString colorString = item->text(0);
            if (!colorString.startsWith("#"))
            {
                colorString = "#" + colorString;
            }

            QColor color = QColor(colorString);
            if (color.isValid())
            {
                DAVA::Color c = DAVA::QColorToColor(color);
                if (c != colors[index])
                {
                    SetColor(index, c);
                }
            }
        }
    }

    if (item->checkState(0) == Qt::Checked)
    {
        SetSelectedTexture(index);
    }
    else
    {
        UpdateSelection();
    }
}

bool TileTexturePreviewWidget::eventFilter(QObject* obj, QEvent* ev)
{
    for (DAVA::int32 i = 0; i < static_cast<DAVA::int32>(labels.size()); ++i)
    {
        if (obj == labels[i])
        {
            if (ev->type() == QEvent::MouseButtonRelease)
            {
                const DAVA::Color oldColor = colors[i];
                DAVA::ColorPickerDialog cp(DAVA::Deprecated::GetAccessor(), this);
                cp.setWindowTitle("Tile color");
                cp.SetDavaColor(oldColor);
                const bool result = cp.Exec();
                const DAVA::Color newColor = cp.GetDavaColor();

                if (result && newColor != oldColor)
                {
                    SetColor(i, newColor);
                }

                return true;
            }
            else if (ev->type() == QEvent::MouseButtonPress)
            {
                return true;
            }
        }
    }

    return QObject::eventFilter(obj, ev);
}

void TileTexturePreviewWidget::SetColor(DAVA::uint32 number, const DAVA::Color& color)
{
    colors[number] = color;
    emit TileColorChanged(number, colors[number]);
    UpdateColor(number);
}

void TileTexturePreviewWidget::SetMode(TileTexturePreviewWidget::eWidgetModes mode)
{
    if (mode == this->mode)
    {
        return;
    }

    if (mode == MODE_WITH_COLORS)
    {
        InitWithColors();
    }
    else if (mode == MODE_WITHOUT_COLORS)
    {
        InitWithoutColors();
    }
    this->mode = mode;
}

DAVA::Image* TileTexturePreviewWidget::MultiplyImageWithColor(DAVA::Image* image, const DAVA::Color& color)
{
    DVASSERT(image->GetPixelFormat() == DAVA::FORMAT_RGBA8888);

    DAVA::Image* newImage = DAVA::Image::Create(image->GetWidth(), image->GetHeight(), image->GetPixelFormat());

    DAVA::uint32* imageData = reinterpret_cast<DAVA::uint32*>(image->GetData());
    DAVA::uint32* newImageData = reinterpret_cast<DAVA::uint32*>(newImage->GetData());

    DAVA::int32 pixelsCount = image->dataSize / sizeof(DAVA::uint32);

    for (DAVA::int32 i = 0; i < pixelsCount; ++i)
    {
        DAVA::uint8* pixelData = reinterpret_cast<DAVA::uint8*>(imageData);
        DAVA::uint8* newPixelData = reinterpret_cast<DAVA::uint8*>(newImageData);

        newPixelData[0] = static_cast<DAVA::uint8>(floorf(pixelData[0] * color.r));
        newPixelData[1] = static_cast<DAVA::uint8>(floorf(pixelData[1] * color.g));
        newPixelData[2] = static_cast<DAVA::uint8>(floorf(pixelData[2] * color.b));
        newPixelData[3] = 255;

        ++imageData;
        ++newImageData;
    }

    return newImage;
}

void TileTexturePreviewWidget::UpdateColor(DAVA::uint32 index, const DAVA::Color& color)
{
    if (index < static_cast<DAVA::uint32>(colors.size()))
    {
        if (colors[index] != color)
        {
            SetColor(index, color);
        }
    }
}
