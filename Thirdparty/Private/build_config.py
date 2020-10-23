import os

# Current variables to use for building libraries
# Change these when migrating over to new compilers, ides etc.
# Functions below are used by build scripts and should check these variables
# to return appropriate values
_win32_vs = "2017"
_win10_vs = "2015"

_win32_sdkver="10.0.14393.0"

_macos_deployment_target="10.8"

# ========================================================================

def get_android_api_version():
    return '16'

def get_android_platform():
    return 'android-16'

def get_android_stl():
    return 'c++_shared'

def get_android_libc():
    return 'libc++'

def get_gyp_msvs_version():
    if _win32_vs == "2013":
        return '2013'  # forces gyp to use v120
    elif _win32_vs == "2015":
        return '2015'
    elif _win32_vs == "2017":
        return '2017'


def get_msvc_toolset_name(vs_ver):
    if vs_ver == '2013':
        return 'v120'
    elif vs_ver == '2015':
        return 'v140'
    elif vs_ver == '2017':
        return 'v141'
    else:
        raise RuntimeError('Unknown Visual Studio version: {}'.format(vs_ver))


def get_msvc_toolset_ver_win32():
    return get_msvc_toolset_name(_win32_vs)


def get_msvc_sdk_version_win32():
    return _win32_sdkver


# Get these from registry?
_vc2013_path = 'C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC'
_vc2015_path = 'C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC'
_vc2017_path = 'C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional/VC/Auxiliary/Build'

_msbuild2013_path='c:/Program Files (x86)/MSBuild/12.0/Bin/MSBuild.exe'
_msbuild2015_path='c:/Program Files (x86)/MSBuild/14.0/Bin/MSBuild.exe'
_msbuild2017_path='c:/Program Files (x86)/Microsoft Visual Studio/2017/Professional/MSBuild/15.0/Bin/MSBuild.exe'

_cmake_path_win32='Bin/CMakeWin32/bin/cmake'
_cmake_path_macos='Bin/CMakeMac/CMake.app/Contents/bin/cmake'
_cmake_path_linux='Bin/CMakeLinux/bin/cmake'
_cmake_path=None


def configure(host, root_path):
    global _cmake_path
    print 'configure: host=%s, root_path=%s' % (host, root_path)
    if host=='win32':
        _cmake_path=os.path.join(root_path, _cmake_path_win32)
    elif host=='darwin':
        _cmake_path=os.path.join(root_path, _cmake_path_macos)
    elif host=='linux':
        _cmake_path=os.path.join(root_path, _cmake_path_linux)
    print 'configure:',_cmake_path


def get_cmake_executable():
    return _cmake_path


def get_msvc_path(vs_ver):
    if vs_ver == '2013':
        return _vc2013_path
    elif vs_ver == '2015':
        return _vc2015_path
    elif vs_ver == '2017':
        return _vc2017_path
    else:
        raise RuntimeError('Unknown Visual Studio version: {}'.format(vs_ver))


def get_msbuild_path(vs_ver):
    if vs_ver == '2013':
        return _msbuild2013_path
    elif vs_ver == '2015':
        return _msbuild2015_path
    elif vs_ver == '2017':
        return _msbuild2017_path
    else:
        raise RuntimeError('Unknown Visual Studio version: {}'.format(vs_ver))


def get_vs_vc_path_win32():
    return get_msvc_path(_win32_vs);


def _get_vs_vc_path_win10():
    return get_msvc_path(_win10_vs);


def get_msbuild_path_win32():
    return get_msbuild_path(_win32_vs)


def get_msbuild_path_win10():
    return get_msbuild_path(_win10_vs)


def get_cmake_visual_studio_generator(vs_ver, arch):
    generator=''
    if vs_ver == '2013':
        generator='Visual Studio 12'
    elif vs_ver == '2015':
        generator='Visual Studio 14 2015'
    elif vs_ver == '2017':
        generator='Visual Studio 15 2017'
    else:
        raise RuntimeError('Unknown Visual Studio version: {}'.format(vs_ver))

    if arch is None or arch == 'x86':
        pass
    elif arch == 'x86_64':
        generator='{} Win64'.format(generator)
    elif arch == 'arm':
        generator='{} ARM'.format(generator)
    else:
        raise RuntimeError('Unknown Visual Studio arch: {}'.format(arch))
    return generator


def get_cmake_generator_win32_x86():
    return get_cmake_visual_studio_generator(_win32_vs, 'x86')


def get_cmake_generator_win32_x64():
    return get_cmake_visual_studio_generator(_win32_vs, 'x86_64')


def get_cmake_generator_win10_x86():
    return get_cmake_visual_studio_generator(_win32_vs, 'x86')


def get_cmake_generator_win10_x64():
    return get_cmake_visual_studio_generator(_win32_vs, 'x86_64')


def get_cmake_generator_win10_arm():
    return get_cmake_visual_studio_generator(_win32_vs, 'arm')


def get_cmake_generator_macos():
    return 'Xcode'


def get_macos_deployment_target():
    return _macos_deployment_target


def get_cmake_generator_linux():
    return 'Unix Makefiles'
