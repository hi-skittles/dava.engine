#include "AppxBundleHelper.h"

#include <QFile>
#include <QXmlStreamReader>

#include "Logger/Logger.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"

#include "ArchiveExtraction.h"

using namespace DAVA;

AppxBundleHelper::AppxBundleHelper(const FilePath& fileName)
{
    String dirName = fileName.GetBasename() + "_files/";
    bundlePackageDir = fileName.GetDirectory() + dirName;

    Logger::Info("Extracting bundle...");
    bool result = ExtractAllFromArchive(fileName.GetAbsolutePathname(),
                                        bundlePackageDir.GetAbsolutePathname());

    DVASSERT(result, "Failed to extract files from bundle");

    if (result)
    {
        ParseBundleManifest();
    }
}

void AppxBundleHelper::RemoveFiles()
{
    if (bundlePackageDir.Exists())
    {
        FileSystem::Instance()->DeleteDirectory(bundlePackageDir);
    }
}

AppxBundleHelper::~AppxBundleHelper()
{
    RemoveFiles();
}

bool AppxBundleHelper::IsBundle(const FilePath& fileName)
{
    return fileName.GetExtension() == ".appxbundle";
}

FilePath AppxBundleHelper::GetApplication(const String& name)
{
    for (const auto& packageInfo : storedPackages)
    {
        if (packageInfo.isApplication && packageInfo.name == name)
        {
            return packageInfo.path;
        }
    }
    return FilePath();
}

FilePath AppxBundleHelper::GetApplicationForArchitecture(const String& name)
{
    String architecture = name;
    std::transform(architecture.begin(), architecture.end(), architecture.begin(), ::tolower);

    for (const auto& packageInfo : storedPackages)
    {
        if (packageInfo.isApplication && packageInfo.architecture == architecture)
        {
            return packageInfo.path;
        }
    }
    return FilePath();
}

const Vector<AppxBundleHelper::PackageInfo>& AppxBundleHelper::GetPackages() const
{
    return storedPackages;
}

Vector<AppxBundleHelper::PackageInfo> AppxBundleHelper::GetApplications() const
{
    Vector<PackageInfo> applications;

    for (const auto& packageInfo : storedPackages)
    {
        if (packageInfo.isApplication)
        {
            applications.push_back(packageInfo);
        }
    }

    return applications;
}

Vector<AppxBundleHelper::PackageInfo> AppxBundleHelper::GetResources() const
{
    Vector<PackageInfo> resources;

    for (const auto& packageInfo : storedPackages)
    {
        if (!packageInfo.isApplication)
        {
            resources.push_back(packageInfo);
        }
    }

    return resources;
}

void AppxBundleHelper::ParseBundleManifest()
{
    FilePath manifest = bundlePackageDir + "AppxMetadata/AppxBundleManifest.xml";

    QFile file(QString::fromStdString(manifest.GetAbsolutePathname()));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement ||
            xml.name() != QStringLiteral("Package"))
        {
            continue;
        }

        QXmlStreamAttributes attributes = xml.attributes();
        PackageInfo packageInfo;
        packageInfo.architecture = "any";
        packageInfo.isApplication = false;

        for (const auto& attribute : attributes)
        {
            if (attribute.name() == QStringLiteral("Type"))
            {
                QString value = attribute.value().toString().toLower();
                packageInfo.isApplication = value == QStringLiteral("application");
            }
            else if (attribute.name() == QStringLiteral("Architecture"))
            {
                packageInfo.architecture = attribute.value().toString().toLower().toStdString();
            }
            else if (attribute.name() == QStringLiteral("FileName"))
            {
                String fileName = attribute.value().toString().toStdString();
                packageInfo.name = fileName;
                packageInfo.path = bundlePackageDir + fileName;
            }
        }
        storedPackages.emplace_back(std::move(packageInfo));
    }
}