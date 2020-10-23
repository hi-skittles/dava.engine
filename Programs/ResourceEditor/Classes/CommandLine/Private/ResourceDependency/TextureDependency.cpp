#include "Classes/CommandLine/Private/ResourceDependency/TextureDependency.h"
#include "Classes/CommandLine/Private/ResourceDependency/ResourceDependencyConstants.h"

#include <Render/TextureDescriptor.h>
#include <Render/TextureDescriptorUtils.h>

#include <memory>

bool TextureDependency::GetDependencies(const DAVA::FilePath& texturePath, DAVA::Set<DAVA::FilePath>& dependencies, DAVA::int32 requestedType)
{
    using namespace DAVA;

    if (requestedType == static_cast<int32>(eDependencyType::CONVERT))
    {
        std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texturePath));
        if (descriptor)
        {
            if (descriptor->IsCubeMap())
            {
                Vector<FilePath> facePathnames;
                descriptor->GetFacePathnames(facePathnames);
                dependencies.insert(facePathnames.begin(), facePathnames.end());
            }
            else
            {
                dependencies.insert(descriptor->GetSourceTexturePathname());
            }

            return true;
        }
    }
    else if (requestedType == static_cast<int32>(eDependencyType::DOWNLOAD))
    { //right now we don't have dependencies for downloading of textures
        return true;
    }

    return false;
};
