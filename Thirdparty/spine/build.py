import os
import shutil
import build_utils


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32', 'win10', 'android']
    else:
        return ['macos', 'ios', 'android']


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


def get_download_url():
    return 'https://github.com/EsotericSoftware/spine-runtimes/archive/3.4.02.zip'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'spine_source')
    url = get_download_url()
    build_utils.download_and_extract(
            url,
            working_directory_path,
            source_folder_path,
            'spine-runtimes-3.4.02')
    return os.path.join(source_folder_path, 'spine-c')


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
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
        'Project.sln', 'spine-c',
        'spine-c.lib', 'spine-c.lib',
        'spine.lib', 'spine.lib',
        'spine.lib', 'spine.lib',
        output_libs_path='Modules/Spine/Libs',
        output_lib_folder='Win32',
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_win10_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'Project.sln', 'spine-c',
        'spine-c.lib', 'spine-c.lib',
        'spined.lib', 'spine.lib',
        'spined.lib', 'spine.lib',
        'spined.lib', 'spine.lib',
        output_libs_path='Modules/Spine/Libs/lib')

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'Project.xcodeproj', 'spine-c',
        'libspine-c.a',
        'spine.a',
        output_libs_path='Modules/Spine/Libs/lib')

    _copy_headers(source_folder_path, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_ios_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'Project.xcodeproj', 'spine-c',
        'libspine-c.a',
        'spine.a',
        output_libs_path='Modules/Spine/Libs/lib')

    _copy_headers(source_folder_path, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_android_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libspine-c.a',
        'spine.a',
        output_libs_path='Modules/Spine/Libs/lib')

    _copy_headers(source_folder_path, root_project_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Modules', 'Spine', 'Libs', 'include', 'spine')
    build_utils.copy_files(os.path.join(source_folder_path, 'include', 'spine'), include_path, '*.h')