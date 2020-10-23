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
    return []


def build_for_target(target, working_directory_path, root_project_path):
    if target == 'win32':
        _build_win32(working_directory_path, root_project_path)
    elif target == 'macos':
        _build_macos(working_directory_path, root_project_path)
    elif target == 'linux':
        _build_linux(working_directory_path, root_project_path)


def get_download_info():
    return 'https://github.com/google/googletest.git'


def _copyLib(src, dst):
    if not os.path.isdir(dst):
        os.makedirs(dst)
    shutil.copy2(src, dst)


def _download(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'googletest')

    build_utils.run_process(
        ['git', 'clone', get_download_info()],
        process_cwd=working_directory_path,
        shell=True)
    build_utils.run_process(
        ['git', 'checkout', 'tags/release-1.8.0'],
        process_cwd=source_folder_path,
        shell=True)

    return source_folder_path


@build_utils.run_once
def _patch_sources(working_directory_path):
    build_utils.apply_patch(
        os.path.abspath('patch.diff'), working_directory_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download(working_directory_path)
    _patch_sources(source_folder_path)

    override_props_file=os.path.abspath('override.props')
    msbuild_args=[
        "/p:ForceImportBeforeCppTargets={}".format(override_props_file),
    ]

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(source_folder_path, '_build'),
        source_folder_path,
        root_project_path,
        'googletest-distribution.sln', 'gmock',
        'gmock.lib', 'gmock.lib',
        'gmock.lib', 'gmock.lib',
        'gmock.lib', 'gmock.lib',
        msbuild_args=msbuild_args,
        target_lib_subdir='googlemock',
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download(working_directory_path)
    _patch_sources(source_folder_path)

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(source_folder_path, '_build'),
        source_folder_path,
        root_project_path,
        'googletest-distribution.xcodeproj', 'gmock',
        'libgmock.a', 'libgmock.a',
        target_lib_subdir='googlemock')

    _copy_headers(source_folder_path, root_project_path)


def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download(working_directory_path)
    _patch_sources(source_folder_path)

    build_utils.build_and_copy_libraries_linux_cmake(
        os.path.join(source_folder_path, '_build'),
        source_folder_path,
        root_project_path,
        target='all',
        lib_name='libgmock.a',
        target_lib_subdir='googlemock')

    _copy_headers(source_folder_path, root_project_path)


def _copy_headers(source_folder_path, root_project_path):
    gmock_from_dir = os.path.join(
        source_folder_path, 'googlemock/include/gmock')
    gmock_to_dir = os.path.join(
        root_project_path, 'Libs/include/googlemock/gmock')
    gtest_from_dir = os.path.join(
        source_folder_path, 'googletest/include/gtest')
    gtest_to_dir = os.path.join(
        root_project_path, 'Libs/include/googlemock/gtest')
    scripts_from_dir = os.path.join(
        source_folder_path, 'googlemock/scripts')
    scripts_to_dir = os.path.join(
        root_project_path, 'Thirdparty/googlemock/scripts')
    build_utils.clean_copy_includes(gmock_from_dir, gmock_to_dir)
    build_utils.clean_copy_includes(gtest_from_dir, gtest_to_dir)
    build_utils.clean_copy_includes(scripts_from_dir, scripts_to_dir)
