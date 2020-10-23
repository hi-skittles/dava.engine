import os
import shutil
import build_utils
import build_config


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32', 'win10', 'android']
    elif platform == 'darwin':
        return ['macos', 'ios', 'android']
    elif platform == 'linux':
        return ['android', 'linux']
    else:
        return []


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


def get_download_info():
    return "https://share.wargaming.net/f/9f184d56e0/?raw=1"


def _create_folder_if_not_exists(path):
    if not os.path.isdir(path):
        os.makedirs(path)


def _copy_libs_list(src, dst, files_list):
    _create_folder_if_not_exists(dst)
    for file_name in os.listdir(src):
        if file_name in files_list:
            shutil.copyfile(os.path.join(src, file_name), os.path.join(dst, file_name))


def _copy_libs(src, dst, files_ext, recursive = False):
    _create_folder_if_not_exists(dst)
    for file_name in os.listdir(src):
        full_file_name = os.path.join(src, file_name)
        if full_file_name.endswith(files_ext):
            shutil.copyfile(full_file_name, os.path.join(dst, file_name))
        if recursive and os.path.isdir(full_file_name):
            _copy_libs(full_file_name, dst, files_ext, recursive)


def _download_and_extract(working_directory_path):
    source_archive_filepath = os.path.join(working_directory_path, 'physx.zip')

    url = get_download_info()
    build_utils.download(url, source_archive_filepath)
    build_utils.unzip_inplace(source_archive_filepath)
    return os.path.join(working_directory_path, 'PhysX-3.4-master')


@build_utils.run_once
def _patch_sources(common_patch_name, patch_name, working_directory_path):
    build_utils.apply_patch(os.path.abspath(common_patch_name), working_directory_path)
    if patch_name != '':
        build_utils.apply_patch(os.path.abspath(patch_name), working_directory_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources('patch_common.diff', 'patch_win32.diff', working_directory_path)
    project_x86_path = os.path.join(source_folder_path, 'PhysX_3.4', 'Source', 'compiler', 'vc14win32', 'PhysX.sln')
    project_x64_path = os.path.join(source_folder_path, 'PhysX_3.4', 'Source', 'compiler', 'vc14win64', 'PhysX.sln')

    binary_dst_path = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'Win32')
    x86_binary_dst_path = os.path.join(binary_dst_path, 'x86')
    x64_binary_dst_path = os.path.join(binary_dst_path, 'x64')

    toolset=build_config.get_msvc_toolset_ver_win32()

    override_props_file=os.path.abspath('override_win32.props')
    msbuild_args=["/p:ForceImportBeforeCppTargets={}".format(override_props_file)]

    # x86
    build_utils.build_vs(project_x86_path, 'debug', toolset=toolset, msbuild_args=msbuild_args)
    #build_utils.build_vs(project_x86_path, 'profile', toolset=toolset, msbuild_args=msbuild_args)
    #build_utils.build_vs(project_x86_path, 'checked', toolset=toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(project_x86_path, 'release', toolset=toolset, msbuild_args=msbuild_args)
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Bin', 'vc14win32'), x86_binary_dst_path,  '.dll')
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'vc14win32'), x86_binary_dst_path,  '.lib')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'bin', 'vc14win32'), x86_binary_dst_path,  '.dll')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'lib', 'vc14win32'), x86_binary_dst_path,  '.lib')

    # x64
    build_utils.build_vs(project_x64_path, 'debug', 'x64', toolset=toolset, msbuild_args=msbuild_args)
    #build_utils.build_vs(project_x64_path, 'profile', 'x64', toolset=toolset, msbuild_args=msbuild_args)
    #build_utils.build_vs(project_x64_path, 'checked', 'x64', toolset=toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(project_x64_path, 'release', 'x64', toolset=toolset, msbuild_args=msbuild_args)
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Bin', 'vc14win64'), x64_binary_dst_path,  '.dll')
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'vc14win64'), x64_binary_dst_path,  '.lib')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'bin', 'vc14win64'), x64_binary_dst_path,  '.dll')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'lib', 'vc14win64'), x64_binary_dst_path,  '.lib')

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources('patch_common.diff', '', working_directory_path)
    project_path = os.path.join(source_folder_path, 'PhysX_3.4', 'Source', 'compiler', 'xcode_osx64', 'PhysX.xcodeproj')

    build_utils.build_xcode_alltargets(project_path, 'debug')
    #build_utils.build_xcode_alltargets(project_path, 'profile')
    #build_utils.build_xcode_alltargets(project_path, 'checked')
    build_utils.build_xcode_alltargets(project_path, 'release')

    binary_dst_path = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'MacOS')
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'osx64'), binary_dst_path,  '.a')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'lib', 'osx64'), binary_dst_path, '.a')
    _copy_headers(source_folder_path, root_project_path)

