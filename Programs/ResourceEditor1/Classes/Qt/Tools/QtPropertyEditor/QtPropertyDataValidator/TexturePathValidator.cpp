#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyDataValidator/TexturePathValidator.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Utils/TextureDescriptor/RETextureDescriptorUtils.h"

#include <FileSystem/FileSystem.h>

TexturePathValidator::TexturePathValidator(const QStringList& value)
    : PathValidator(value)
{
}

bool TexturePathValidator::ValidateInternal(const QVariant& v)
{
    bool res = RegExpValidator::ValidateInternal(v);

    QString val = v.toString();
    if (res && val != "")
    {
        res = val.endsWith(QString::fromStdString(DAVA::TextureDescriptor::GetDescriptorExtension()));
    }

    return res;
}

void TexturePathValidator::FixupInternal(QVariant& v) const
{
    CommonInternalSettings* settings = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>();
    if (v.type() == QVariant::String)
    {
        DAVA::FilePath texturePath = DAVA::FilePath(v.toString().toStdString());
        if (DAVA::FileSystem::Instance()->Exists(texturePath) && RETextureDescriptorUtils::CreateOrUpdateDescriptor(texturePath))
        {
            DAVA::FilePath descriptorPath = DAVA::TextureDescriptor::GetDescriptorPathname(texturePath);

            auto& texturesMap = DAVA::Texture::GetTextureMap();
            auto found = texturesMap.find(FILEPATH_MAP_KEY(descriptorPath));
            if (found != texturesMap.end())
            {
                DAVA::Vector<DAVA::Texture*> reloadTextures;
                reloadTextures.push_back(found->second);

                REGlobal::GetInvoker()->Invoke(REGlobal::ReloadTextures.ID, reloadTextures);
            }

            v = QVariant(QString::fromStdString(descriptorPath.GetAbsolutePathname()));
        }
    }
}
