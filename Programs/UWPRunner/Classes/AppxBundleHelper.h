#ifndef APPX_BUNDLE_HELPER_H
#define APPX_BUNDLE_HELPER_H

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"

class AppxBundleHelper
{
public:
    struct PackageInfo
    {
        DAVA::String name;
        DAVA::String architecture;
        bool isApplication;
        DAVA::FilePath path;
    };

    AppxBundleHelper(const DAVA::FilePath& fileName);
    ~AppxBundleHelper();

    void RemoveFiles();

    static bool IsBundle(const DAVA::FilePath& fileName);
    const DAVA::Vector<PackageInfo>& GetPackages() const;
    DAVA::Vector<PackageInfo> GetApplications() const;
    DAVA::Vector<PackageInfo> GetResources() const;

    DAVA::FilePath GetApplication(const DAVA::String& name);
    DAVA::FilePath GetApplicationForArchitecture(const DAVA::String& name);
    DAVA::FilePath GetResource(const DAVA::String& name);

private:
    void ParseBundleManifest();

    DAVA::FilePath bundlePackageDir;
    DAVA::Vector<PackageInfo> storedPackages;
};

#endif // APPX_BUNDLE_HELPER_H