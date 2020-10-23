#ifndef UWP_RUNNER_H
#define UWP_RUNNER_H

#include "Base/BaseTypes.h"
#include "FileSystem/File.h"
#include "FileSystem/FilePath.h"
#include "Network/NetCore.h"
#include "UWPLogConsumer.h"
#include "AppxBundleHelper.h"

struct PackageOptions
{
    //TODO: replace on Optional
    DAVA::String mainPackage;
    DAVA::String packageToInstall;
    DAVA::String architecture;
    DAVA::String profile;
    DAVA::String dependencies;
    DAVA::String outputFile;
    DAVA::Vector<DAVA::String> resources;
    bool useTeamCityTestOutput = false;
    bool installOnly = false;
    bool runOnly = false;
    bool isDavaApplication = false;
};
PackageOptions ParseCommandLine();
bool CheckOptions(const PackageOptions& options);

class Runner;
class RegKey;

class AppxBundleHelper;
class UWPRunner
{
public:
    UWPRunner(const PackageOptions& opt);
    ~UWPRunner();

    void Run();
    bool IsSucceed();

private:
    void Run(Runner& runner);
    void WaitApp();

    void ProcessPackageOptions();
    void ProcessBundlePackage();
    void ProcessProfileInfo();
    void InitializeNetwork(bool isMobileDevice);
    void UnInitializeNetwork();

    bool UpdateIpOverUsbConfig(RegKey& key);
    bool ConfigureIpOverUsb();
    bool RestartIpOverUsb();

    void NetLogOutput(const DAVA::String& logString);
    const DAVA::EngineContext* GetEngineContext() const;

    PackageOptions options;
    DAVA::Signal<> cleanNeeded;
    std::unique_ptr<AppxBundleHelper> bundleHelper;
    UWPLogConsumer logConsumer;
    DAVA::Net::NetCore::TrackId controllerId = DAVA::Net::NetCore::INVALID_TRACK_ID;
    DAVA::String qtProfile;
    DAVA::RefPtr<DAVA::File> outputFile;
    bool succeed = false;
};

#endif // UWP_RUNNER_H