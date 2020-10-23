#include "TextureScrollArea.h"
#include "Render/Texture.h"

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsLinearLayout>
#include <QProgressBar>
#include <QApplication>
#include <QWheelEvent>
#include <QScrollBar>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QMovie>
#include <QLabel>

static int facePositions[6][2] =
{
  { 2, 1 }, //pos x
  { 0, 1 }, //neg x
  { 1, 0 }, //pos y
  { 1, 2 }, //neg y
  { 1, 1 }, //pos z
  { 1, 3 } //neg z
};

TextureScrollArea::TextureScrollArea(QWidget* parent /* = 0 */)
    : QGraphicsView(parent)
    , textureColorMask((int)ChannelAll)
    , mouseInMoveState(false)
    , textureBorder(NULL)
    , compositeImagesFlags(0)
    , textureScene(NULL)
    , zoomFactor(1.0)
    , tiledBgDoDraw(false)
    , noImageVisible(false)
{
    // create and setup scene
    textureScene = new QGraphicsScene();
    setRenderHints((QPainter::RenderHints)0);
    setScene(textureScene);

    // we can have complex background (if tiledBgDoDraw set to true),
    // so set mode to redraw it each time
    // and then prepare pixmap for our custom background
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    sutupCustomTiledBg();

    // add items to scene
    texturePixmap = textureScene->addPixmap(QPixmap());
    textureBorder = textureScene->addRect(0, 0, 10, 10, QPen(QColor(255, 255, 0, 255)), QBrush(Qt::NoBrush));

    // add "No Image" label
    {
        noImageLabel = new QLabel("No image");
        noImageLabel->setAttribute(Qt::WA_NoSystemBackground, true);
        // label color
        QPalette palette = noImageLabel->palette();
        palette.setColor(noImageLabel->foregroundRole(), Qt::gray);
        noImageLabel->setPalette(palette);
        // label font size
        QFont font = noImageLabel->font();
        font.setPointSize(18);
        font.setBold(true);
        noImageLabel->setFont(font);
        noImageLabel->setAlignment(Qt::AlignCenter);
        // add it to scene
        noImageProxy = textureScene->addWidget(noImageLabel);
        noImageProxy->setGeometry(QRectF(-150, -20, 150, 20));
        noImageProxy->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    }

    // add warning label
    {
        warningLabel = new QLabel("Warning");
        warningLabel->setAttribute(Qt::WA_NoSystemBackground, true);
        // label color
        QPalette palette = noImageLabel->palette();
        palette.setColor(noImageLabel->foregroundRole(), Qt::red);
        warningLabel->setPalette(palette);
        // label font size
        QFont font = warningLabel->font();
        font.setPointSize(18);
        font.setBold(true);
        warningLabel->setFont(font);
        warningLabel->setAlignment(Qt::AlignCenter);
        // add it to scene
        warningProxy = textureScene->addWidget(warningLabel);
        warningProxy->setGeometry(QRectF(-150, -40, 150, -20));
        warningProxy->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    }

    // add wait-bar to scene
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(0);
    progressBar->setTextVisible(false);
    progressBar->setAttribute(Qt::WA_NoSystemBackground, true);
    // add to scene
    waitBar = textureScene->addWidget(progressBar);
    waitBar->setGeometry(QRectF(-120, -15, 120, 15));
    waitBar->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    adjustWidgetsPos();

    borderShow(false);
    bgmaskShow(false);
    waitbarShow(false);
    warningShow(false);
}

namespace TextureScrollAreaDetail
{
template <class TYPE>
void SafeDeleteObject(TYPE*& obj)
{
    if (obj != nullptr && obj->parent() != nullptr)
    {
        DAVA::SafeDelete(obj);
    }
}
}

TextureScrollArea::~TextureScrollArea()
{
    TextureScrollAreaDetail::SafeDeleteObject(waitBar);

    TextureScrollAreaDetail::SafeDeleteObject(warningProxy);
    TextureScrollAreaDetail::SafeDeleteObject(warningLabel);

    TextureScrollAreaDetail::SafeDeleteObject(noImageProxy);
    TextureScrollAreaDetail::SafeDeleteObject(noImageLabel);

    TextureScrollAreaDetail::SafeDeleteObject(textureScene);
}

