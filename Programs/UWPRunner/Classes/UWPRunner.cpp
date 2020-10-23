#include <iostream>
#include <QFile>
#include <QXmlStreamReader>

#include "Base/Exception.h"
#include "Concurrency/Thread.h"
#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "Functional/Function.h"
#include "Network/NetConfig.h"
#include "Platform/TemplateWin32/UAPNetworkHelper.h"
#include "Logger/TeamCityTestsOutput.h"
#include "Utils/UTF8Utils.h"

#include "AppxBundleHelper.h"
#include "ArchiveExtraction.h"
#include "SvcHelper.h"
#include "runner.h"
#include "RegKey.h"
#include "UWPLogConsumer.h"
#include "UWPRunner.h"

#include <LoggerService/LogConsumer.h>
#include <LoggerService/ServiceInfo.h>

namespace UWPRunnerDetails
{
using namespace DAVA;

String GetCurrentArchitecture()
{
    return sizeof(void*) == 4 ? "x86" : "x64";
}

QString GetQtWinRTRunnerProfile(const String& profile, const FilePath& manifest)
{
    //if profile is set, just convert it
    if (!profile.empty())
    {
        return profile == "local" ? QStringLiteral("appx") : QStringLiteral("appxphone");
    }

    //else try to find out profile from manifest
    QFile file(QString::fromStdString(manifest.GetAbsolutePathname()));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement ||
            xml.name() != QStringLiteral("Identity"))
        {
            continue;
        }

        QXmlStreamAttributes attributes = xml.attributes();
        for (const auto& attribute : attributes)
        {
            if (attribute.name() == QStringLiteral("ProcessorArchitecture"))
            {
                QString arch = attribute.value().toString().toLower();
                if (arch == QStringLiteral("arm"))
                {
                    return QStringLiteral("appxphone");
                }
                else
                {
                    return QStringLiteral("appx");
                }
            }
        }
    }

    return "";
}

FilePath ExtractManifest(const FilePath& package)
{
    String manifestFilePath = package.GetAbsolutePathname() + "_manifest.xml";

    //extract manifest from appx
    if (ExtractFileFromArchive(package.GetAbsolutePathname(),
                               "AppxManifest.xml",
                               manifestFilePath))
    {
        return manifestFilePath;
    }
    return FilePath();
}
}

UWPRunner::UWPRunner(const PackageOptions& opt)
    : options(opt)
{
    //install slot to log consumer
    logConsumer.newMessageNotifier.Connect(this, [this](const DAVA::String& logStr) {
        NetLogOutput(logStr);
    });
}

UWPRunner::~UWPRunner()
{
    cleanNeeded.Emit();
    logConsumer.newMessageNotifier.Disconnect(this);

    if (!options.installOnly)
    {
        UnInitializeNetwork();
    }
}

void UWPRunner::Run()
{
    if (GetEngineContext()->netCore == nullptr)
    {
        DAVA_THROW(DAVA::Exception, "NetCore module is not created");
    }

    //Create Qt runner
    DAVA::Logger::Info("Preparing to launch...");
    ProcessPackageOptions();
    ProcessProfileInfo();

    //Init network
    if (!options.installOnly)
    {
        DAVA::Logger::Info("Initializing network...");
        InitializeNetwork(qtProfile == "appxphone");
    }

    QStringList resources;
    for (const auto& x : options.resources)
    {
        resources.push_back(QString::fromStdString(x));
    }

    Runner runner(QString::fromStdString(options.mainPackage),
                  QString::fromStdString(options.packageToInstall),
                  resources,
                  QString::fromStdString(options.dependencies),
                  QStringList(),
                  QString::fromStdString(qtProfile));

    //Check runner state
    if (!runner.isValid())
    {
        DAVA_THROW(DAVA::Exception, "Runner core is not valid");
    }

    Run(runner);
}

bool UWPRunner::IsSucceed()
{
    return succeed;
}

void UWPRunner::Run(Runner& runner)
{
    //installing and starting application
    if (!options.runOnly)
    {
        DAVA::Logger::Info("Installing package...");
        if (!runner.install(true))
        {
            DAVA_THROW(DAVA::Exception, "Can't install application package");
            return;
        }
    }

    if (options.installOnly)
    {
        succeed = true;
        return;
    }

    DAVA::Logger::Info("Starting application...");
    if (!runner.start())
    {
        DAVA_THROW(DAVA::Exception, "Can't install application package");
        return;
    }

    //post-start cleaning
    cleanNeeded.Emit();

    //wait application exit
    WaitApp();
}

