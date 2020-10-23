#include <iostream>
#include <QFileInfo>

#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "CommandLine/CommandLineParser.h"
#include "Utils/Utils.h"

#include "UWPRunner.h"

using namespace DAVA;

void ShowUsage();
PackageOptions ParseShortFormArgs(const String& packagePath);
PackageOptions ParseLongFormArgs(const Vector<String>& arguments);

PackageOptions ParseCommandLine()
{
    const Vector<String>& arguments = Engine::Instance()->GetCommandLine();

    //no args
    if (arguments.size() == 1)
    {
        return PackageOptions();
    }
    //short form (only file)
    else if (arguments.size() == 2)
    {
        return ParseShortFormArgs(arguments[1]);
    }
    //long form
    else
    {
        return ParseLongFormArgs(arguments);
    }
}

PackageOptions ParseShortFormArgs(const String& packagePath)
{
    Vector<String> packageInfo;
    Split(FilePath(packagePath).GetBasename(), "_", packageInfo);

    PackageOptions options;
    options.mainPackage = packagePath;
    options.dependencies = FilePath(packagePath).GetDirectory().GetAbsolutePathname();

    return options;
}

PackageOptions ParseLongFormArgs(const Vector<String>& arguments)
{
    PackageOptions out;

    CommandLineParser parser;
    parser.SetFlags(arguments);

    //parse parameters 'arch', 'package', 'profile' and 'dependencies'
    if (parser.IsFlagSet("--arch"))
    {
        out.architecture = parser.GetParamForFlag("--arch");
    }

    if (parser.IsFlagSet("--package"))
    {
        out.mainPackage = parser.GetParamForFlag("--package");
    }

    if (parser.IsFlagSet("--profile"))
    {
        String profile = parser.GetParamForFlag("--profile");
        std::transform(profile.begin(), profile.end(), profile.begin(), ::tolower);
        out.profile = std::move(profile);
    }

    if (parser.IsFlagSet("--dependencies"))
    {
        out.dependencies = parser.GetParamForFlag("--dependencies");
    }
    else if (!out.mainPackage.empty())
    {
        out.dependencies = FilePath(out.mainPackage).GetDirectory().GetAbsolutePathname();
    }

    if (parser.IsFlagSet("--tc_test"))
    {
        out.useTeamCityTestOutput = true;
    }

    if (parser.IsFlagSet("--install_only"))
    {
        out.installOnly = true;
    }

    if (parser.IsFlagSet("--run_only"))
    {
        out.runOnly = true;
    }

    if (parser.IsFlagSet("--dava_app"))
    {
        out.isDavaApplication = true;
    }

    if (parser.IsFlagSet("--output"))
    {
        out.outputFile = parser.GetParamForFlag("--output");
    }

    return out;
}

void ShowUsage()
{
    String message =
    "UWPRunner is a utility for installing, running and collection output "
    "of universal windows applications.\n"
    "UWPRunner may need administrative rights for configuring of IpOverUsb service\n"
    "Usage: \n"
    "    --package [path to appx package]\n"
    "    --dependencies [path to package dependencies dir]\n"
    "    --profile (local/phone) [target device for package]\n"
    "    --arch [architecture of launching package, only for bundle]\n"
    "    --tc_test [use teamcity test output]\n"
    "    --install_only [only install package]\n"
    "    --run_only [don't install, just run]\n"
    "    --dava_app [application based on DAVA Framework, detects its abnormal termination]\n"
    "    --output [path to output file for logs]\n";

    std::cout << message;
}

bool CheckPackageOption(const String& package)
{
    QFileInfo file(QString::fromStdString(package));

    if (package.empty())
    {
        std::cout << "Package file is not set" << std::endl;
        return false;
    }
    else if (!file.exists() || !file.isFile())
    {
        std::cout << "Cannot find the specified file: " << package << std::endl;
        return false;
    }

    return true;
}

bool CheckDependenciesOption(const String& dependencies)
{
    FileSystem* fs = FileSystem::Instance();

    if (dependencies.empty())
    {
        std::cout << "Package dependencies path is not set" << std::endl;
        return false;
    }
    else if (!fs->IsDirectory(dependencies))
    {
        std::cout << "Cannot find the specified path: " << dependencies << std::endl;
        return false;
    }

    return true;
}

bool CheckWorkFlags(const PackageOptions& options)
{
    bool workFlagsIsOk = !((options.runOnly == options.installOnly) && (options.runOnly == true));

    if (!workFlagsIsOk)
    {
        std::cout << "'Run only' and 'Install only' flags are both set" << std::endl;
    }

    return workFlagsIsOk;
}

bool CheckOptions(const PackageOptions& options)
{
    bool packageIsOk = CheckPackageOption(options.mainPackage);
    bool dependenciesIsOk = CheckDependenciesOption(options.dependencies);
    bool workFlagsIsOk = CheckWorkFlags(options);

    bool allIsOk = packageIsOk && dependenciesIsOk && workFlagsIsOk;

    if (!allIsOk)
    {
        ShowUsage();
    }

    return allIsOk;
}