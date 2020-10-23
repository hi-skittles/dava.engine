import os
import shutil
import build_utils


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32', 'win10']
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
    elif target == 'win10':
        _build_win10(working_directory_path, root_project_path)
    elif target == 'macos':
        _build_macos(working_directory_path, root_project_path)
    elif target == 'ios':
        _build_ios(working_directory_path, root_project_path)
    elif target == 'android':
        _build_android(working_directory_path, root_project_path)
    elif target == 'linux':
        _build_linux(working_directory_path, root_project_path)


def get_download_info():
    return 'https://github.com/adah1972/libunibreak/archive/8c92b46511baf5b51457f202cf53d8602e1aef17.zip'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(
        working_directory_path, 'libunibreak_source')
    url = get_download_info()
    build_utils.download_and_extract(
        url, working_directory_path,
        source_folder_path,
        'libunibreak-8c92b46511baf5b51457f202cf53d8602e1aef17')
    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
    shutil.copyfile(
        'CMakeLists.txt', os.path.join(source_folder_path, 'CMakeLists.txt'))
    build_utils.apply_patch(
        os.path.abspath('patch.diff'),
        working_directory_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'unibreak.sln', 'unibreak',
        'unibreak.lib', 'unibreak.lib',
        'unibreak.lib', 'unibreak.lib',
        'unibreak.lib', 'unibreak.lib',
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_win10_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'unibreak.sln', 'unibreak',
        'unibreak.lib', 'unibreak.lib',
        'unibreak.lib', 'unibreak.lib',
        'unibreak.lib', 'unibreak.lib',
        'unibreak.lib', 'unibreak.lib')

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    install_dir_macos = os.path.join(
        working_directory_path, 'gen/install_macos')
    build_utils.run_process(['autoreconf', '-i'], process_cwd=source_folder_path)
    env=build_utils.get_autotools_macos_env()
    env['CFLAGS'] += ' -DNDEBUG'
    env['CXXFLAGS'] += ' -DNDEBUG'
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=x86_64-apple-darwin', '--disable-shared', '--enable-static'],
        install_dir_macos,
        env=env)

    libunibreak_lib_path = os.path.join(install_dir_macos, 'lib/libunibreak.a')
    shutil.copyfile(
        libunibreak_lib_path,
        os.path.join(
            root_project_path,
            'Libs/lib_CMake/mac/libunibreak_macos.a'))

    _copy_headers_from_install(install_dir_macos, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    install_dir_ios = os.path.join(working_directory_path, 'gen/install_ios')
    build_utils.run_process(['autoreconf', '-i'], process_cwd=source_folder_path)
    env=build_utils.get_autotools_ios_env()
    env['CFLAGS'] += ' -DNDEBUG'
    env['CXXFLAGS'] += ' -DNDEBUG'
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=armv7-apple-darwin', '--disable-shared', '--enable-static'],
        install_dir_ios,
        env=env)

    libunibreak_lib_path = os.path.join(install_dir_ios, 'lib/libunibreak.a')
    shutil.copyfile(
        libunibreak_lib_path,
        os.path.join(
            root_project_path,
            'Libs/lib_CMake/ios/libunibreak_ios.a'))

    _copy_headers_from_install(install_dir_ios, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.run_process(['autoreconf', '-i'], process_cwd=source_folder_path)

    # ARM
    toolchain_path_arm = build_utils.android_ndk_get_toolchain_arm()

    env = build_utils.get_autotools_android_arm_env(toolchain_path_arm)
    env['CFLAGS'] += ' -DNDEBUG'
    env['CPPFLAGS'] += ' -DNDEBUG'
    install_dir_android_arm = os.path.join(working_directory_path, 'gen/install_android_arm')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=arm-linux-androideabi',
         '--disable-shared',
         '--enable-static'],
        install_dir_android_arm,
        env=env)

    # x86
    toolchain_path_x86 = build_utils.android_ndk_get_toolchain_x86()

    env = build_utils.get_autotools_android_x86_env(toolchain_path_x86)
    env['CFLAGS'] += ' -DNDEBUG'
    env['CPPFLAGS'] += ' -DNDEBUG'
    install_dir_android_x86 = os.path.join(working_directory_path, 'gen/install_android_x86')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=i686-linux-android',
         '--disable-shared',
         '--enable-static'],
        install_dir_android_x86,
        env=env)

    libs_android_root = os.path.join(
        root_project_path, 'Libs/lib_CMake/android')

    libunibreak_lib_path_arm = os.path.join(
        install_dir_android_arm, 'lib/libunibreak.a')
    shutil.copyfile(
        libunibreak_lib_path_arm,
        os.path.join(libs_android_root, 'armeabi-v7a/libunibreak.a'))

    libunibreak_lib_path_x86 = os.path.join(
        install_dir_android_x86, 'lib/libunibreak.a')
    shutil.copyfile(
        libunibreak_lib_path_x86,
        os.path.join(libs_android_root, 'x86/libunibreak.a'))

    _copy_headers_from_install(install_dir_android_arm, root_project_path)

def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    env = build_utils.get_autotools_linux_env()

    build_utils.run_process(['autoreconf', '-i'], process_cwd=source_folder_path)

    install_dir = os.path.join(working_directory_path, 'gen/install_linux')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--disable-shared', '--enable-static'],
        install_dir,
        env=env)

    lib_path = os.path.join(install_dir, 'lib/libunibreak.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/linux/libunibreak.a'))

    _copy_headers_from_install(install_dir, root_project_path)

def _copy_headers_from_install(install_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/unibreak')
    build_utils.copy_folder_recursive(
        os.path.join(install_folder_path, 'include'), include_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/unibreak')
    build_utils.copy_files(
        os.path.join(source_folder_path, 'src'), include_path, '*.h')
