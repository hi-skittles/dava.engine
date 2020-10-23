import os
import build_utils
import build_config
import shutil


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
    return 'https://download.microsoft.com/download/C/3/0/C30E4049-F205-475F-BEA1-DD069207E8FC/Detours%20Version%203.0%20Build_343.zip'


def _get_downloaded_archive_inner_dir():
    # Because archive inner folder and archive file name do not match
    # If you change download link - change this one too
    return 'Detours'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'detours_source')

    url = get_download_info()
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        _get_downloaded_archive_inner_dir())

    return source_folder_path


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    build_utils.apply_patch(os.path.abspath('patch_debug_and_dynamic_runtime.diff'), working_directory_path)

    # Build only detours library excluding examples
    makefile_path=os.path.join(source_folder_path, 'src')

    # x86 debug build
    env=build_utils.get_win32_vs_x86_env()
    env['DETOURS_TARGET_PROCESSOR']='X86'
    env['DETOURS_RUNTIME']='DYNAMIC'
    env['DETOURS_BUILD_MODE']='DEBUG'
    env['DETOURS_CONFIG']='.debug'
    build_utils.run_process(
        ['nmake'],
        process_cwd=makefile_path,
        shell=True,
        environment=env)

    # x86 release build
    env=build_utils.get_win32_vs_x86_env()
    env['DETOURS_TARGET_PROCESSOR']='X86'
    env['DETOURS_RUNTIME']='DYNAMIC'
    env['DETOURS_BUILD_MODE']='RELEASE'
    env['DETOURS_CONFIG']='.release'
    build_utils.run_process(
        ['nmake'],
        process_cwd=makefile_path,
        shell=True,
        environment=env)

    # x64 debug build
    env=build_utils.get_win32_vs_x64_env()
    env['DETOURS_TARGET_PROCESSOR']='X64'
    env['DETOURS_RUNTIME']='DYNAMIC'
    env['DETOURS_BUILD_MODE']='DEBUG'
    env['DETOURS_CONFIG']='.debug'
    build_utils.run_process(
        ['nmake'],
        process_cwd=makefile_path,
        shell=True,
        environment=env)

    # x64 release build
    env=build_utils.get_win32_vs_x64_env()
    env['DETOURS_TARGET_PROCESSOR']='X64'
    env['DETOURS_RUNTIME']='DYNAMIC'
    env['DETOURS_BUILD_MODE']='RELEASE'
    env['DETOURS_CONFIG']='.release'
    build_utils.run_process(
        ['nmake'],
        process_cwd=makefile_path,
        shell=True,
        environment=env)

    libraries_win_root=os.path.join(root_project_path, 'Libs/lib_CMake/win')
    shutil.copyfile(os.path.join(source_folder_path, 'lib.X86.debug/detours.lib'),
                    os.path.join(libraries_win_root, 'x86/Debug/detours.lib'))
    shutil.copyfile(os.path.join(source_folder_path, 'lib.X86.release/detours.lib'),
                    os.path.join(libraries_win_root, 'x86/Release/detours.lib'))

    shutil.copyfile(os.path.join(source_folder_path, 'lib.X64.debug/detours.lib'),
                    os.path.join(libraries_win_root, 'x64/Debug/detours.lib'))
    shutil.copyfile(os.path.join(source_folder_path, 'lib.X64.release/detours.lib'),
                    os.path.join(libraries_win_root, 'x64/Release/detours.lib'))

    _copy_headers(source_folder_path, root_project_path)

def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/detours')
    build_utils.copy_files_by_name(os.path.join(source_folder_path, 'src'),
                                   include_path,
                                   ['detver.h',
                                    'detours.h'
                                   ])