def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources('patch_common.diff', 'patch_ios.diff', working_directory_path)

    project_path = os.path.join(source_folder_path, 'PhysX_3.4', 'Source', 'compiler', 'xcode_ios64', 'PhysX.xcodeproj')

    build_utils.build_xcode_alltargets(project_path, 'debug')
    #build_utils.build_xcode_alltargets(project_path, 'profile')
    #build_utils.build_xcode_alltargets(project_path, 'checked')
    build_utils.build_xcode_alltargets(project_path, 'release')

    binary_dst_path = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'iOS')
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'ios64'), binary_dst_path, '.a')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'lib', 'ios64'), binary_dst_path, '.a')
    _copy_headers(source_folder_path, root_project_path)

def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources('patch_common.diff', 'patch_linux.diff', working_directory_path)

    project_path = os.path.join(source_folder_path, 'PhysX_3.4', 'Source', 'compiler', 'linux64')
    build_utils.run_process("make debug", process_cwd=project_path, shell=True)
    build_utils.run_process("make release", process_cwd=project_path, shell=True)
    #build_utils.run_process("make profile", process_cwd=project_path, shell=True)
    #build_utils.run_process("make checked", process_cwd=project_path, shell=True)

    binary_dst_path = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'Linux')
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'linux64'), binary_dst_path, '.a')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'lib', 'linux64'), binary_dst_path, '.a')
    _copy_libs(os.path.join(source_folder_path, 'PhysX_3.4', 'Bin', 'linux64'), binary_dst_path, '.so')
    _copy_libs(os.path.join(source_folder_path, 'PxShared', 'bin', 'linux64'), binary_dst_path, '.so')
    _copy_headers(source_folder_path, root_project_path)

