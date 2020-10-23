#!/usr/bin/env python
#

import sys
import os
import platform
import subprocess
import shutil
import argparse

class PlatformValues:
    frameworkPath = ""
    toolsetPath = ""
    toolsetBuildPath = ""
    toolsetBinaryPath = ""
    cmakeGenerator = ""
    cmakePath = ""
    cmakeConfiguration = ""

    reBinaryPathname = ""
    qeBinaryPathname = ""

    def dump(self):
        print ["Dump values: "]
        print ["frameworkPath: ", self.frameworkPath]
        print ["toolsetPath: ", self.toolsetPath]
        print ["toolsetBuildPath: ", self.toolsetBuildPath]
        print ["toolsetBinaryPath: ", self.toolsetBinaryPath]
        print ["cmakeGenerator: ", self.cmakeGenerator]
        print ["cmakePath: ", self.cmakePath]
        print ["cmakeConfiguration: ", self.cmakeConfiguration]

        print ["reBinaryPathname: ", self.reBinaryPathname]
        print ["qeBinaryPathname: ", self.qeBinaryPathname]


def get_platform():
    platformName = platform.system()
    print ["platformName: ", platformName]

    windowsNames = ["win32", "Windows"]
    if platformName == "Darwin":
        return "Mac"
    elif platformName in windowsNames:
        return "Win"

    return ""


def get_mac_values(values = PlatformValues):
    values.cmakeGenerator = "Xcode"
    values.cmakePath = os.path.join(values.frameworkPath, "Bin/CMake.app/Contents/bin/cmake")
    values.cmakeConfiguration = "Release"
    values.reBinaryPathname = os.path.join(values.toolsetBinaryPath, "ResourceEditor.app/Contents/MacOS/ResourceEditor")
    values.qeBinaryPathname = os.path.join(values.toolsetBinaryPath, "QuickEd.app/Contents/MacOS/QuickEd")
    return values


def get_windows_values(values = PlatformValues):
    values.cmakeGenerator = "Visual Studio 12 Win64"
    values.cmakePath = os.path.join(values.frameworkPath, "Bin/cmake/bin/cmake.exe")
    values.cmakeConfiguration = "RelWithDebinfo"
    values.reBinaryPathname = os.path.join(values.toolsetBinaryPath, "ResourceEditor.exe")
    values.qeBinaryPathname = os.path.join(values.toolsetBinaryPath, "QuickEd.exe")
    return values


def create_folder(folderPath):
    if os.path.exists(folderPath):
        shutil.rmtree(folderPath)
    os.makedirs(folderPath)


def create_toolset(values = PlatformValues):
    commandLine = [values.cmakePath, "-G", values.cmakeGenerator, values.toolsetPath, "-DUNITY_BUILD=true",
                       "-B" + values.toolsetBuildPath, "-DDEPLOY=true", "-DONLY_CONTENT_TOOLS=true",
                       "-DDEPLOY_DIR=" + values.toolsetBinaryPath]
    print "create_toolset: ", commandLine
    sys.stdout.flush()
    subprocess.call(commandLine)


def build_toolset(values = PlatformValues):
    commandLine = ["cmake", "--build", values.toolsetBuildPath, "--config", values.cmakeConfiguration]
    print "build_toolset: ", commandLine
    sys.stdout.flush()
    subprocess.call(commandLine)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--generate', dest='generateToolset', action='store_true')
    parser.add_argument('--no-generate', dest='generateToolset', action='store_false')
    parser.set_defaults(generateToolset=True)

    parser.add_argument('--build', dest='buildToolset', action='store_true')
    parser.add_argument('--no-build', dest='buildToolset', action='store_false')
    parser.set_defaults(buildToolset=True)

    parser.add_argument('--test', dest='selfTest', action='store_true')
    parser.add_argument('--no-test', dest='selfTest', action='store_false')
    parser.set_defaults(selfTest=False)
    args = parser.parse_args()
    print args

    platform = get_platform()
    values = PlatformValues()
    executablePath = os.path.dirname(sys.argv[0])
    values.frameworkPath = os.path.abspath(executablePath)
    values.toolsetPath = os.path.join(values.frameworkPath,"Programs/Toolset")
    values.toolsetBuildPath = os.path.join(values.toolsetPath, "_build")
    values.toolsetBinaryPath = os.path.join(values.frameworkPath, "Bin/Toolset").replace("\\","/")

    if platform == "Win":
        values = get_windows_values(values)
    elif platform == "Mac":
        values = get_mac_values(values)
    else:
        print "Error: Cannot detect platform"
        exit()

    values.dump()
    sys.stdout.flush()

    if args.generateToolset:
        create_folder(values.toolsetBuildPath)
        create_folder(values.toolsetBinaryPath)
        create_toolset(values)

    if args.buildToolset:
        build_toolset(values)

    if args.selfTest:
        print "Run Tests:"
        commandLineRE = [values.reBinaryPathname, "--selftest"]
        print commandLineRE
        sys.stdout.flush()
        subprocess.call(commandLineRE)

        commandLineQE = [values.reBinaryPathname, "--selftest"]
        print commandLineQE
        sys.stdout.flush()
        subprocess.call(commandLineQE)





