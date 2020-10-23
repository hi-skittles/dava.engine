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
    elif target == 'linux':
        _build_linux(working_directory_path, root_project_path)


def get_download_url():
    return 'https://github.com/10gen/mongo-c-driver-legacy/archive/v0.6.tar.gz'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'mongodb_source')
    build_utils.download_and_extract(
        get_download_url(),
        working_directory_path,
        source_folder_path,
        'mongo-c-driver-legacy-0.6')
    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
    shutil.copyfile(
        'CMakeLists.txt',
        os.path.join(source_folder_path, 'CMakeLists.txt'))
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
        'mongodb.sln', 'mongodb',
        'mongodb.lib', 'mongodb.lib',
        'libmongodb.lib', 'libmongodb.lib',
        'libmongodb.lib', 'libmongodb.lib',
        static_runtime=False)

    _copy_headers(os.path.join(source_folder_path, 'src'), root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_win10_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'mongodb.sln', 'mongodb',
        'mongodb.lib', 'mongodb.lib',
        'libmongodb_wind.lib', 'libmongodb_win.lib',
        'libmongodb_wind.lib', 'libmongodb_win.lib',
        'libmongodb_wind.lib', 'libmongodb_win.lib')

    _copy_headers(os.path.join(source_folder_path, 'src'), root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'mongodb.xcodeproj', 'mongodb',
        'libmongodb.a',
        'libmongodb_macos.a')

    _copy_headers(os.path.join(source_folder_path, 'src'), root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_ios_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'mongodb.xcodeproj', 'mongodb',
        'libmongodb.a',
        'libmongodb_ios.a')

    _copy_headers(os.path.join(source_folder_path, 'src'), root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_android_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libmongodb.a',
        'libmongodb.a')

    _copy_headers(os.path.join(source_folder_path, 'src'), root_project_path)


def _build_linux(working_directory_path, root_project_path):
    # TODO: check building
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_linux_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        target='all',
        lib_name='libmongodb.a')
    
    _copy_headers(source_folder_path, root_project_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/mongodb')
    build_utils.clean_copy_includes(source_folder_path, include_path)