void TextureScrollArea::setImage(const QImage& image)
{
    currentCompositeImages.clear();
    compositeImagesFlags = 0;
    cubeDrawPixmap = QPixmap(1, 1);

    currentTextureImage = image;

    applyTextureImageToScenePixmap();
    applyTextureImageBorder();

    noImageVisible = currentTextureImage.isNull();
    noImageProxy->setVisible(noImageVisible);

    adjustWidgetsPos();
}

void TextureScrollArea::setColorChannel(int mask, ChannelApplyMode mode)
{
    textureColorMask = mask;
    if (mode == ApplyNow)
    {
        applyTextureImageToScenePixmap();
    }
}

float TextureScrollArea::getTextureZoom()
{
    return zoomFactor;
}

QColor TextureScrollArea::getPixelColor(QPoint pos)
{
    QRgb rgb = 0;

    if (isCompositeImage())
    {
        int tileWidth = currentCompositeImages[0].width();
        int tileHeight = currentCompositeImages[0].height();
        for (int i = 0; i < DAVA::Texture::CUBE_FACE_COUNT; ++i)
        {
            if ((compositeImagesFlags & (1 << i)) != 0)
            {
                int px = facePositions[i][0] * tileWidth;
                int py = facePositions[i][1] * tileHeight;

                if (pos.x() >= px &&
                    pos.x() <= px + tileWidth &&
                    pos.y() >= py &&
                    pos.y() <= py + tileWidth)
                {
                    int x = pos.x() - px;
                    int y = pos.y() - py;

                    rgb = currentCompositeImages[i].pixel(QPoint(x, y));
                }
            }
        }
    }
    else
    {
        if (pos.x() >= 0 && pos.x() < currentTextureImage.width() &&
            pos.y() >= 0 && pos.y() < currentTextureImage.height())
        {
            rgb = currentTextureImage.pixel(pos);
        }
    }

    return QColor::fromRgba(rgb);
}

void TextureScrollArea::resetTexturePosZoom()
{
    setTextureZoom(1.0);
    setTexturePos(QPoint(0, 0));
}

void TextureScrollArea::borderShow(bool show)
{
    if (show)
    {
        textureBorder->show();
    }
    else
    {
        textureBorder->hide();
    }
}

void TextureScrollArea::bgmaskShow(bool show)
{
    tiledBgDoDraw = show;

    // call this setBackgroundBrush function to force background redraw
    textureScene->setBackgroundBrush(QBrush(QColor(0, 0, 0)));
}

void TextureScrollArea::waitbarShow(bool show)
{
    if (show)
    {
        noImageProxy->setVisible(false);
        waitBar->show();
    }
    else
    {
        noImageProxy->setVisible(noImageVisible);
        waitBar->hide();
    }

    adjustWidgetsPos();
}

void TextureScrollArea::warningSetText(const QString& text)
{
    warningLabel->setText(text);
}

void TextureScrollArea::warningShow(bool show)
{
    warningProxy->setVisible(show);
}

void TextureScrollArea::setTextureZoom(const float& zoom)
{
    if (zoom != zoomFactor)
    {
        zoomFactor = zoom;

        resetTransform();
        scale(zoomFactor, zoomFactor);
        emit textureZoomChanged(zoomFactor);

        adjustWidgetsPos();
    }
}

void TextureScrollArea::setTexturePos(const QPoint& pos)
{
    horizontalScrollBar()->setValue(pos.x());
    verticalScrollBar()->setValue(pos.y());
}

void TextureScrollArea::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx, dy);
    emit texturePosChanged(QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value()));
}

void TextureScrollArea::wheelEvent(QWheelEvent* e)
{
    emit mouseWheel(e->delta());
}

void TextureScrollArea::mouseMoveEvent(QMouseEvent* event)
{
    QGraphicsView::mouseMoveEvent(event);

    if (mouseInMoveState)
    {
        int mouseDx = event->pos().x() - mousePressPos.x();
        int mouseDy = event->pos().y() - mousePressPos.y();

        horizontalScrollBar()->setValue(mousePressScrollPos.x() - mouseDx);
        verticalScrollBar()->setValue(mousePressScrollPos.y() - mouseDy);
    }
    else
    {
        QPointF scenePos = mapToScene(event->pos().x(), event->pos().y());
        emit mouseOverPixel(QPoint((int)scenePos.x(), (int)scenePos.y()));
    }
}

