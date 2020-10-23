import os
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
    if target in ['win32', 'win10']:
        return ['zlib']
    else:
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
    return 'http://download.savannah.gnu.org/releases/freetype/freetype-2.7.tar.gz'


def _cmake_args(additional_args=[]):
    default_args = [
        '-DFREETYPE_NO_DIST=true',
        '-DWITH_BZip2=OFF',
        '-DWITH_HarfBuzz=OFF',
        '-DWITH_PNG=OFF',
        '-DWITH_ZLIB=ON'
    ]
    default_args.extend(additional_args)
    return default_args


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(
        working_directory_path, 'freetype_source')

    url = get_download_info()
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        build_utils.get_url_file_name_no_ext(url))

    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path):
    build_utils.apply_patch(
        os.path.abspath('patch.diff'), working_directory_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    # TODO: Get zlib paths for correct arch and configuration
    zlib_cmake_flags = [
        '-DZLIB_LIBRARY=' + os.path.join(root_project_path, 'Libs/lib_CMake/win/x86/Release/zlib.lib'),
        '-DZLIB_INCLUDE_DIR=' + os.path.join(root_project_path, 'Libs/zlib')
    ]

    build_utils.build_and_copy_libraries_win32_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'freetype.sln', 'freetype',
        'freetyped.lib', 'freetype.lib',
        'freetype.lib', 'freetype.lib',
        'freetype.lib', 'freetype.lib',
        cmake_additional_args=_cmake_args(zlib_cmake_flags),
        static_runtime=False)

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    # TODO: Get zlib paths for correct arch and configuration
    zlib_cmake_flags = [
        '-DZLIB_LIBRARY=' + os.path.join(root_project_path, 'Libs/lib_CMake/win10/Win32/Release/zlib.lib'),
        '-DZLIB_INCLUDE_DIR=' + os.path.join(root_project_path, 'Libs/zlib')
    ]

    build_utils.build_and_copy_libraries_win10_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'freetype.sln', 'freetype',
        'freetyped.lib', 'freetype.lib',
        'freetype.lib', 'freetype.lib',
        'freetype.lib', 'freetype.lib',
        'freetype.lib', 'freetype.lib',
        cmake_additional_args=_cmake_args(zlib_cmake_flags))

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_macos_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'freetype.xcodeproj', 'freetype',
        'libfreetype.a',
        'libfreetype_macos.a',
        cmake_additional_args=_cmake_args())

    _copy_headers(source_folder_path, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_ios_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'freetype.xcodeproj', 'freetype',
        'libfreetype.a',
        'libfreetype_ios.a',
        cmake_additional_args=_cmake_args(['-DIOS_PLATFORM=OS']))

    _copy_headers(source_folder_path, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_android_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        'libfreetype.a',
        'libfreetype.a',
        cmake_additional_args=_cmake_args())

    _copy_headers(source_folder_path, root_project_path)


def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path)

    build_utils.build_and_copy_libraries_linux_cmake(
        os.path.join(working_directory_path, 'gen'),
        source_folder_path,
        root_project_path,
        target='all',
        lib_name='libfreetype.a',
        cmake_additional_args=_cmake_args())

    _copy_headers(source_folder_path, root_project_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/freetype')
    build_utils.copy_folder_recursive(
        os.path.join(source_folder_path, 'include/freetype'), include_path)
