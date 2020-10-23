#!/usr/bin/python
import sys
import subprocess
import os
import re
import platform
import argparse
from subprocess import call

g_engine_dir_name="dava.framework"
g_framework_path = ""
g_toolchains_full_path = ""
g_toolchains_relative_framework_path = "Sources/CMake/Toolchains/"
g_ios_toolchain = "ios.toolchain.cmake"
g_android_toolchain = "android.toolchain.cmake"

g_cmake_file_path = ""
g_generation_dir = ""
g_supported_platforms = ["macos", "ios", "android", "windows"]
g_supported_additional_parameters = ["console", "uap", "x64"]
g_is_console = False
g_is_uap = False
g_is_x64 = False
g_is_unity_build = False

def is_exe(fpath):
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

def get_cmake_executable():
    host=sys.platform
    cmake_executable=None
    if host == "win32":
        cmake_executable=os.path.join(g_framework_path, 'Bin', 'CMakeWin32', 'bin', 'cmake.exe')
    elif host == "darwin":
        cmake_executable=os.path.join(g_framework_path, 'Bin', 'CMakeMac', 'CMake.app', 'Contents', 'bin', 'cmake')

    if not os.path.isfile(cmake_executable):
        cmake_executable=None

    if cmake_executable is not None:
        return cmake_executable
    return False


def parse_additional_params(additional):
    global g_is_console
    global g_is_uap
    global g_is_x64
    global g_is_unity_build

    if not additional:
        return True;

    for param in additional:
        param = param.lower()

        if "console" == param:
            g_is_console = True
        elif "uap" == param:
            g_is_uap = True
        elif "x64" == param:
            g_is_x64 = True
        elif "ub" == param:
            g_is_unity_build = True                
        else:
            print "Unsupported additional parameter " + "'" + param + "'" + " Use combination of " + str(g_supported_additional_parameters)
            return False

    return True


def setup_framework_env():
    global g_framework_path
    global g_toolchains_full_path
    global g_toolchains_relative_framework_path
    global g_cmake_file_path

    script_path=os.path.abspath(__file__)
    i=script_path.find(g_engine_dir_name)
    if i == -1:
        return False

    g_framework_path=script_path[0:i+len(g_engine_dir_name)]
    g_toolchains_full_path = os.path.realpath(os.path.join(g_framework_path, g_toolchains_relative_framework_path))

    return True


def get_project_type(dst_platform, is_console):
    dst_platform = dst_platform.lower()
    project_string = ""

    if "macos" == dst_platform or "ios" == dst_platform:
        project_string += "Xcode"

    if "windows" == dst_platform:
        project_string += "Visual Studio 15 2017"

    if "android" == dst_platform:
        current_platform = platform.system()

        if not g_is_console:
            project_string += "Eclipse CDT4 - "

        if "MinGW" == current_platform:
            project_string += "Mingw Makefiles"
        elif "Windows" == current_platform:
            project_string += "NMake Makefiles"
        else:
            project_string += "Unix Makefiles"

    return project_string


def get_toolchain(input_platform, input_project_type):
    global g_ios_toolchain
    global g_android_toolchain
    global g_is_x64

    toolchain_base = "-DCMAKE_TOOLCHAIN_FILE="
    toolchain_string = ""
    output_project = input_project_type

    if "ios" == input_platform:
        toolchain_string = toolchain_base + os.path.join(g_toolchains_full_path, g_ios_toolchain)

    if "android" == input_platform:
        toolchain_string = toolchain_base + os.path.join(g_toolchains_full_path, g_android_toolchain)

    if "windows" == input_platform and g_is_x64:
        output_project += " Win64";

    return toolchain_string, output_project

def safe_call(command):
    ret = call(command)
    if ret != 0:
        print "Error executing command: %s" % (command)
        exit(1)

def main():
    global g_supported_additional_parameters
    global g_supported_platforms

    parser = argparse.ArgumentParser(description='Dava.Framework projects genarator')
    parser.add_argument('platform_name', help='One of ' + str(g_supported_platforms))
    parser.add_argument('additional_params', nargs='*', help= 'One of ' + str(g_supported_additional_parameters))
    parser.add_argument('cmake_path', help='relative path to cmake list')
    parser.add_argument('--generation_dir', default="", help="path to generation cmake list" )
    parser.add_argument('--add_definitions', '-defs', default="", help="add definitions" )
    parser.add_argument('--x64', default=False )
    parser.add_argument('-D', action='append', default=[], help="add definitions" )

    options = parser.parse_args()

    if not setup_framework_env():
        print "Couldn't configure environment. Make sure that you run this script from dava.framework subfolder."
        exit(1)

    destination_platform = ""

    g_is_x64          = options.x64
    g_cmake_file_path = os.path.realpath(options.cmake_path)
    g_generation_dir  = options.generation_dir
    g_add_definitions = options.add_definitions.replace(',',' ')

    if options.platform_name not in g_supported_platforms:
        print "Wrong destination OS name " + "'" + options.platform_name + "'"
        parser.print_help()
        exit(1);
    else:
        destination_platform = options.platform_name.lower()

    if False == parse_additional_params(options.additional_params):
        parser.print_help()
        exit(1)

    project_type = get_project_type(destination_platform, g_is_console)
    if project_type == "":
        print "Unknown project type. Seems get_project_type() works wrong."
        exit(1)


    toolchain, project_type = get_toolchain(destination_platform, project_type)

    g_cmake_file_path = os.path.realpath(options.cmake_path)

    g_generation_dir  = options.generation_dir
    g_add_definitions = options.add_definitions.replace(',',' ')

    if len(g_generation_dir) :
        if not os.path.exists(g_generation_dir):
            os.makedirs(g_generation_dir)
        os.chdir( g_generation_dir )

    cmake_program = get_cmake_executable()

    if False == cmake_program:
        print "cmake command not found."
        exit(1)

    call_string = [cmake_program, '-G', project_type, toolchain, g_cmake_file_path]

    if len(options.add_definitions):
        call_string += options.add_definitions.split(',') 

    if len(options.D):
        call_string += map(lambda val: '=' in val and '-D'+val or '-D'+val+'=true', options.D )
    
    if g_is_unity_build:
        call_string.append("-DUNITY_BUILD=true")
    print call_string

    safe_call(call_string)
    
    if "android" == destination_platform:
        safe_call(call_string)    

if __name__ == '__main__':
    main()

