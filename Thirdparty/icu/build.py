import os
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
    return 'Libs\icucommon'
    """ Use icucommon from dava repository
    return {'windows': 'http://download.icu-project.org/files/icu4c/57.1/icu4c-57_1-src.zip',
            'others': 'http://download.icu-project.org/files/icu4c/57.1/icu4c-57_1-src.tgz'}
    """


def _download_and_extract(
        working_directory_path,
        download_url_key,
        source_folder_postfix=''):
    source_folder_name = 'icu_source' + source_folder_postfix
    source_folder_path = os.path.join(
        working_directory_path, source_folder_name)

    url = get_download_info()[download_url_key]
    build_utils.download_and_extract(
        url, working_directory_path, source_folder_path, 'icu')

    return source_folder_path


def _patch_sources(source_folder_path, working_directory_path, patch_postifx):
    try:
        if source_folder_path in _patch_sources.cache:
            return
    except AttributeError:
        _patch_sources.cache = []
        pass

    # Apply fixes
    build_utils.apply_patch(
        os.path.abspath('patch' + patch_postifx + '.diff'),
        working_directory_path)

    _patch_sources.cache.append(source_folder_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path=os.path.join(root_project_path, get_download_info())

    build_x86_folder, build_x64_folder = (
        build_utils.build_and_copy_libraries_win32_cmake(
            os.path.join(working_directory_path, 'gen'),
            source_folder_path,
            root_project_path,
            'icucommon.sln', 'icucommon',
            'icucommon.lib', 'icucommon.lib',
            'icucommon.lib', 'icucommon.lib',
            'icucommon.lib', 'icucommon.lib',
            static_runtime=False))

    """
    prefix = '_win32'
    source_folder_path = _download_and_extract(
        working_directory_path, 'windows', prefix)
    _patch_sources(
        source_folder_path, working_directory_path, prefix)

    vc_solution_file_path = os.path.join(
        source_folder_path, 'source/allinone/allinone.sln')
    build_utils.build_vs(vc_solution_file_path, 'Debug', 'Win32', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Release', 'Win32', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Debug', 'x64', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Release', 'x64', 'common')
    """


def _build_win10(working_directory_path, root_project_path):
    prefix = '_win10'
    source_folder_path = _download_and_extract(
        working_directory_path, 'windows', prefix)
    _patch_sources(source_folder_path, working_directory_path, prefix)

    vc_solution_file_path = os.path.join(
        source_folder_path, 'source/allinone/allinone.sln')
    build_utils.build_vs(vc_solution_file_path, 'Debug', 'Win32', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Release', 'Win32', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Debug', 'x64', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Release', 'x64', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Debug', 'ARM', 'common')
    build_utils.build_vs(vc_solution_file_path, 'Release', 'ARM', 'common')


# Guard execution to avoid multiple macos builds,
# since macos binaries should be built to cross-compile for android & ios
@build_utils.run_once
def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(
        working_directory_path, 'others', '_macos')

    install_dir_macos = os.path.join(
        working_directory_path, 'gen/install_macos')
    build_utils.build_with_autotools(
        os.path.join(source_folder_path, 'source'),
        ['--host=x86_64-apple-darwin', '--disable-shared', '--enable-static'],
        install_dir_macos,
        env=build_utils.get_autotools_macos_env(),
        postclean=False)


def _build_ios(working_directory_path, root_project_path):
    _build_macos(working_directory_path, root_project_path)

    source_folder_path = _download_and_extract(
        working_directory_path, 'others')

    install_dir_ios = os.path.join(working_directory_path, 'gen/install_ios')
    build_utils.build_with_autotools(
        os.path.join(source_folder_path, 'source'),
        ['--with-cross-build=' + os.path.abspath(
            source_folder_path + '_macos/source'),
         '--host=armv7-apple-darwin',
         '--disable-shared',
         '--enable-static'],
        install_dir_ios,
        env=build_utils.get_autotools_ios_env())


def _build_android(working_directory_path, root_project_path):
    _build_macos(working_directory_path, root_project_path)

    source_folder_path = _download_and_extract(
        working_directory_path, 'others')

    # ARM

    toolchain_path_arm = build_utils.android_ndk_get_toolchain_arm()

    install_dir_android_arm = os.path.join(
        working_directory_path, 'gen/install_android_arm')
    build_utils.build_with_autotools(
        os.path.join(source_folder_path, 'source'),
        ['--with-cross-build=' + os.path.abspath(
            source_folder_path + '_macos/source'),
         '--host=arm-linux-androideabi',
         '--disable-shared',
         '--enable-static'],
        install_dir_android_arm,
        env=build_utils.get_autotools_android_arm_env(
            toolchain_path_arm, enable_stl=True))

    # x86

    toolchain_path_x86 = build_utils.android_ndk_get_toolchain_x86()

    install_dir_android_x86 = os.path.join(
        working_directory_path, 'gen/install_android_x86')
    build_utils.build_with_autotools(
        os.path.join(source_folder_path, 'source'),
        ['--with-cross-build=' + os.path.abspath(
            source_folder_path + '_macos/source'),
         '--host=i686-linux-android',
         '--disable-shared',
         '--enable-static'],
        install_dir_android_x86,
        env=build_utils.get_autotools_android_x86_env(
            toolchain_path_x86, enable_stl=True))

# TODO: Add copying headers & libraries when switching to new directories structure

def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path, 'others', '_linux')

    env = build_utils.get_autotools_linux_env()
    install_dir = os.path.join(working_directory_path, 'gen/install_linux')

    build_utils.build_with_autotools(
        os.path.join(source_folder_path, 'source'),
        ['--disable-shared', '--enable-static'],
        install_dir,
        env=env,
        postclean=False)

