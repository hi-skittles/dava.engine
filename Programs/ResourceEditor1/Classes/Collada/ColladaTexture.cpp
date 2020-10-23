#include "stdafx.h"
#include <wchar.h>
#include "ColladaTexture.h"
#include "Logger/Logger.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "FileSystem/FileSystem.h"
#include "Utils/Utils.h"
#include "Qt/Main/QtUtils.h"

#include "Utils/TextureDescriptor/RETextureDescriptorUtils.h"

namespace DAVA
{
ColladaTexture::ColladaTexture(FCDImage* _image)
    : texturePathName(_image->GetFilename())
{
    image = _image;

    const FilePath texturePath(texturePathName);
    bool pathApplied = (FileSystem::Instance()->Exists(texturePath) && RETextureDescriptorUtils::CreateOrUpdateDescriptor(texturePath));

    if (!pathApplied)
    {
        texturePathName.clear();
    }
}

ColladaTexture::~ColladaTexture()
{
}

bool ColladaTexture::PreLoad()
{
    return true;
}
};
