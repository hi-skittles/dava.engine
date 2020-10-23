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
    return 'Programs/ColladaConverter/Collada15'

def _download(working_directory_path, root_project_path):
    # FCollada source files are inside dava.engine
    # Copy source files to shadow directory before building
    source_path = os.path.join(root_project_path, get_download_info())
    target_path = os.path.join(working_directory_path, 'Collada15')
    shutil.rmtree(target_path, ignore_errors=True)
    shutil.copytree(source_path, target_path)

    return target_path


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download(working_directory_path, root_project_path)

    vc_solution_path = os.path.join(source_folder_path, 'FCollada')
    vc_solution_file = os.path.join(vc_solution_path, 'FColladaVS2010.sln')

    override_props_file=os.path.abspath('override.props')
    msbuild_args=[
        "/p:ForceImportBeforeCppTargets={}".format(override_props_file),
    ]

    toolset=build_config.get_msvc_toolset_ver_win32()
    build_utils.build_vs(vc_solution_file, 'Debug', 'Win32', 'FColladaVS2010', toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc_solution_file, 'Release', 'Win32', 'FColladaVS2010', toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc_solution_file, 'Debug', 'x64', 'FColladaVS2010', toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc_solution_file, 'Release', 'x64', 'FColladaVS2010', toolset, msbuild_args=msbuild_args)

    shutil.copyfile(os.path.join(vc_solution_path, 'Output/Debug Win32/FColladaVS2010.lib'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/win/x86/Debug/FColladaVS2010.lib'))
    shutil.copyfile(os.path.join(vc_solution_path, 'Output/Release Win32/FColladaVS2010.lib'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/win/x86/Release/FColladaVS2010.lib'))
    shutil.copyfile(os.path.join(vc_solution_path, 'Output/Debug x64/FColladaVS2010.lib'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/win/x64/Debug/FColladaVS2010.lib'))
    shutil.copyfile(os.path.join(vc_solution_path, 'Output/Release x64/FColladaVS2010.lib'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/win/x64/Release/FColladaVS2010.lib'))


def _build_macos(working_directory_path, root_project_path):
    # TODO: build macos
    raise RuntimeError('Building for macos is not implemented. Do it yourself')
