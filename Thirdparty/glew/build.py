import os
import build_utils


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32']
    else:
        return []


def get_dependencies_for_target(target):
    return []


def build_for_target(target, working_directory_path, root_project_path):
    if target == 'win32':
        _build_win32(working_directory_path, root_project_path)


def get_download_info():
    return 'https://sourceforge.net/projects/glew/files/glew/2.0.0/glew-2.0.0.zip'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'glew_source')

    url = get_download_info()
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        build_utils.get_url_file_name_no_ext(url))

    return source_folder_path


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, 'gen'),
        os.path.join(source_folder_path, 'build/cmake'),
        root_project_path,
        'glew.sln', 'glew_s',
        'libglew32d.lib', 'libglew32.lib',
        'glew32.lib', 'glew32.lib',
        'glew32.lib', 'glew32.lib',
        target_lib_subdir='lib')


# TODO: add copying headers after switching to new folders structure