#ifndef __TEXTURE_SCROLL_AREA_H__
#define __TEXTURE_SCROLL_AREA_H__

#include <QGraphicsView>

#include "Base/BaseTypes.h"

class QImage;
class QLabel;

class TextureScrollArea : public QGraphicsView
{
    Q_OBJECT

public:
    enum TextureColorChannels : DAVA::uint32
    {
        ChannelNo = 0,

        ChannelR = 0x1,
        ChannelG = 0x2,
        ChannelB = 0x4,
        ChannelA = 0x8,

        ChannelAll = 0xFFFFFFFF
    };

    enum ChannelApplyMode
    {
        ApplyNow, // apply to image immediately
        DoNotApplyNow // apply on next setImage
    };

    TextureScrollArea(QWidget* parent = 0);
    ~TextureScrollArea();

    void setImage(const QImage& image);
    void setImage(const QList<QImage>& images, int flags = 0x000000FF); //this method sets cubemap faces
    QImage getImage();
    void setColorChannel(int mask, ChannelApplyMode mode = ApplyNow);

    QColor getPixelColor(QPoint pos);
    float getTextureZoom();

    void borderShow(bool show);
    void bgmaskShow(bool show);
    void waitbarShow(bool show);

    void warningSetText(const QString& text);
    void warningShow(bool show);

    void resetTexturePosZoom();

    QSize getContentSize();

public slots:
    void setTexturePos(const QPoint& pos);
    void setTextureZoom(const float& zoom);

signals:
    void texturePosChanged(const QPoint& pos);
    void textureZoomChanged(const float& zoom);

    void mouseOverPixel(const QPoint& pos);
    void mouseWheel(int delta);

protected:
    virtual void drawBackground(QPainter* painter, const QRectF& rect);
    virtual void scrollContentsBy(int dx, int dy);
    virtual void wheelEvent(QWheelEvent* e);

    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);

private:
    int textureColorMask;

    bool mouseInMoveState;
    QPoint mousePressPos;
    QPoint mousePressScrollPos;

    QGraphicsRectItem* textureBorder = nullptr;
    QGraphicsProxyWidget* waitBar = nullptr;

    QImage currentTextureImage;

    DAVA::Vector<QImage> currentCompositeImages;
    int compositeImagesFlags;
    QPixmap cubeDrawPixmap;

    QGraphicsScene* textureScene = nullptr;
    QGraphicsPixmapItem* texturePixmap = nullptr;
    float zoomFactor;

    bool tiledBgDoDraw;
    QPixmap tiledBgPixmap;

    bool noImageVisible;
    QLabel* noImageLabel = nullptr;
    QGraphicsProxyWidget* noImageProxy = nullptr;

    QLabel* warningLabel = nullptr;
    QGraphicsProxyWidget* warningProxy = nullptr;

    void sutupCustomTiledBg();

    void applyTextureImageToScenePixmap();

    void applyCurrentImageToScenePixmap();
    void applyCurrentCompositeImagesToScenePixmap();

    void applyTextureImageBorder();
    void applyCompositeImageBorder();
    void applyCurrentImageBorder();

    void adjustWidgetsPos();

    void prepareImageWithColormask(QImage& srcImage, QImage& dstImage);

    bool isCompositeImage() const;
};

#endif // __TEXTURE_SCROLL_AREA_H__
