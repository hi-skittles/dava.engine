#ifndef __TEXTURE_PROPERTIES_H__
#define __TEXTURE_PROPERTIES_H__

#include "DAVAEngine.h"
#include "Base/EnumMap.h"

#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyEditor.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataMetaObject.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspMember.h"

class LazyUpdater;
class TextureProperties : public QtPropertyEditor
{
    Q_OBJECT

public:
    typedef enum PropType
    {
        PROP_MIPMAP,
        PROP_NORMALMAP,
        PROP_WRAP,
        PROP_FILTER,
        PROP_FORMAT,
        PROP_SIZE
    } PropertiesType;

public:
    TextureProperties(QWidget* parent = nullptr);
    ~TextureProperties() override;

    void setTextureDescriptor(DAVA::TextureDescriptor* descriptor);
    void setTextureGPU(DAVA::eGPUFamily gpu);

    const DAVA::TextureDescriptor* getTextureDescriptor();
    void setOriginalImageSize(const QSize& size);

signals:
    void PropertyChanged(int type);

private slots:

    void OnPropertyChanged(int type);

protected:
    void OnItemEdited(const QModelIndex&) override;

    void Save();

    void MipMapSizesInit(int baseWidth, int baseHeight);
    void MipMapSizesReset();

    void ReloadEnumFormats();
    void ReloadEnumWrap();
    void ReloadEnumFilters();
    void ReloadProperties();

    QtPropertyDataInspMember* AddPropertyItem(const DAVA::FastName& name, DAVA::InspBase* object, const QModelIndex& parent);
    void SetPropertyItemValidValues(QtPropertyDataInspMember* item, EnumMap* validValues);
    void SetPropertyItemValidValues(QtPropertyDataMetaObject* item, EnumMap* validValues);

    void LoadCurSizeToProp();
    void SaveCurSizeFromProp();

protected:
    DAVA::TextureDescriptor* curTextureDescriptor = nullptr;
    DAVA::eGPUFamily curGPU;

    QtPropertyDataInspMember* propMipMap = nullptr;
    QtPropertyDataInspMember* propNormalMap = nullptr;
    QtPropertyDataInspMember* propWrapModeS = nullptr;
    QtPropertyDataInspMember* propWrapModeT = nullptr;
    QtPropertyDataInspMember* propMinFilter = nullptr;
    QtPropertyDataInspMember* propMagFilter = nullptr;
    QtPropertyDataInspMember* propMipFilter = nullptr;
    QtPropertyDataInspMember* propFormat = nullptr;
    QtPropertyDataMetaObject* propSizes = nullptr;

    bool skipPropSizeChanged;

    QSize origImageSize;
    int curSizeLevelObject;

    EnumMap enumFormats;
    EnumMap enumSizes;
    EnumMap enumWpar;
    EnumMap enumFiltersMin;
    EnumMap enumFiltersMag;
    EnumMap enumFiltersMip;

    QMap<int, QSize> availableSizes;

    LazyUpdater* updater = nullptr;
};

#endif // __TEXTURE_PROPERTIES_H__
