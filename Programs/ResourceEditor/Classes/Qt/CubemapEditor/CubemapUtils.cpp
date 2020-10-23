#include "Classes/Qt/CubemapEditor/CubemapUtils.h"

#include <Render/Texture.h>
#include <Render/TextureDescriptor.h>

void CubemapUtils::GenerateFaceNames(const DAVA::String& baseName, DAVA::Vector<DAVA::FilePath>& faceNames)
{
    faceNames.clear();

    DAVA::FilePath filePath(baseName);

    std::unique_ptr<DAVA::TextureDescriptor> descriptor(new DAVA::TextureDescriptor());
    if (!descriptor->Load(filePath))
        return;

    descriptor->GetFacePathnames(faceNames);
}