void TextureScrollArea::mousePressEvent(QMouseEvent* event)
{
    QGraphicsView::mousePressEvent(event);

    mouseInMoveState = true;
    mousePressPos = event->pos();
    mousePressScrollPos.setX(horizontalScrollBar()->value());
    mousePressScrollPos.setY(verticalScrollBar()->value());
    viewport()->setProperty("cursor", QVariant(QCursor(Qt::ClosedHandCursor)));
}

void TextureScrollArea::mouseReleaseEvent(QMouseEvent* event)
{
    QGraphicsView::mouseReleaseEvent(event);

    mouseInMoveState = false;
    viewport()->setProperty("cursor", QVariant(QCursor(Qt::ArrowCursor)));
}

void TextureScrollArea::applyCurrentImageToScenePixmap()
{
    if (~textureColorMask)
    {
        QImage tmpImage;
        prepareImageWithColormask(currentTextureImage, tmpImage);

        QPixmap pixmap = QPixmap::fromImage(tmpImage);
        if (!pixmap.isNull())
            textureScene->setSceneRect(pixmap.rect());
        texturePixmap->setPixmap(pixmap);
    }
    else
    {
        QPixmap pixmap = QPixmap::fromImage(currentTextureImage);
        textureScene->setSceneRect(pixmap.rect());
        texturePixmap->setPixmap(pixmap);
    }
}

void TextureScrollArea::applyCurrentImageBorder()
{
    QRectF r(currentTextureImage.rect());

    if (r.width() != 0 && r.height() != 0)
    {
        r.adjust(-0.1, -0.1, 0.1, 0.1);
    }

    textureBorder->setRect(r);
    r.adjust(0, 0, 1, 1);
    textureScene->setSceneRect(r);
}

void TextureScrollArea::adjustWidgetsPos()
{
    // apply to waitBar inverted transform - so it will be always same size
    // waitBar->setTransform(transform().inverted());

    // calculate new waitBar pos
    qreal scaleX = transform().m11();
    qreal scaleY = transform().m22();
    QRectF rect = waitBar->sceneBoundingRect();
    QPointF viewCenter = mapToScene(width() / 2.0, height() / 2.0);
    qreal x = viewCenter.x() - rect.width() / 2.0 / scaleX;
    qreal y = viewCenter.x() - rect.height() / 2.0 / scaleY;
    waitBar->setPos(x, y);

    // calculate new noImage pos
    rect = noImageProxy->sceneBoundingRect();
    x = viewCenter.x() - rect.width() / 2.0 / scaleX;
    y = viewCenter.x() - rect.height() / 2.0 / scaleY;
    noImageProxy->setPos(x, y);

    // calculate warning pos
    rect = warningProxy->sceneBoundingRect();
    x = viewCenter.x() - rect.width() / 2.0 / scaleX;
    y = viewCenter.x() - rect.height() / 2.0 / scaleY - (warningLabel->font().pointSize() + 25); // 25 is spacing
    warningProxy->setPos(x, y);
}

void TextureScrollArea::drawBackground(QPainter* painter, const QRectF& rect)
{
    if (tiledBgDoDraw)
    {
        painter->resetTransform();
        painter->drawTiledPixmap(this->rect(), tiledBgPixmap);
    }
    else
    {
        QGraphicsView::drawBackground(painter, rect);
    }
}

void TextureScrollArea::sutupCustomTiledBg()
{
    tiledBgPixmap = QPixmap(30, 30);
    QPainter p(&tiledBgPixmap);
    p.setBrush(QBrush(QColor(150, 150, 150)));
    p.setPen(Qt::NoPen);
    p.drawRect(QRect(0, 0, 30, 30));
    p.setBrush(QBrush(QColor(200, 200, 200)));
    p.drawRect(QRect(0, 0, 15, 15));
    p.drawRect(QRect(15, 15, 15, 15));
}

QImage TextureScrollArea::getImage()
{
    return currentTextureImage;
}

void TextureScrollArea::setImage(const QList<QImage>& images, int flags)
{
    currentTextureImage = QImage();

    currentCompositeImages.clear();
    currentCompositeImages.reserve(images.size());

    noImageVisible = true;
    for (const QImage& image : images)
    {
        currentCompositeImages.push_back(image);
        noImageVisible = noImageVisible && image.isNull();
    }
    noImageProxy->setVisible(noImageVisible);

    compositeImagesFlags = flags;

    if (!noImageVisible)
    {
        applyTextureImageToScenePixmap();
        applyTextureImageBorder();
    }

    adjustWidgetsPos();
}

