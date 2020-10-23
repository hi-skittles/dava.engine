#include "PathDescriptor.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Image/ImageFormatInterface.h"
#include "Render/TextureDescriptor.h"
#include "Render/Highlevel/Heightmap.h"

DAVA::Vector<PathDescriptor> PathDescriptor::descriptors;

void PathDescriptor::InitializePathDescriptors()
{
    descriptors.push_back(PathDescriptor("", "All (*.*)", PathDescriptor::PATH_NOT_SPECIFIED));
    descriptors.push_back(PathDescriptor("customGeometry", "All (*.sc2);;SC2 (*.sc2);", PathDescriptor::PATH_SCENE));
    descriptors.push_back(PathDescriptor("textureSheet", "All (*.tex);;TEX (*.tex)", PathDescriptor::PATH_TEXTURE_SHEET));

    QString sourceFileString;
    QString separateSourceFileString;

    for (auto formatType : DAVA::TextureDescriptor::sourceTextureTypes)
    {
        QString fileTypeString;

        auto extensions = DAVA::ImageSystem::GetExtensionsFor(formatType);

        for (auto& ex : extensions)
        {
            if (fileTypeString.isEmpty())
            {
                fileTypeString = QString::fromStdString(DAVA::ImageSystem::GetImageFormatInterface(formatType)->GetName()) + " (*";
            }
            else
            {
                fileTypeString += QString(" *");
            }
            fileTypeString += QString(ex.c_str());

            if (sourceFileString.isEmpty())
            {
                sourceFileString = "*";
            }
            else
            {
                sourceFileString += " *";
            }
            sourceFileString += ex.c_str();
        }

        fileTypeString += ")";

        if (!separateSourceFileString.isEmpty())
        {
            separateSourceFileString += QString(";;");
        }
        separateSourceFileString += fileTypeString;
    }

    QString imageFilter = QString("All (%1);;%2").arg(sourceFileString).arg(separateSourceFileString);

    auto texExtension = DAVA::TextureDescriptor::GetDescriptorExtension();
    QString textureFilter = QString("All (*%1 %2);;TEX (*%3);;%4").arg(texExtension.c_str()).arg(sourceFileString).arg(texExtension.c_str()).arg(separateSourceFileString);
    auto heightExtension = DAVA::Heightmap::FileExtension();
    QString heightmapFilter = QString("All (*%1 %2);;Heightmap (*%3);;%4").arg(heightExtension.c_str()).arg(sourceFileString).arg(heightExtension.c_str()).arg(separateSourceFileString);

    descriptors.push_back(PathDescriptor("heightmapPath", heightmapFilter, PathDescriptor::PATH_HEIGHTMAP));
    descriptors.push_back(PathDescriptor("densityMap", imageFilter, PathDescriptor::PATH_IMAGE));
    descriptors.push_back(PathDescriptor("texture", textureFilter, PathDescriptor::PATH_TEXTURE));
    descriptors.push_back(PathDescriptor("lightmap", textureFilter, PathDescriptor::PATH_TEXTURE));
    descriptors.push_back(PathDescriptor("densityMap", textureFilter, PathDescriptor::PATH_TEXTURE));
    descriptors.push_back(PathDescriptor("Decal albedo", textureFilter, PathDescriptor::PATH_TEXTURE));
    descriptors.push_back(PathDescriptor("Decal normal", textureFilter, PathDescriptor::PATH_TEXTURE));
}

PathDescriptor& PathDescriptor::GetPathDescriptor(PathDescriptor::eType type)
{
    for (auto& descr : descriptors)
    {
        if (descr.pathType == type)
        {
            return descr;
        }
    }

    return GetPathDescriptor(PATH_NOT_SPECIFIED);
}