def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources('patch_common.diff', '', working_directory_path)

    source_dir = os.path.join(source_folder_path, 'PhysX_3.4', 'Source')
    shutil.copyfile(os.path.abspath('root.CMakeLists.txt'), os.path.join(source_dir, 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('common.CMakeLists.txt'), os.path.join(source_dir, 'Common', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('LowLevel.CMakeLists.txt'), os.path.join(source_dir,'LowLevel', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('LowLevelAABB.CMakeLists.txt'), os.path.join(source_dir, 'LowLevelAABB', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('LowLevelCloth.CMakeLists.txt'), os.path.join(source_dir, 'LowLevelCloth', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('LowLevelDynamics.CMakeLists.txt'), os.path.join(source_dir, 'LowLevelDynamics', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('LowLevelParticles.CMakeLists.txt'), os.path.join(source_dir, 'LowLevelParticles', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('PhysX.CMakeLists.txt'), os.path.join(source_dir, 'PhysX', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('PhysXCharacterKinematic.CMakeLists.txt'), os.path.join(source_dir, 'PhysXCharacterKinematic', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('PhysXCooking.CMakeLists.txt'), os.path.join(source_dir, 'PhysXCooking', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('PhysXExtensions.CMakeLists.txt'), os.path.join(source_dir, 'PhysXExtensions', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('PhysXVehicle.CMakeLists.txt'), os.path.join(source_dir, 'PhysXVehicle', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('SceneQuery.CMakeLists.txt'), os.path.join(source_dir, 'SceneQuery', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('SimulationController.CMakeLists.txt'), os.path.join(source_dir, 'SimulationController', 'CMakeLists.txt'))

    pxSharedDir = os.path.join(source_folder_path, 'PxShared', 'src')
    shutil.copyfile(os.path.abspath('fastxml.CMakeLists.txt'), os.path.join(pxSharedDir, 'fastxml', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('foundation.CMakeLists.txt'), os.path.join(pxSharedDir, 'foundation', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('task.CMakeLists.txt'), os.path.join(pxSharedDir, 'task', 'CMakeLists.txt'))
    shutil.copyfile(os.path.abspath('pvd.CMakeLists.txt'), os.path.join(pxSharedDir, 'pvd', 'CMakeLists.txt'))

    build_android_armeabiv7a_folder = os.path.join(working_directory_path, 'gen', 'build_android_armeabiv7a')
    build_android_x86_folder = os.path.join(working_directory_path, 'gen', 'build_android_x86')
    toolchain_filepath = os.path.join(root_project_path, 'Sources/CMake/Toolchains/android.toolchain.cmake')
    android_ndk_folder_path = build_utils.get_android_ndk_path()

    build_utils.cmake_generate_build_ndk(build_android_armeabiv7a_folder, source_dir,
                                         android_ndk_folder_path, 'armeabi-v7a',
                                         ['-DFRAMEWORK_ROOT_PATH=' + root_project_path])

    binary_dst_path = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'Android')
    _copy_libs(build_android_armeabiv7a_folder, os.path.join(binary_dst_path, 'armeabi-v7a'), '.a', True)

    #build_utils.cmake_generate_build_ndk(build_android_x86_folder, source_dir,
    #                                     android_ndk_folder_path, 'x86',
    #                                     ['-DFRAMEWORK_ROOT_PATH=' + root_project_path, '-Wno-dev'])
    _copy_headers(source_folder_path, root_project_path)

def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources('patch_common.diff', 'patch_win10.diff', working_directory_path)

    project_x86_path = os.path.join(source_folder_path, 'PhysX_3.4', 'Source', 'compiler', 'vc14win32', 'PhysX.sln')
    project_x64_path = os.path.join(source_folder_path, 'PhysX_3.4', 'Source', 'compiler', 'vc14win64', 'PhysX.sln')

    binary_dst_path = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'Win10')
    x86_binary_dst_path = os.path.join(binary_dst_path, 'Win32')
    x64_binary_dst_path = os.path.join(binary_dst_path, 'x64')

    debug_libs_list_x86 = ['LowLevelAABBDEBUG.lib',
                     'LowLevelClothDEBUG.lib',
                     'LowLevelDEBUG.lib',
                     'LowLevelDynamicsDEBUG.lib',
                     'LowLevelParticlesDEBUG.lib',
                     'nvToolsExt32_1.dll',
                     'nvToolsExt64_1.dll',
                     'PhysX3CharacterKinematicDEBUG_x86.dll',
                     'PhysX3CharacterKinematicDEBUG_x86.lib',
                     'PhysX3CommonDEBUG_x86.dll',
                     'PhysX3CommonDEBUG_x86.lib',
                     'PhysX3CookingDEBUG_x86.dll',
                     'PhysX3CookingDEBUG_x86.lib',
                     'PhysX3DEBUG_x86.dll',
                     'PhysX3DEBUG_x86.lib',
                     'PhysX3ExtensionsDEBUG.lib',
                     'PhysX3VehicleDEBUG.lib',
                     'PsFastXmlDEBUG_x86.lib',
                     'PxFoundationDEBUG_x86.dll',
                     'PxFoundationDEBUG_x86.lib',
                     'PxPvdSDKDEBUG_x86.dll',
                     'PxPvdSDKDEBUG_x86.lib',
                     'PxTaskDEBUG_x86.lib',
                     'SceneQueryDEBUG.lib',
                     'SimulationControllerDEBUG.lib']

    release_libs_list_x86 = map(lambda libName: libName.replace('DEBUG', ''), debug_libs_list_x86)
    debug_libs_list_x64 = map(lambda libName: libName.replace('x86', 'x64'), debug_libs_list_x86)
    release_libs_list_x64 = map(lambda libName: libName.replace('x86', 'x64'), release_libs_list_x86)

    build_utils.build_vs(project_x86_path, 'debug')
    #build_utils.build_vs(project_x86_path, 'profile')
    #build_utils.build_vs(project_x86_path, 'checked')
    build_utils.build_vs(project_x86_path, 'release')

    debug_win32_dst_path = os.path.join(binary_dst_path, 'Win32', 'Debug')
    release_win32_dst_path = os.path.join(binary_dst_path, 'Win32', 'Release')

    debug_win64_dst_path = os.path.join(binary_dst_path, 'x64', 'Debug')
    release_win64_dst_path = os.path.join(binary_dst_path, 'x64', 'Release')

    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Bin', 'vc14win32'), debug_win32_dst_path, debug_libs_list_x86)
    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'vc14win32'), debug_win32_dst_path, debug_libs_list_x86)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'bin', 'vc14win32'), debug_win32_dst_path, debug_libs_list_x86)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'lib', 'vc14win32'), debug_win32_dst_path, debug_libs_list_x86)

    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Bin', 'vc14win32'), release_win32_dst_path, release_libs_list_x86)
    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'vc14win32'), release_win32_dst_path, release_libs_list_x86)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'bin', 'vc14win32'), release_win32_dst_path, release_libs_list_x86)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'lib', 'vc14win32'), release_win32_dst_path, release_libs_list_x86)

    build_utils.build_vs(project_x64_path, 'debug', 'x64')
    #build_utils.build_vs(project_x64_path, 'profile', 'x64')
    #build_utils.build_vs(project_x64_path, 'checked', 'x64')
    build_utils.build_vs(project_x64_path, 'release', 'x64')
    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Bin', 'vc14win64'), debug_win64_dst_path, debug_libs_list_x64)
    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'vc14win64'), debug_win64_dst_path, debug_libs_list_x64)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'bin', 'vc14win64'), debug_win64_dst_path, debug_libs_list_x64)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'lib', 'vc14win64'), debug_win64_dst_path, debug_libs_list_x64)

    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Bin', 'vc14win64'), release_win64_dst_path, release_libs_list_x64)
    _copy_libs_list(os.path.join(source_folder_path, 'PhysX_3.4', 'Lib', 'vc14win64'), release_win64_dst_path, release_libs_list_x64)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'bin', 'vc14win64'), release_win64_dst_path, release_libs_list_x64)
    _copy_libs_list(os.path.join(source_folder_path, 'PxShared', 'lib', 'vc14win64'), release_win64_dst_path, release_libs_list_x64)
    _copy_headers(source_folder_path, root_project_path)

def _copy_headers(source_folder_path, root_project_path):
    copy_to_folder = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'Include', 'physx')
    copy_from_folder = os.path.join(source_folder_path, 'PhysX_3.4', 'Include')
    _create_folder_if_not_exists(copy_to_folder)
    build_utils.clean_copy_includes(copy_from_folder, copy_to_folder)

    shared_copy_to_folder = os.path.join(root_project_path, 'Modules', 'Physics', 'Libs', 'Include', 'PxShared')
    shared_copy_from_folder = os.path.join(source_folder_path, 'PxShared', 'include')
    build_utils.clean_copy_includes(shared_copy_from_folder, shared_copy_to_folder)