void UWPRunner::WaitApp()
{
    const size_t connectionTimeout = 5 * 60 * 1000;
    const DAVA::uint32 sleepTimeMS = 10;
    size_t watchDogTimer = 0;

    DAVA::Logger::Info("Waiting application exit...");

    do
    {
        GetEngineContext()->netCore->Poll();

        if (logConsumer.IsChannelOpen())
        {
            succeed = true;
            watchDogTimer = 0;
        }
        else
        {
            watchDogTimer += sleepTimeMS;
            if (watchDogTimer >= connectionTimeout)
            {
                DAVA::Logger::Error("Cannot connect to application");
                break;
            }
        }

        DAVA::Thread::Sleep(sleepTimeMS);
    } while (!logConsumer.IsSessionEnded());
}

void UWPRunner::ProcessPackageOptions()
{
    //if package is bundle, extract concrete package from it
    if (AppxBundleHelper::IsBundle(options.mainPackage))
    {
        ProcessBundlePackage();
    }
    else
    {
        options.packageToInstall = options.mainPackage;
    }
}

void UWPRunner::ProcessBundlePackage()
{
    DAVA::FilePath package = options.mainPackage;
    bundleHelper.reset(new AppxBundleHelper(package));
    cleanNeeded.Connect([this] { bundleHelper.reset(); });

    //try to extract package for specified architecture
    if (!options.architecture.empty())
    {
        package = bundleHelper->GetApplicationForArchitecture(options.architecture);
    }
    //try to extract package for current architecture
    else
    {
        package = bundleHelper->GetApplicationForArchitecture(UWPRunnerDetails::GetCurrentArchitecture());

        //try to extract package for any architecture
        if (package.IsEmpty())
        {
            DAVA::Vector<AppxBundleHelper::PackageInfo> applications = bundleHelper->GetApplications();
            package = bundleHelper->GetApplication(applications.at(0).name);
        }
    }

    if (!package.IsEmpty())
    {
        DAVA::Vector<AppxBundleHelper::PackageInfo> resources = bundleHelper->GetResources();
        for (const auto& x : resources)
        {
            options.resources.push_back(x.path.GetAbsolutePathname());
        }

        options.packageToInstall = package.GetAbsolutePathname();
    }
    else
    {
        DAVA_THROW(DAVA::Exception, "Can't extract app package from bundle");
    }
}

void UWPRunner::ProcessProfileInfo()
{
    //Extract manifest from package
    DAVA::Logger::Info("Extracting manifest...");
    DAVA::FilePath manifest = UWPRunnerDetails::ExtractManifest(options.packageToInstall);
    if (manifest.IsEmpty())
    {
        DAVA_THROW(DAVA::Exception, "Can't extract manifest file from package");
    }

    //figure out if app should be started on mobile device
    qtProfile = UWPRunnerDetails::GetQtWinRTRunnerProfile(options.profile, manifest).toStdString();
    DAVA::FileSystem::Instance()->DeleteFile(manifest);
}

void UWPRunner::InitializeNetwork(bool isMobileDevice)
{
    using namespace DAVA;
    using namespace Net;

    if (isMobileDevice)
    {
        bool ipOverUsbConfigured = ConfigureIpOverUsb();
        if (!ipOverUsbConfigured)
        {
            DAVA_THROW(Exception, "Cannot configure IpOverUSB service");
        }
    }

    auto logCreator = [this](uint32, void*) -> IChannelListener* { return &logConsumer; };
    auto logDestroyer = [](IChannelListener* obj, void*) {};
    GetEngineContext()->netCore->RegisterService(LOG_SERVICE_ID, logCreator, logDestroyer);

    eNetworkRole role;
    Endpoint endPoint;
    if (isMobileDevice)
    {
        role = CLIENT_ROLE;
        endPoint = Endpoint(UAPNetworkHelper::UAP_IP_ADDRESS, UAPNetworkHelper::UAP_MOBILE_TCP_PORT);
    }
    else
    {
        role = SERVER_ROLE;
        endPoint = Endpoint(UAPNetworkHelper::UAP_DESKTOP_TCP_PORT);
    }

    NetConfig config(role);
    config.AddTransport(TRANSPORT_TCP, endPoint);
    config.AddService(LOG_SERVICE_ID);

    const uint32 timeout = 5 * 60 * 1000; //5 min
    controllerId = GetEngineContext()->netCore->CreateController(config, nullptr, timeout);
}

void UWPRunner::UnInitializeNetwork()
{
    if (controllerId != DAVA::Net::NetCore::INVALID_TRACK_ID)
    {
        DAVA::Net::NetCore* netCore = GetEngineContext()->netCore;
        netCore->DestroyControllerBlocked(controllerId);
        netCore->UnregisterService(DAVA::Net::LOG_SERVICE_ID);
        controllerId = DAVA::Net::NetCore::INVALID_TRACK_ID;
    }
}

