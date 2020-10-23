import os
import build_utils
import build_config
import shutil


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32']
    elif platform == 'darwin':
        return ['macos']
    else:
        return []


def get_dependencies_for_target(target):
    return []


def build_for_target(target, working_directory_path, root_project_path):
    if target == 'win32':
        _build_win32(working_directory_path, root_project_path)
    elif target == 'macos':
        _build_macos(working_directory_path, root_project_path)


def get_download_info():
    return 'Libs/bullet'


def _build_win32(working_directory_path, root_project_path):
    source_folder_path=os.path.join(root_project_path, get_download_info())

    build_x86_folder, build_x64_folder = (
        build_utils.build_and_copy_libraries_win32_cmake(
            os.path.join(working_directory_path, 'gen'),
            source_folder_path,
            root_project_path,
            'bullet.sln', 'bullet',
            'bullet.lib', 'bullet.lib',
            'bullet.lib', 'bullet.lib',
            'bullet.lib', 'bullet.lib',
            static_runtime=False))


def _build_macos(working_directory_path, root_project_path):
    source_folder_path=os.path.join(root_project_path, get_download_info())

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'bullet.xcodeproj', 'bullet',
        'libbullet.a',
        'libbullet.a')

