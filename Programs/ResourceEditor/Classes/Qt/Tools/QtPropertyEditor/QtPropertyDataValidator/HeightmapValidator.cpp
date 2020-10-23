#include "HeightmapValidator.h"
#include "Utils/StringFormat.h"
#include <QMessageBox>

HeightMapValidator::HeightMapValidator(const QStringList& value)
    : PathValidator(value)
    , notifyMessage("")
{
}

void HeightMapValidator::ErrorNotifyInternal(const QVariant& v) const
{
    QMessageBox::warning(NULL, "Wrong file selected", notifyMessage.c_str(), QMessageBox::Ok);
}

bool HeightMapValidator::ValidateInternal(const QVariant& v)
{
    if (!PathValidator::ValidateInternal(v))
    {
        notifyMessage = PrepareErrorMessage(v);
        return false;
    }

    DAVA::FilePath path(v.toString().toStdString());
    if (path.IsEmpty() || path.IsEqualToExtension(".heightmap"))
    {
        return true;
    }
    else
    {
        auto extension = path.GetExtension();
        auto imageFormat = DAVA::ImageSystem::GetImageFormatForExtension(extension);

        if (DAVA::IMAGE_FORMAT_UNKNOWN != imageFormat)
        {
            auto imgSystem = DAVA::ImageSystem::GetImageFormatInterface(imageFormat);
            DAVA::Size2i size = imgSystem->GetImageInfo(path).GetImageSize();
            if (size.dx != size.dy)
            {
                notifyMessage = DAVA::Format("\"%s\" has wrong size: landscape requires square heightmap.",
                                             path.GetAbsolutePathname().c_str());
                return false;
            }

            if (!DAVA::IsPowerOf2(size.dx))
            {
                notifyMessage = DAVA::Format("\"%s\" has wrong size: landscape requires square heightmap with size 2^n.",
                                             path.GetAbsolutePathname().c_str());
                return false;
            }

            DAVA::Vector<DAVA::Image*> imageVector;
            DAVA::ImageSystem::Load(path, imageVector);
            DVASSERT(imageVector.size());

            DAVA::PixelFormat format = imageVector[0]->GetPixelFormat();

            for_each(imageVector.begin(), imageVector.end(), DAVA::SafeRelease<DAVA::Image>);
            if (format == DAVA::FORMAT_A8 || format == DAVA::FORMAT_A16)
            {
                return true;
            }
            notifyMessage = DAVA::Format("\"%s\" is wrong: png file should be in format A8 or A16.", path.GetAbsolutePathname().c_str());
        }
        else
        {
            notifyMessage = DAVA::Format("\"%s\" is wrong: should be *.png, *.tga, *.jpeg or *.heightmap.", path.GetAbsolutePathname().c_str());
        }
    }

    return false;
}
