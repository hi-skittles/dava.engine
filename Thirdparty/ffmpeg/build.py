import os
import shutil
import build_utils

# This script follows officially supported build instructions
# (https://www.ffmpeg.org/platform.html#Microsoft-Visual-C_002b_002b-or-Intel-C_002b_002b-Compiler-for-Windows)
# So, the following dependencies should be satisified before invoking it:
# - MSYS2 (http://msys2.github.io/) should be installed and its root should be in the PATH
# - base-devel, diffutils, yasm should be installed via pacman:
#   pacman -S base-devel
#   pacman -S diffutils
#   pacman -S yasm


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32']
    else:
        return []


def get_dependencies_for_target(target):
    return []


def build_for_target(target, working_directory_path, root_project_path):
    if target == 'win32':
        return _build_win32(working_directory_path, root_project_path)


def get_download_info():
    return {'source': 'https://github.com/FFmpeg/FFmpeg/archive/n3.1.4.zip',
            'c99-to-c89': 'http://download.videolan.org/pub/contrib/c99-to-c89/1.0.3/c99-to-c89-1.0.3.zip'}


def _get_downloaded_archive_inner_dir():
    # Because ffmpeg archive inner folder and archive file name do not match
    # If you change download link - change this one too
    return 'FFmpeg-n3.1.4'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'ffmpeg_source')
    c99_to_c89_folder_path = os.path.join(working_directory_path, 'c99-to-c89')

    urls = get_download_info()
    source_url = urls['source']
    c99_to_c89_url = urls['c99-to-c89']
    build_utils.download_and_extract(
        source_url,
        working_directory_path,
        source_folder_path,
        _get_downloaded_archive_inner_dir())
    build_utils.download_and_extract(
        c99_to_c89_url,
        os.path.join(working_directory_path, 'c99_to_c89_folder_path'),
        c99_to_c89_folder_path)

    return source_folder_path, c99_to_c89_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
    shutil.copyfile(
        'inttypes.h', os.path.join(source_folder_path, 'inttypes.h'))
    shutil.copyfile(
        'stdint.h', os.path.join(source_folder_path, 'stdint.h'))