bool TextureScrollArea::isCompositeImage() const
{
    return (currentCompositeImages.size() > 0);
}

void TextureScrollArea::prepareImageWithColormask(QImage& srcImage, QImage& dstImage)
{
    // TODO: optimize this code

    int mask = 0xFFFFFFFF;
    int maskA = 0;

    dstImage = srcImage.convertToFormat(QImage::Format_ARGB32);

    if (!(textureColorMask & ChannelR))
        mask &= 0xFF00FFFF;
    if (!(textureColorMask & ChannelG))
        mask &= 0xFFFF00FF;
    if (!(textureColorMask & ChannelB))
        mask &= 0xFFFFFF00;
    if (!(textureColorMask & ChannelA))
        maskA |= 0xFF000000;

    if (mask == 0xFF000000)
    {
        maskA ^= 0xFF000000;

        // only alpha, so show it
        for (int y = 0; y < dstImage.height(); y++)
        {
            QRgb* line = (QRgb*)dstImage.scanLine(y);
            for (int x = 0; x < dstImage.width(); x++)
            {
                int c = (line[x] & 0xFF000000) >> 24;
                line[x] = (maskA | c << 16 | c << 8 | c);
            }
        }
    }
    else
    {
        for (int y = 0; y < dstImage.height(); y++)
        {
            QRgb* line = (QRgb*)dstImage.scanLine(y);
            for (int x = 0; x < dstImage.width(); x++)
            {
                line[x] &= mask;
                line[x] |= maskA;
            }
        }
    }
}

void TextureScrollArea::applyCurrentCompositeImagesToScenePixmap()
{
    int tileWidth = currentCompositeImages[0].width();
    int tileHeight = currentCompositeImages[0].height();

    cubeDrawPixmap = QPixmap(tileWidth * 3,
                             tileHeight * 4);

    QPainter p(&cubeDrawPixmap);
    p.setBrush(QBrush(QColor(0, 0, 0)));
    p.setPen(Qt::NoPen);
    p.drawRect(0, 0, cubeDrawPixmap.width(), cubeDrawPixmap.height());

    QMatrix rotation;
    //rotation.rotate(-90);
    int currentIndex = 0;
    for (int i = 0; i < DAVA::Texture::CUBE_FACE_COUNT; ++i)
    {
        if ((compositeImagesFlags & (1 << i)) != 0)
        {
            int px = facePositions[i][0] * tileWidth;
            int py = facePositions[i][1] * tileHeight;

            if (~textureColorMask)
            {
                QImage tmpImage;
                prepareImageWithColormask(currentCompositeImages[currentIndex], tmpImage);
                p.drawImage(QPoint(px, py), tmpImage.transformed(rotation));
            }
            else
            {
                p.drawImage(QPoint(px, py), currentCompositeImages[currentIndex].transformed(rotation));
            }

            currentIndex++;
        }
    }

    textureScene->setSceneRect(cubeDrawPixmap.rect());
    texturePixmap->setPixmap(cubeDrawPixmap);
}

void TextureScrollArea::applyTextureImageToScenePixmap()
{
    if (isCompositeImage())
    {
        applyCurrentCompositeImagesToScenePixmap();
    }
    else
    {
        applyCurrentImageToScenePixmap();
    }
}

void TextureScrollArea::applyTextureImageBorder()
{
    if (isCompositeImage())
    {
        applyCompositeImageBorder();
    }
    else
    {
        applyCurrentImageBorder();
    }
}

void TextureScrollArea::applyCompositeImageBorder()
{
    QRectF r(cubeDrawPixmap.rect());

    if (r.width() != 0 && r.height() != 0)
    {
        r.adjust(-0.1, -0.1, 0.1, 0.1);
    }

    textureBorder->setRect(r);
    r.adjust(0, 0, 1, 1);
    textureScene->setSceneRect(r);
}

QSize TextureScrollArea::getContentSize()
{
    QSize size;
    if (isCompositeImage())
    {
        int tileWidth = currentCompositeImages[0].width();
        int tileHeight = currentCompositeImages[0].height();

        size.setWidth(tileWidth * 3);
        size.setHeight(tileHeight * 4);
    }
    else
    {
        size = currentTextureImage.size();
    }

    return size;
}
