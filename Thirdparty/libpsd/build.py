import os
import shutil
import build_utils


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32']
    elif platform == 'darwin':
        return ['macos']
    elif platform == 'linux':
        return ['linux']
    else:
        return []


def get_dependencies_for_target(target):
    if target == 'win32':
        return ['zlib']
    else:
        return []


def build_for_target(target, working_directory_path, root_project_path):
    if target == 'win32':
        _build_win32(working_directory_path, root_project_path)
    elif target == 'macos':
        _build_macos(working_directory_path, root_project_path)
    elif target == 'linux':
        _build_linux(working_directory_path, root_project_path)


def get_download_info():
    return 'https://sourceforge.net/projects/libpsd/files/libpsd/0.9/libpsd-0.9.zip'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'libpsd_source')

    url = get_download_info()
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        build_utils.get_url_file_name_no_ext(url))

    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
    build_utils.apply_patch(
        os.path.abspath('patch_v0.9.diff'), working_directory_path)
    shutil.copyfile(
        'CMakeLists.txt', os.path.join(source_folder_path, 'CMakeLists.txt'))


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    cmake_flags = ['-DZLIB_INCLUDE_DIR=' + os.path.join(working_directory_path, '../zlib/zlib_source/')]

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'psd.sln', 'psd',
        'psd.lib', 'psd.lib',
        'libpsd.lib', 'libpsd.lib',
        'libpsd.lib', 'libpsd.lib',
        cmake_flags,
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'psd.xcodeproj', 'psd',
        'libpsd.a',
        'libpsd.a')

    _copy_headers(source_folder_path, root_project_path)

def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_linux_cmake(
        gen_folder_path=os.path.join(working_directory_path, 'gen'),
        source_folder_path=source_folder_path,
        root_project_path=root_project_path,
        target="all",
        lib_name='libpsd.a')

    _copy_headers(source_folder_path, root_project_path)

def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libpsd')
    build_utils.copy_files_by_name(
        os.path.join(source_folder_path, 'include'),
        include_path,
        ['libpsd.h', 'psd_color.h', 'psd_types.h'])
