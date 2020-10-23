import os
import shutil
import build_utils
import build_config


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32']
    elif platform == 'darwin':
        return ['macos', 'ios', 'android']
    elif platform == 'linux':
        return ['android', 'linux']
    else:
        return []


def get_dependencies_for_target(target):
    return []


def build_for_target(target, working_directory_path, root_project_path):
    if target == 'win32':
        _build_win32(working_directory_path, root_project_path)
    elif target == 'macos':
        _build_macos(working_directory_path, root_project_path)
    elif target == 'ios':
        _build_ios(working_directory_path, root_project_path)
    elif target == 'android':
        _build_android(working_directory_path, root_project_path)
    elif target == 'linux':
        _build_linux(working_directory_path, root_project_path)


def get_download_info():
    return 'https://github.com/kkdaemon/libuv.git'


def _download(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'libuv')

    build_utils.run_process(
        ['git', 'clone', '-b', 'winuap_support', get_download_info()],
        process_cwd=working_directory_path,
        shell=True)

    return source_folder_path


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download(working_directory_path)

    build_folder_path = os.path.join(working_directory_path, 'gen/build_win32')
    build_folder_path_x86 = os.path.join(build_folder_path, 'x86')
    build_folder_path_x86_debug = os.path.join(build_folder_path_x86, 'Debug')
    build_folder_path_x86_release = os.path.join(build_folder_path_x86, 'Release')
    build_folder_path_x64 = os.path.join(build_folder_path, 'x64')
    build_folder_path_x64_debug = os.path.join(build_folder_path_x64, 'Debug')
    build_folder_path_x64_release = os.path.join(build_folder_path_x64, 'Release')

    os.makedirs(build_folder_path_x86_debug)
    os.makedirs(build_folder_path_x86_release)
    os.makedirs(build_folder_path_x64_debug)
    os.makedirs(build_folder_path_x64_release)

    vc_solution_file=os.path.join(source_folder_path, 'uv.sln')
    override_props_file=os.path.abspath('override_win32.props')
    toolset=build_config.get_msvc_toolset_ver_win32()
    msbuild_args=[
        "/p:ForceImportBeforeCppTargets={}".format(override_props_file),
        "/p:WindowsTargetPlatformVersion={}".format(build_config.get_msvc_sdk_version_win32())
    ]

    # x86
    x86_env = build_utils.get_win32_vs_x86_env()
    x86_env['GYP_MSVS_VERSION'] = build_config.get_gyp_msvs_version()
    build_utils.run_process(
        ['vcbuild.bat', 'x86', 'nobuild'],
        process_cwd=source_folder_path,
        environment=x86_env,
        shell=True)

    build_utils.build_vs(vc_solution_file, 'Debug', 'Win32', 'libuv', toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc_solution_file, 'Release', 'Win32', 'libuv', toolset, msbuild_args=msbuild_args)

    lib_path_x86_debug = os.path.join(build_folder_path_x86_debug, 'libuv.lib')
    lib_path_x86_release = os.path.join(build_folder_path_x86_release, 'libuv.lib')
    shutil.copyfile(
        os.path.join(source_folder_path, 'Debug/lib/libuv.lib'),
        lib_path_x86_debug)
    shutil.copyfile(
        os.path.join(source_folder_path, 'Release/lib/libuv.lib'),
        lib_path_x86_release)

    build_utils.run_process(
        ['vcbuild.bat', 'clean'],
        process_cwd=source_folder_path,
        environment=x86_env,
        shell=True)

    # x64
    x64_env = build_utils.get_win32_vs_x64_env()
    x64_env['GYP_MSVS_VERSION'] = build_config.get_gyp_msvs_version()
    build_utils.run_process(
        ['vcbuild.bat', 'x64', 'nobuild'],
        process_cwd=source_folder_path,
        environment=x64_env,
        shell=True)

    build_utils.build_vs(vc_solution_file, 'Debug', 'x64', 'libuv', toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc_solution_file, 'Release', 'x64', 'libuv', toolset, msbuild_args=msbuild_args)

    lib_path_x64_debug = os.path.join(build_folder_path_x64_debug, 'libuv.lib')
    lib_path_x64_release = os.path.join(build_folder_path_x64_release, 'libuv.lib')
    shutil.copyfile(
        os.path.join(source_folder_path, 'Debug/lib/libuv.lib'),
        lib_path_x64_debug)
    shutil.copyfile(
        os.path.join(source_folder_path, 'Release/lib/libuv.lib'),
        lib_path_x64_release)

    # copy libs
    libs_win_root = os.path.join(root_project_path, 'Libs/lib_CMake/win')
    shutil.copyfile(
        lib_path_x86_debug,
        os.path.join(libs_win_root, 'x86/Debug/libuv.lib'))
    shutil.copyfile(
        lib_path_x86_release,
        os.path.join(libs_win_root, 'x86/Release/libuv.lib'))
    shutil.copyfile(
        lib_path_x64_debug,
        os.path.join(libs_win_root, 'x64/Debug/libuv.lib'))
    shutil.copyfile(
        lib_path_x64_release,
        os.path.join(libs_win_root, 'x64/Release/libuv.lib'))

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    env = build_utils.get_autotools_macos_env()

    install_dir_macos = os.path.join(
        working_directory_path, 'gen/install_macos')
    build_utils.run_process(
        ['sh', 'autogen.sh'], process_cwd=source_folder_path, environment=env)
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=x86_64-apple-darwin', '--disable-shared', '--enable-static'],
        install_dir_macos,
        env=env)

    lib_path = os.path.join(install_dir_macos, 'lib/libuv.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/mac/libuv_macos.a'))

    _copy_headers_from_install(install_dir_macos, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    env = build_utils.get_autotools_ios_env()

    install_dir_ios = os.path.join(working_directory_path, 'gen/install_ios')
    build_utils.run_process(
        ['sh', 'autogen.sh'],
        process_cwd=source_folder_path,
        environment=env)
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=armv7-apple-darwin', '--disable-shared', '--enable-static'],
        install_dir_ios,
        env=env)

    lib_path = os.path.join(install_dir_ios, 'lib/libuv.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/ios/libuv_ios.a'))

    _copy_headers_from_install(install_dir_ios, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    additional_defines = ' -D__ANDROID__ -DHAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC=1'

    # ARM
    toolchain_path_arm = build_utils.android_ndk_get_toolchain_arm()

    env_arm = build_utils.get_autotools_android_arm_env(toolchain_path_arm)
    env_arm['CFLAGS'] = env_arm['CFLAGS'] + additional_defines
    env_arm['CPPFLAGS'] = env_arm['CPPFLAGS'] + additional_defines

    install_dir_android_arm = os.path.join(
        working_directory_path, 'gen/install_android_arm')
    build_utils.run_process(
        ['sh', 'autogen.sh'],
        process_cwd=source_folder_path,
        environment=env_arm)
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=arm-linux-androideabi',
         '--disable-shared',
         '--enable-static'],
        install_dir_android_arm, env=env_arm)

    # x86    
    toolchain_path_x86 = build_utils.android_ndk_get_toolchain_x86()

    env_x86 = build_utils.get_autotools_android_x86_env(toolchain_path_x86)
    env_x86['CFLAGS'] = env_x86['CFLAGS'] + additional_defines
    env_x86['CPPFLAGS'] = env_x86['CPPFLAGS'] + additional_defines

    install_dir_android_x86 = os.path.join(
        working_directory_path, 'gen/install_android_x86')
    build_utils.run_process(
        ['sh', 'autogen.sh'],
        process_cwd=source_folder_path,
        environment=env_x86)
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=i686-linux-android', '--disable-shared', '--enable-static'],
        install_dir_android_x86,
        env=env_x86)

    libs_android_root = os.path.join(root_project_path, 'Libs/lib_CMake/android')

    lib_path_arm = os.path.join(install_dir_android_arm, 'lib/libuv.a')
    shutil.copyfile(
        lib_path_arm, os.path.join(libs_android_root, 'armeabi-v7a/libuv.a'))

    lib_path_x86 = os.path.join(install_dir_android_x86, 'lib/libuv.a')
    shutil.copyfile(
        lib_path_x86, os.path.join(libs_android_root, 'x86/libuv.a'))

    _copy_headers_from_install(install_dir_android_arm, root_project_path)

