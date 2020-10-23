import os
import shutil
import build_utils
import build_config


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
    return 'Libs/NVTT'


def _build_win32(working_directory_path, root_project_path):
    source_folder_path=os.path.join(root_project_path, 'Libs/NVTT')

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, '_build'),
        source_folder_path,
        root_project_path,
        "dxt.sln",
        "dxt",
        "dxt.lib", "dxt.lib",
        "libdxt.lib", "libdxt.lib",
        "libdxt.lib", "libdxt.lib",
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path=os.path.join(root_project_path, 'Libs/NVTT')

    # TODO: implement macos
    raise RuntimeError('Building for macos is not implemented. Do it yourself')


def _build_linux(working_directory_path, root_project_path):
    source_folder_path=os.path.join(root_project_path, 'Libs/NVTT')

    build_utils.build_and_copy_libraries_linux_cmake(
        os.path.join(working_directory_path, '_build'),
        source_folder_path,
        root_project_path,
        target='all',
        lib_name='libdxt.a')
    
    _copy_headers(source_folder_path, root_project_path)


def _copy_headers(source_folder_path, root_project_path):
    src_dir = os.path.join(source_folder_path, 'Sources/nvtt')
    include_path = os.path.join(root_project_path, 'Libs/include/libdxt')

    build_utils.copy_files_by_name(src_dir, include_path, ['nvtt.h', 'nvtt_extra.h'])
