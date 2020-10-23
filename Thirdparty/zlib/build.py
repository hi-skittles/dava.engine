import os
import shutil
import build_utils


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32', 'win10']
    else:
        return []


def get_dependencies_for_target(target):
    return []


def build_for_target(target, working_directory_path, root_project_path):
    if target == 'win32':
        _build_win32(working_directory_path, root_project_path)
    elif target == 'win10':
        _build_win10(working_directory_path, root_project_path)


def get_download_info():
    return 'http://zlib.net/fossils/zlib-1.2.5.tar.gz'


def _get_downloaded_archive_inner_dir():
    # Because zlib archive inner folder and archive file name do not match
    # If you change download link - change this one too
    return 'zlib-1.2.5'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'zlib_source')
    build_utils.download_and_extract(
        get_download_info(),
        working_directory_path,
        source_folder_path,
        _get_downloaded_archive_inner_dir())
    return source_folder_path


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    # zlib's CMakeLists.txt states:
    #   You must remove zconf.h from the source tree.  This file is included with zlib
    #   but CMake generates this file for you automatically in the build directory.
    os.remove(os.path.join(source_folder_path, 'zconf.h'))

    build_x86_folder, build_x64_folder = (
        build_utils.build_and_copy_libraries_win32_cmake(
            os.path.join(working_directory_path, 'gen'),
            source_folder_path,
            root_project_path,
            'zlib.sln', 'zlib',
            'zlibd.lib', 'zlib.lib',
            'zlib.lib', 'zlib.lib',
            'zlib.lib', 'zlib.lib',
            cmake_additional_args=['-DBUILD_SHARED_LIBS=0'],
            static_runtime=False))

    _copy_headers(source_folder_path, build_x86_folder, root_project_path)

    # Copy created configuration header to root folder
    # Required to use source folder as include path
    # TODO: get rid of this and copy to Libs/Include directly
    shutil.copyfile(
        os.path.join(build_x86_folder, 'zconf.h'),
        os.path.join(source_folder_path, 'zconf.h'))


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    build_x86_folder, build_x64_folder, build_arm_folder = (
        build_utils.build_and_copy_libraries_win10_cmake(
            os.path.join(working_directory_path, 'gen'),
            source_folder_path,
            root_project_path,
            'zlib.sln', 'zlibstatic',
            'zlibstaticd.lib', 'zlibstatic.lib',
            'zlib.lib', 'zlib.lib',
            'zlib.lib', 'zlib.lib',
            'zlib.lib', 'zlib.lib'))

    _copy_headers(source_folder_path, build_x86_folder, root_project_path)

    # Copy created configuration header to root folder
    # Required to use source folder as include path
    # TODO: get rid of this and copy to Libs/Include directly
    shutil.copyfile(
        os.path.join(build_x86_folder, 'zconf.h'),
        os.path.join(source_folder_path, 'zconf.h'))


def _copy_headers(source_folder_path, build_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libpng')
    build_utils.copy_files(source_folder_path, include_path, 'zlib.h')
    build_utils.copy_files(build_folder_path, include_path, 'zconf.h')
