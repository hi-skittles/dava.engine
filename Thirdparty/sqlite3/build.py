import os
import shutil
import build_utils


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32', 'win10', 'android']
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
    return 'https://www.sqlite.org/2016/sqlite-amalgamation-3130000.zip'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'sqlite3_source')
    url = get_download_info()
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        build_utils.get_url_file_name_no_ext(url))
    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
    shutil.copyfile(
        'CMakeLists.txt',
        os.path.join(source_folder_path, 'CMakeLists.txt'))


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'sqlite3.sln', 'sqlite3',
        'sqlite3.lib', 'sqlite3.lib',
        'sqlite3.lib', 'sqlite3.lib',
        'sqlite3.lib', 'sqlite3.lib',
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_win10_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'sqlite3.sln', 'sqlite3',
        'sqlite3.lib', 'sqlite3.lib',
        'sqlite3_uap.lib', 'sqlite3_uap.lib',
        'sqlite3_uap.lib', 'sqlite3_uap.lib',
        'sqlite3_uap.lib', 'sqlite3_uap.lib',
        ['-DWIN_UWP=1'])

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'sqlite3.xcodeproj', 'sqlite3',
        'libsqlite3.a',
        'libsqlite3.a')

    _copy_headers(source_folder_path, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_ios_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'sqlite3.xcodeproj', 'sqlite3',
        'libsqlite3.a',
        'libsqlite3_ios.a')

    _copy_headers(source_folder_path, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_android_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libsqlite3.a',
        'libsqlite3.a')

    _copy_headers(source_folder_path, root_project_path)

def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_linux_cmake(
        gen_folder_path=os.path.join(working_directory_path, 'gen'),
        source_folder_path=source_folder_path,
        root_project_path=root_project_path,
        target="all",
        lib_name='libsqlite3.a')

    _copy_headers(source_folder_path, root_project_path)

def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include')
    build_utils.copy_files(source_folder_path, include_path, '*.h')