bool UWPRunner::UpdateIpOverUsbConfig(RegKey& key)
{
    using namespace DAVA;

    const WideString desiredAddr = UTF8Utils::EncodeToWideString(UAPNetworkHelper::UAP_IP_ADDRESS);
    const DWORD desiredPort = UAPNetworkHelper::UAP_MOBILE_TCP_PORT;
    bool changed = false;

    WideString address = key.QueryString(L"DestinationAddress");
    if (address != desiredAddr)
    {
        if (!key.SetValue(L"DestinationAddress", desiredAddr))
        {
            DAVA_THROW(Exception, "Unable to set DestinationAddress");
        }
        changed = true;
    }

    DWORD port = key.QueryDWORD(L"DestinationPort");
    if (port != desiredPort)
    {
        if (!key.SetValue(L"DestinationPort", desiredPort))
        {
            DAVA_THROW(Exception, "Unable to set DestinationPort");
        }
        changed = true;
    }

    address = key.QueryString(L"LocalAddress");
    if (address != desiredAddr)
    {
        if (!key.SetValue(L"LocalAddress", desiredAddr))
        {
            DAVA_THROW(Exception, "Unable to set LocalAddress");
        }
        changed = true;
    }

    port = key.QueryDWORD(L"LocalPort");
    if (port != desiredPort)
    {
        if (!key.SetValue(L"LocalPort", desiredPort))
        {
            DAVA_THROW(Exception, "Unable to set LocalPort");
        }
        changed = true;
    }

    return changed;
}

bool UWPRunner::RestartIpOverUsb()
{
    //open service
    SvcHelper service(L"IpOverUsbSvc");
    if (!service.IsInstalled())
    {
        DAVA_THROW(DAVA::Exception, "Can't open IpOverUsb service");
    }

    //stop it
    if (!service.Stop())
    {
        DAVA_THROW(DAVA::Exception, "Can't stop IpOverUsb service");
    }

    //start it
    if (!service.Start())
    {
        DAVA_THROW(DAVA::Exception, "Can't start IpOverUsb service");
    }

    return true;
}

bool UWPRunner::ConfigureIpOverUsb()
{
    bool needRestart = false;

    //open or create key
    RegKey key(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\IpOverUsbSdk\\DavaDebugging", true);
    if (!key.IsExist())
    {
        DAVA_THROW(DAVA::Exception, "Can't open or create key");
    }
    needRestart |= key.IsCreated();

    //update config values
    bool result = UpdateIpOverUsbConfig(key);
    needRestart |= result;

    //restart service to applying new config
    if (needRestart)
        return RestartIpOverUsb();
    return true;
}

void SplitLoggerMessage(const DAVA::String& logString, DAVA::String& logLevel, DAVA::String& message)
{
    size_t spaces = 0;
    for (auto i : logString)
    {
        if (::isspace(i))
        {
            spaces++;
        }

        if (spaces == 3)
        {
            logLevel += i;
        }
        else if (spaces >= 4)
        {
            message += i;
        }
    }
}

void TeamcityTestOutputFunc(const char* logLevelStr, const char* messageStr)
{
    DAVA::Logger* logger = DAVA::GetEngineContext()->logger;
    DAVA::Logger::eLogLevel ll = logger->GetLogLevelFromString(logLevelStr);

    if (ll != DAVA::Logger::LEVEL__DISABLE)
    {
        DAVA::TeamcityTestsOutput testOutput;
        testOutput.Output(ll, messageStr);
    }
}

void UWPRunner::NetLogOutput(const DAVA::String& logString)
{
    using namespace DAVA;

    //incoming string is formatted in style "[ip:port] date time message"
    //extract only message text
    String logLevel;
    String message;
    SplitLoggerMessage(logString, logLevel, message);

    if (logLevel.empty())
    {
        return;
    }

    //remove first space
    const char* logLevelStr = logLevel.c_str() + 1;
    const char* messageStr = message.c_str() + 1;

    if (options.useTeamCityTestOutput)
    {
        TeamcityTestOutputFunc(logLevelStr, messageStr);
    }
    else if (!options.useTeamCityTestOutput || !options.outputFile.empty())
    {
        StringStream ss;
        ss << "[" << logLevelStr << "] " << messageStr;
        if (message.back() != '\n' || message.back() != '\r')
        {
            ss << "\n";
        }

        std::cout << ss.str();

        if (!options.outputFile.empty())
        {
            if (!outputFile)
            {
                FileSystem::Instance()->DeleteFile(options.outputFile);
                outputFile.Set(File::Create(options.outputFile, File::WRITE));
            }

            if (outputFile)
            {
                outputFile->WriteString(ss.str(), false);
                outputFile->Flush();
            }
        }
    }
}

const DAVA::EngineContext* UWPRunner::GetEngineContext() const
{
    return DAVA::Engine::Instance()->GetContext();
}
