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
    return {'win': 'http://www.ijg.org/files/jpegsr9a.zip',
            'others': 'http://www.ijg.org/files/jpegsrc.v9a.tar.gz'}


def _get_downloaded_archive_inner_dir():
    # Because archive inner folder and archive file name do not match
    # If you change download link - change this one too
    return 'jpeg-9a'


def _download_and_extract(
        working_directory_path,
        download_url_key,
        source_folder_postfix=''):
    source_folder_name = 'libjpeg_source' + source_folder_postfix
    source_folder_path = os.path.join(
        working_directory_path, source_folder_name)

    url = get_download_info()[download_url_key]
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        _get_downloaded_archive_inner_dir())

    return source_folder_path


def _patch_sources(
        source_folder_path,
        working_directory_path,
        patch_file_path):
    try:
        if source_folder_path in _patch_sources.cache:
            return
    except AttributeError:
        _patch_sources.cache = []
        pass

    shutil.copyfile('WIN32.MAK', os.path.join(source_folder_path, 'WIN32.MAK'))
    build_utils.run_process(
        ['nmake', '/f', 'makefile.vc', 'setup-v10'],
        process_cwd=source_folder_path,
        shell=True,
        environment=build_utils.get_win32_vs_x86_env())
    build_utils.apply_patch(
        os.path.abspath(patch_file_path), working_directory_path)

    _patch_sources.cache.append(source_folder_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(
        working_directory_path, 'win', source_folder_postfix='_win32')

    shutil.copyfile(
        'CMakeLists.txt',
        os.path.join(source_folder_path, 'CMakeLists.txt'))
    os.rename(os.path.join(source_folder_path, 'jconfig.vc'),
              os.path.join(source_folder_path, 'jconfig.h'))

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'jpeg.sln', 'jpeg',
        'jpeg.lib', 'jpeg.lib',
        'libjpeg.lib', 'libjpeg.lib',
        'libjpeg.lib', 'libjpeg.lib',
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(
        working_directory_path, 'win', '_win10')
    _patch_sources(
        source_folder_path, working_directory_path, 'patch_win10.diff')

    sln_path = os.path.join(source_folder_path, 'jpeg.sln')
    build_utils.build_vs(sln_path, 'Debug', 'Win32')
    build_utils.build_vs(sln_path, 'Release', 'Win32')
    build_utils.build_vs(sln_path, 'Debug', 'x64')
    build_utils.build_vs(sln_path, 'Release', 'x64')
    build_utils.build_vs(sln_path, 'Debug', 'ARM')
    build_utils.build_vs(sln_path, 'Release', 'ARM')

    libs_win10_root = os.path.join(root_project_path, 'Libs/lib_CMake/win10')

    lib_path_x86_debug = os.path.join(source_folder_path, 'Debug/jpeg.lib')
    lib_path_x86_release = os.path.join(source_folder_path, 'Release/jpeg.lib')
    shutil.copyfile(
        lib_path_x86_debug,
        os.path.join(libs_win10_root, 'Win32/Debug/jpeg_win10.lib'))
    shutil.copyfile(
        lib_path_x86_release,
        os.path.join(libs_win10_root, 'Win32/Release/jpeg_win10.lib'))

    lib_path_x64_debug = os.path.join(
        source_folder_path, 'x64/Debug/jpeg.lib')
    lib_path_x64_release = os.path.join(
        source_folder_path, 'x64/Release/jpeg.lib')
    shutil.copyfile(
        lib_path_x64_debug,
        os.path.join(libs_win10_root, 'x64/Debug/jpeg_win10.lib'))
    shutil.copyfile(
        lib_path_x64_release,
        os.path.join(libs_win10_root, 'x64/Release/jpeg_win10.lib'))

    lib_path_arm_debug = os.path.join(
        source_folder_path, 'ARM/Debug/jpeg.lib')
    lib_path_arm_release = os.path.join(
        source_folder_path, 'ARM/Release/jpeg.lib')
    shutil.copyfile(
        lib_path_arm_debug,
        os.path.join(libs_win10_root, 'arm/Debug/jpeg_win10.lib'))
    shutil.copyfile(
        lib_path_arm_release,
        os.path.join(libs_win10_root, 'arm/Release/jpeg_win10.lib'))

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(
        working_directory_path, 'others')

    install_dir_macos = os.path.join(
        working_directory_path, 'gen/install_macos')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=x86_64-apple-darwin', '--disable-shared', '--enable-static'],
        install_dir_macos,
        env=build_utils.get_autotools_macos_env())

    lib_path = os.path.join(install_dir_macos, 'lib/libjpeg.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/mac/libjpeg_macos.a'))

    _copy_headers_from_install(install_dir_macos, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(
        working_directory_path, 'others')

    install_dir_ios = os.path.join(working_directory_path, 'gen/install_ios')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=armv7-apple-darwin', '--disable-shared', '--enable-static'],
        install_dir_ios,
        env=build_utils.get_autotools_ios_env())

    lib_path = os.path.join(install_dir_ios, 'lib/libjpeg.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/ios/libjpeg_ios.a'))

    _copy_headers_from_install(install_dir_ios, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(
        working_directory_path, 'others')

    # ARM
    toolchain_path_arm = build_utils.android_ndk_get_toolchain_arm()

    install_dir_android_arm = os.path.join(
        working_directory_path, 'gen/install_android_arm')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=arm-linux-androideabi',
         '--disable-shared',
         '--enable-static'],
        install_dir_android_arm,
       env=build_utils.get_autotools_android_arm_env(toolchain_path_arm))

    # x86
    toolchain_path_x86 = build_utils.android_ndk_get_toolchain_x86()

    install_dir_android_x86 = os.path.join(
        working_directory_path, 'gen/install_android_x86')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=i686-linux-android',
         '--disable-shared',
         '--enable-static'],
        install_dir_android_x86,
        env=build_utils.get_autotools_android_x86_env(toolchain_path_x86))

    libs_android_root = os.path.join(root_project_path, 'Libs/lib_CMake/android')

    lib_path_arm = os.path.join(install_dir_android_arm, 'lib/libjpeg.a')
    shutil.copyfile(
        lib_path_arm, os.path.join(libs_android_root, 'armeabi-v7a/libjpeg.a'))

    lib_path_x86 = os.path.join(install_dir_android_x86, 'lib/libjpeg.a')
    shutil.copyfile(
        lib_path_x86, os.path.join(libs_android_root, 'x86/libjpeg.a'))

    _copy_headers_from_install(install_dir_android_arm, root_project_path)


def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path, 'others')

    env = build_utils.get_autotools_linux_env()
    install_dir = os.path.join(working_directory_path, 'gen/install_linux')

    build_utils.build_with_autotools(
        source_folder_path,
        ['--disable-shared', '--enable-static'],
        install_dir,
        env=env)

    shutil.copyfile(os.path.join(install_dir, 'lib/libjpeg.a'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/linux/libjpeg.a'))

    _copy_headers_from_install(install_dir, root_project_path)


def _copy_headers_from_install(install_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libjpeg')
    build_utils.copy_folder_recursive(
        os.path.join(install_folder_path, 'include'), include_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libjpeg')
    build_utils.copy_files_by_name(source_folder_path,
                                   include_path,
                                   ['jpeglib.h',
                                    'jmorecfg.h',
                                    'jerror.h',
                                    'jconfig.h'
                                   ])
