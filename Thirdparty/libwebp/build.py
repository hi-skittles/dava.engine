import os
import shutil
import build_utils


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
    return 'https://github.com/webmproject/libwebp/archive/v0.4.3.tar.gz'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'libwebp_source')
    url = get_download_info()
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        'libwebp-0.4.3')
    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
    build_utils.apply_patch(
        os.path.abspath('patch_win.diff'), working_directory_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    # x86
    x86_env = build_utils.get_win32_vs_x86_env()
    build_utils.run_process(
        ['nmake', '-f', 'Makefile.vc', 'CFG=release-static', 'RTLIBCFG=dynamic', 'OBJDIR=output'],
        process_cwd=source_folder_path,
        environment=x86_env,
        shell=True)
    build_utils.run_process(
        ['nmake', '-f', 'Makefile.vc', 'CFG=debug-static', 'RTLIBCFG=dynamic', 'OBJDIR=output'],
        process_cwd=source_folder_path,
        environment=x86_env,
        shell=True)

    # x64
    x64_env = build_utils.get_win32_vs_x64_env()
    build_utils.run_process(
        ['nmake', '-f', 'Makefile.vc', 'CFG=release-static', 'RTLIBCFG=dynamic', 'OBJDIR=output'],
        process_cwd=source_folder_path,
        environment=x64_env,
        shell=True)
    build_utils.run_process(
        ['nmake', '-f', 'Makefile.vc', 'CFG=debug-static', 'RTLIBCFG=dynamic', 'OBJDIR=output'],
        process_cwd=source_folder_path,
        environment=x64_env,
        shell=True)

    libs_win_root = os.path.join(root_project_path, 'Libs/lib_CMake/win')
    shutil.copyfile(
        os.path.join(source_folder_path, 'output/debug-static/x86/lib/libwebp_debug.lib'),
        os.path.join(libs_win_root, 'x86/Debug/libwebp.lib'))
    shutil.copyfile(
        os.path.join(source_folder_path, 'output/release-static/x86/lib/libwebp.lib'),
        os.path.join(libs_win_root, 'x86/Release/libwebp.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'output/debug-static/x64/lib/libwebp_debug.lib'),
        os.path.join(libs_win_root, 'x64/Debug/libwebp.lib'))
    shutil.copyfile(
        os.path.join(source_folder_path, 'output/release-static/x64/lib/libwebp.lib'),
        os.path.join(libs_win_root, 'x64/Release/libwebp.lib'))

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_win10_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libwebp.sln', 'webp',
        'webp.lib', 'webp.lib',
        'libwebp.lib', 'libwebp.lib',
        'libwebp.lib', 'libwebp.lib',
        'libwebp.lib', 'libwebp.lib',
        ['-DCMAKE_SYSTEM_PROCESSOR=arm'])

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libwebp.xcodeproj', 'webp',
        'libwebp.a',
        'libwebp.a')

    _copy_headers(source_folder_path, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_ios_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libwebp.xcodeproj', 'webp',
        'libwebp.a',
        'libwebp.a')

    _copy_headers(source_folder_path, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_android_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libwebp.a',
        'libwebp.a',
        arm_abi='armeabi-v7a')

    _copy_headers(source_folder_path, root_project_path)

def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_linux_cmake(
        gen_folder_path=os.path.join(working_directory_path, 'gen'),
        source_folder_path=source_folder_path,
        root_project_path=root_project_path,
        target="all",
        lib_name='libwebp.a')

    _copy_headers(source_folder_path, root_project_path)

def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/webp')
    build_utils.copy_files(
        os.path.join(source_folder_path, 'src/webp'), include_path, '*.h')
