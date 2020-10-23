#ifndef __MATERIALS_ITEM_H__
#define __MATERIALS_ITEM_H__

#include "Render/Material/NMaterial.h"

#include <QObject>
#include <QStandardItem>

class MaterialModel;
class MaterialItem
: public QObject
  ,
  public QStandardItem
{
    Q_OBJECT

public:
    enum MaterialFlag : DAVA::uint32
    {
        IS_MARK_FOR_DELETE = 0x1,
        IS_PART_OF_SELECTION = 0x2,
    };

    MaterialItem(DAVA::NMaterial* material, bool dragEnabled, bool dropEnabled);
    virtual ~MaterialItem();

    QVariant data(int role = Qt::UserRole + 1) const;
    DAVA::NMaterial* GetMaterial() const;

    void SetFlag(MaterialFlag flag, bool set);
    bool GetFlag(MaterialFlag flag) const;

    void SetLodIndex(DAVA::int32 index);
    int GetLodIndex() const;

    void SetSwitchIndex(DAVA::int32 index);
    int GetSwitchIndex() const;

    void requestPreview();

private slots:
    void onThumbnailReady(const QList<QImage>& images, QVariant userData);

private:
    DAVA::NMaterial* material;
    DAVA::uint32 curFlag = 0;
    DAVA::int32 lodIndex = -1;
    DAVA::int32 switchIndex = -1;
    bool isPreviewRequested = false;
};


#endif // __MATERIALS_ITEM_H__