def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    # Clone gyp
    build_utils.run_process(
        ['git clone https://chromium.googlesource.com/external/gyp.git build/gyp'],
        process_cwd=source_folder_path,
        shell=True)

    # Generate makefile using gyp
    env = build_utils.get_autotools_linux_env()
    build_utils.run_process(
        ['./gyp_uv.py -f make'],
        process_cwd=source_folder_path,
        environment=env,
        shell=True)
    # Build release library: only libuv.a, skipping tests
    build_utils.run_process(
        ['BUILDTYPE=Release make libuv -C out'],
        process_cwd=source_folder_path,
        environment=env,
        shell=True)

    # Copy binary files to dava.engine's library folder
    source_dir = os.path.join(source_folder_path, 'out/Release')
    target_dir = os.path.join(root_project_path, 'Libs/lib_CMake/linux')
    shutil.copyfile(os.path.join(source_dir, 'libuv.a'),
                    os.path.join(target_dir, 'libuv.a'))

    # Copy headers to dava.engine's include folder
    _copy_headers_from_install(source_folder_path, root_project_path)

def _copy_headers_from_install(install_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libuv')
    build_utils.copy_folder_recursive(
        os.path.join(install_folder_path, 'include'), include_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libuv')
    build_utils.copy_files(
        os.path.join(source_folder_path, 'include'), include_path, '*.h')