def _build_win32(working_directory_path, root_project_path):
    source_folder_path, c99_to_c89_folder_path = _download_and_extract(
        working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    install_path_debug_x86 = os.path.join(
        working_directory_path, 'gen/install_win32_debug_x86')
    install_path_debug_x64 = os.path.join(
        working_directory_path, 'gen/install_win32_debug_x64')
    install_path_release_x86 = os.path.join(
        working_directory_path, 'gen/install_win32_release_x86')
    install_path_release_x64 = os.path.join(
        working_directory_path, 'gen/install_win32_release_x64')

    msys2_cmd_template = ['msys2_shell.cmd', '-use-full-path', '-l', '-c']

    # make clean is so dumb
    bash_arg_clean_part = (' && make clean && rm compat/msvcrt/snprintf.d'
                           ' && rm compat/msvcrt/snprintf.o'
                           ' && rm compat/strtod.o && rm compat/strtod.d')
    bash_arg_template = ('cd {} && ./configure {} && '
                         'make && make install' + bash_arg_clean_part)

    configure_args_debug_template = (
        '--toolchain=msvc --prefix={} --disable-all --enable-static '
        '--disable-shared --enable-avcodec --enable-avdevice --enable-avfilter '
        '--enable-avformat --enable-swresample --enable-swscale '
        '--enable-gpl --enable-postproc')
    configure_args_release_template = configure_args_debug_template + ' --disable-debug'

    env_x86 = build_utils.get_win32_vs_x86_env()
    env_x64 = build_utils.get_win32_vs_x64_env()
    env_x86['PATH'] = c99_to_c89_folder_path + ';' + env_x86['PATH']
    env_x64['PATH'] = c99_to_c89_folder_path + ';' + env_x64['PATH']

    configure_args_debug_x86 = configure_args_debug_template.format(
        _path_to_msys_path(install_path_debug_x86))
    bash_arg = bash_arg_template.format(
        _path_to_msys_path(source_folder_path), configure_args_debug_x86)
    cmd = msys2_cmd_template[:]
    cmd.append(bash_arg)
    build_utils.run_process(
        cmd, process_cwd=source_folder_path, environment=env_x86)

    configure_args_release_x86 = configure_args_release_template.format(
        _path_to_msys_path(install_path_release_x86))
    bash_arg = bash_arg_template.format(
        _path_to_msys_path(source_folder_path), configure_args_release_x86)
    cmd = msys2_cmd_template[:]
    cmd.append(bash_arg)
    build_utils.run_process(
        cmd, process_cwd=source_folder_path, environment=env_x86)

    configure_args_debug_x64 = configure_args_debug_template.format(
        _path_to_msys_path(install_path_debug_x64))
    bash_arg = bash_arg_template.format(
        _path_to_msys_path(source_folder_path), configure_args_debug_x64)
    cmd = msys2_cmd_template[:]
    cmd.append(bash_arg)
    build_utils.run_process(
        cmd, process_cwd=source_folder_path, environment=env_x64)

    configure_args_release_x64 = configure_args_release_template.format(
        _path_to_msys_path(install_path_release_x64))
    bash_arg = bash_arg_template.format(
        _path_to_msys_path(source_folder_path), configure_args_release_x64)
    cmd = msys2_cmd_template[:]
    cmd.append(bash_arg)
    build_utils.run_process(
        cmd, process_cwd=source_folder_path, environment=env_x64)

    _copy_headers(install_path_debug_x86, root_project_path)

    _copy_libraries(
        install_path_debug_x86,
        os.path.join(root_project_path, 'Libs/lib_CMake/win/x86/Debug/'))
    _copy_libraries(
        install_path_release_x86,
        os.path.join(root_project_path, 'Libs/lib_CMake/win/x86/Release/'))
    _copy_libraries(
        install_path_debug_x64,
        os.path.join(root_project_path, 'Libs/lib_CMake/win/x64/Debug/'))
    _copy_libraries(
        install_path_release_x64,
        os.path.join(root_project_path, 'Libs/lib_CMake/win/x64/Release/'))


def _path_to_msys_path(path):
    return '/{}'.format(path.replace('\\', '/').replace(':', ''))


def _copy_libraries(install_dir_path, target_libs_folder_path):
    libavcodec_from = os.path.join(install_dir_path, 'lib/libavcodec.a')
    libavcodec_to = os.path.join(target_libs_folder_path, 'avcodec.lib')
    shutil.copyfile(libavcodec_from, libavcodec_to)

    libavdevice_from = os.path.join(install_dir_path, 'lib/libavdevice.a')
    libavdevice_to = os.path.join(target_libs_folder_path, 'avdevice.lib')
    shutil.copyfile(libavdevice_from, libavdevice_to)

    libavfilter_from = os.path.join(install_dir_path, 'lib/libavfilter.a')
    libavfilter_to = os.path.join(target_libs_folder_path, 'avfilter.lib')
    shutil.copyfile(libavfilter_from, libavfilter_to)

    libavformat_from = os.path.join(install_dir_path, 'lib/libavformat.a')
    libavformat_to = os.path.join(target_libs_folder_path, 'avformat.lib')
    shutil.copyfile(libavformat_from, libavformat_to)

    libavutil_from = os.path.join(install_dir_path, 'lib/libavutil.a')
    libavutil_to = os.path.join(target_libs_folder_path, 'avutil.lib')
    shutil.copyfile(libavutil_from, libavutil_to)

    libpostproc_from = os.path.join(install_dir_path, 'lib/libpostproc.a')
    libpostproc_to = os.path.join(target_libs_folder_path, 'postproc.lib')
    shutil.copyfile(libpostproc_from, libpostproc_to)

    libswresample_from = os.path.join(install_dir_path, 'lib/libswresample.a')
    libswresample_to = os.path.join(target_libs_folder_path, 'swresample.lib')
    shutil.copyfile(libswresample_from, libswresample_to)

    libswscale_from = os.path.join(install_dir_path, 'lib/libswscale.a')
    libswscale_to = os.path.join(target_libs_folder_path, 'swscale.lib')
    shutil.copyfile(libswscale_from, libswscale_to)


def _copy_headers(install_dir_path, root_project_path):
    include_folder_from = os.path.join(install_dir_path, 'include')
    include_folder_to = os.path.join(root_project_path, 'Libs/include/ffmpeg/')

    if os.path.exists(include_folder_to):
        shutil.rmtree(include_folder_to)
    shutil.copytree(include_folder_from, include_folder_to)
