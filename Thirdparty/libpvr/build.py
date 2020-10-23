import os
import shutil
import build_utils
import build_config


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
    return 'Libs/libpvr_win32'


def _build_win32(working_directory_path, root_project_path):
    source_folder_path=os.path.join(root_project_path, get_download_info())

    (build_folder_x86,build_folder_x64)=build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        "PVRTexLib.sln",
        "PVRTexLib",
        "PVRTexLib.lib", "PVRTexLib.lib",
        "PVRTexLib.lib", "PVRTexLib.lib",
        "PVRTexLib.lib", "PVRTexLib.lib",
        static_runtime=True)

    # PVRTexLib is a DLL library which wraps original static library PVRTexLib.lib to 
    # make msvc2017 linker happy. So use only release version of DLL and import library
    lib_path_x86=os.path.join(root_project_path, 'Libs/lib_CMake/win/x86')
    build_folder_x86=os.path.join(build_folder_x86, 'Release')
    shutil.copyfile(os.path.join(build_folder_x86, 'PVRTexLib.lib'), os.path.join(lib_path_x86, 'Debug/PVRTexLib.lib'))
    shutil.copyfile(os.path.join(build_folder_x86, 'PVRTexLib.lib'), os.path.join(lib_path_x86, 'Release/PVRTexLib.lib'))
    shutil.copyfile(os.path.join(build_folder_x86, 'PVRTexLib.dll'), os.path.join(lib_path_x86, 'Release/PVRTexLib.dll'))

    lib_path_x64=os.path.join(root_project_path, 'Libs/lib_CMake/win/x64')
    build_folder_x64=os.path.join(build_folder_x64, 'Release')
    shutil.copyfile(os.path.join(build_folder_x64, 'PVRTexLib.lib'), os.path.join(lib_path_x64, 'Debug/PVRTexLib.lib'))
    shutil.copyfile(os.path.join(build_folder_x64, 'PVRTexLib.lib'), os.path.join(lib_path_x64, 'Release/PVRTexLib.lib'))
    shutil.copyfile(os.path.join(build_folder_x64, 'PVRTexLib.dll'), os.path.join(lib_path_x64, 'Release/PVRTexLib.dll'))
