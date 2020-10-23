import os
import shutil
import build_utils
import build_config


def get_supported_targets(platform):
    if platform == 'win32':
        return ['win32', 'win10']
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
    return 'ftp://xmlsoft.org/libxml2/libxml2-2.7.8.tar.gz'


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'libxml2_source')

    url = get_download_info()
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        build_utils.get_url_file_name_no_ext(url))

    return source_folder_path


def _patch_sources_android(source_folder_path, working_directory_path):
    try:
        if source_folder_path in _patch_sources_android.cache:
            return
    except AttributeError:
        _patch_sources_android.cache = []
        pass

    build_utils.apply_patch(
        os.path.abspath('patch_android.diff'),
        working_directory_path)

    _patch_sources_android.cache.append(source_folder_path)


def _patch_sources_windows(source_folder_path, working_directory_path):
    try:
        if source_folder_path in _patch_sources_windows.cache:
            return
    except AttributeError:
        _patch_sources_windows.cache = []
        pass

    """ Patch for libxml2 v2.7.8 """
    build_utils.apply_patch(
        os.path.abspath('patch_win_v2.7.8.diff'),
        working_directory_path)

    """ Patch for libxml2 v2.9.0 and later
    shutil.copytree(
        os.path.join(source_folder_path, 'win32/VC10'),
        os.path.join(source_folder_path, 'win32/Win10'))
    build_utils.apply_patch(
        os.path.abspath('patch_win.diff'),
        working_directory_path)
    """

    _patch_sources_windows.cache.append(source_folder_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources_windows(source_folder_path, working_directory_path)

    """ Build instructions for libxml2 v2.7.8 """
    libraries_win_root=os.path.join(root_project_path, 'Libs/lib_CMake/win')
    makefile_path=os.path.join(source_folder_path, 'win32')

    cscript_debug=['cscript', 'configure.js', 'compiler=msvc', 'static=yes', 'iconv=no', 'ftp=no', 'http=no', 'cruntime=/MDd', 'debug=yes']
    cscript_release=['cscript', 'configure.js', 'compiler=msvc', 'static=yes', 'iconv=no', 'ftp=no', 'http=no', 'cruntime=/MD', 'debug=no']
    nmake_clean=['nmake', 'clean']

    # Generate makefile for debug build
    build_utils.run_process(
        cscript_debug,
        process_cwd=makefile_path,
        shell=True,
        environment=None)

    # Build and copy x86 debug static lib
    build_utils.run_process(
        ['nmake', '/f', 'Makefile', 'libxmla'],
        process_cwd=makefile_path,
        shell=True,
        environment=build_utils.get_win32_vs_x86_env())
    shutil.copyfile(os.path.join(makefile_path, 'bin.msvc/libxml2_a.lib'),
                    os.path.join(libraries_win_root, 'x86/Debug/libxml2.lib'))
    build_utils.run_process(nmake_clean, process_cwd=makefile_path, shell=True, environment=build_utils.get_win32_vs_x86_env())

    # Build and copy x64 debug static lib
    build_utils.run_process(
        ['nmake', '/f', 'Makefile', 'libxmla'],
        process_cwd=makefile_path,
        shell=True,
        environment=build_utils.get_win32_vs_x64_env())
    shutil.copyfile(os.path.join(makefile_path, 'bin.msvc/libxml2_a.lib'),
                    os.path.join(libraries_win_root, 'x64/Debug/libxml2.lib'))
    build_utils.run_process(nmake_clean, process_cwd=makefile_path, shell=True, environment=build_utils.get_win32_vs_x64_env())

    # Generate makefile for release build
    build_utils.run_process(
        cscript_release,
        process_cwd=makefile_path,
        shell=True,
        environment=None)

    # Build and copy x86 release static lib
    build_utils.run_process(
        ['nmake', '/f', 'Makefile', 'libxmla'],
        process_cwd=makefile_path,
        shell=True,
        environment=build_utils.get_win32_vs_x86_env())
    shutil.copyfile(os.path.join(makefile_path, 'bin.msvc/libxml2_a.lib'),
                    os.path.join(libraries_win_root, 'x86/Release/libxml2.lib'))
    build_utils.run_process(nmake_clean, process_cwd=makefile_path, shell=True, environment=build_utils.get_win32_vs_x86_env())

    # Build and copy x64 release static lib
    build_utils.run_process(
        ['nmake', '/f', 'Makefile', 'libxmla'],
        process_cwd=makefile_path,
        shell=True,
        environment=build_utils.get_win32_vs_x64_env())
    shutil.copyfile(os.path.join(makefile_path, 'bin.msvc/libxml2_a.lib'),
                    os.path.join(libraries_win_root, 'x64/Release/libxml2.lib'))
    build_utils.run_process(nmake_clean, process_cwd=makefile_path, shell=True, environment=build_utils.get_win32_vs_x64_env())

    """ Build instructions for libxml2 v2.9.0 and later
    sln_path = os.path.join(source_folder_path, 'win32/VC10/libxml2.sln')
    build_utils.build_vs(sln_path, 'Debug', 'Win32', target='libxml2', toolset=build_config.get_msvc_toolset_ver_win32())
    build_utils.build_vs(sln_path, 'Release', 'Win32', target='libxml2', toolset=build_config.get_msvc_toolset_ver_win32())
    build_utils.build_vs(sln_path, 'Debug', 'x64', target='libxml2', toolset=build_config.get_msvc_toolset_ver_win32())
    build_utils.build_vs(sln_path, 'Release', 'x64', target='libxml2', toolset=build_config.get_msvc_toolset_ver_win32())
    
    libraries_win_root = os.path.join(root_project_path, 'Libs/lib_CMake/win')
    
    lib_path_x86_debug = os.path.join(
        source_folder_path, 'win32/VC10/Debug/libxml2.lib')
    lib_path_x86_release = os.path.join(
        source_folder_path, 'win32/VC10/Release/libxml2.lib')
    shutil.copyfile(
        lib_path_x86_debug,
        os.path.join(libraries_win_root, 'x86/Debug/libxml2.lib'))
    shutil.copyfile(
        lib_path_x86_release,
        os.path.join(libraries_win_root, 'x86/Release/libxml2.lib'))
    
    lib_path_x64_debug = os.path.join(
        source_folder_path, 'win32/VC10/x64/Debug/libxml2.lib')
    lib_path_x64_release = os.path.join(
        source_folder_path, 'win32/VC10/x64/Release/libxml2.lib')
    shutil.copyfile(
        lib_path_x64_debug,
        os.path.join(libraries_win_root, 'x64/Debug/libxml2.lib'))
    shutil.copyfile(
        lib_path_x64_release,
        os.path.join(libraries_win_root, 'x64/Release/libxml2.lib'))
    """

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources_windows(source_folder_path, working_directory_path)

    sln_path = os.path.join(source_folder_path, 'win32/Win10/libxml2.sln')
    build_utils.build_vs(sln_path, 'Debug', 'Win32', target='libxml2')
    build_utils.build_vs(sln_path, 'Release', 'Win32', target='libxml2')
    build_utils.build_vs(sln_path, 'Debug', 'x64', target='libxml2')
    build_utils.build_vs(sln_path, 'Release', 'x64', target='libxml2')
    build_utils.build_vs(sln_path, 'Debug', 'ARM', target='libxml2')
    build_utils.build_vs(sln_path, 'Release', 'ARM', target='libxml2')

    libraries_win10_root = os.path.join(
        root_project_path, 'Libs/lib_CMake/win10')

    lib_path_x86_debug = os.path.join(
        source_folder_path, 'win32/Win10/Debug/libxml2.lib')
    lib_path_x86_release = os.path.join(
        source_folder_path, 'win32/Win10/Release/libxml2.lib')
    shutil.copyfile(
        lib_path_x86_debug,
        os.path.join(libraries_win10_root, 'Win32/Debug/libxml.lib'))
    shutil.copyfile(
        lib_path_x86_release,
        os.path.join(libraries_win10_root, 'Win32/Release/libxml.lib'))

    lib_path_x64_debug = os.path.join(
        source_folder_path, 'win32/Win10/x64/Debug/libxml2.lib')
    lib_path_x64_release = os.path.join(
        source_folder_path, 'win32/Win10/x64/Release/libxml2.lib')
    shutil.copyfile(
        lib_path_x64_debug,
        os.path.join(libraries_win10_root, 'x64/Debug/libxml.lib'))
    shutil.copyfile(
        lib_path_x64_release,
        os.path.join(libraries_win10_root, 'x64/Release/libxml.lib'))

    lib_path_arm_debug = os.path.join(
        source_folder_path, 'win32/Win10/ARM/Debug/libxml2.lib')
    lib_path_arm_release = os.path.join(
        source_folder_path, 'win32/Win10/ARM/Release/libxml2.lib')
    shutil.copyfile(
        lib_path_arm_debug,
        os.path.join(libraries_win10_root, 'arm/Debug/libxml.lib'))
    shutil.copyfile(
        lib_path_arm_release,
        os.path.join(libraries_win10_root, 'arm/Release/libxml.lib'))

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    install_dir_macos = os.path.join(
        working_directory_path, 'gen/install_macos')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=x86_64-apple-darwin', '--disable-shared', '--enable-static', '--without-lzma', '--without-python'],
        install_dir_macos,
        env=build_utils.get_autotools_macos_env())

    lib_path = os.path.join(install_dir_macos, 'lib/libxml2.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/mac/libxml_macos.a'))

    _copy_headers_from_install(install_dir_macos, root_project_path)


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    install_dir_ios = os.path.join(working_directory_path, 'gen/install_ios')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=armv7-apple-darwin', '--disable-shared', '--enable-static', '--without-lzma', '--without-python'],
        install_dir_ios,
        env=build_utils.get_autotools_ios_env())

    lib_path = os.path.join(install_dir_ios, 'lib/libxml2.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/ios/libxml_ios.a'))

    _copy_headers_from_install(install_dir_ios, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources_android(source_folder_path, working_directory_path)

    gen_folder_path = os.path.join(working_directory_path, 'gen')
    if not os.path.exists(gen_folder_path):
        os.makedirs(gen_folder_path)

    # ARM
    toolchain_path_arm = build_utils.android_ndk_get_toolchain_arm()

    arm_env = build_utils.get_autotools_android_arm_env(toolchain_path_arm)
    install_dir_android_arm = os.path.join(working_directory_path, 'gen/install_android_arm')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=arm-linux-androideabi',
         '--disable-shared',
         '--enable-static',
         '--without-lzma',
         '--without-python'],
        install_dir_android_arm,
        env=arm_env)

    # x86
    toolchain_path_x86 = build_utils.android_ndk_get_toolchain_x86()

    x86_env = build_utils.get_autotools_android_x86_env(toolchain_path_x86)
    install_dir_android_x86 = os.path.join(working_directory_path, 'gen/install_android_x86')
    build_utils.build_with_autotools(
        source_folder_path,
        ['--host=i686-linux-android',
         '--disable-shared',
         '--enable-static',
         '--without-lzma',
         '--without-python'],
        install_dir_android_x86,
        env=x86_env)

    libraries_android_root = os.path.join(
        root_project_path, 'Libs/lib_CMake/android')

    lib_path_arm = os.path.join(install_dir_android_arm, 'lib/libxml2.a')
    shutil.copyfile(
        lib_path_arm,
        os.path.join(libraries_android_root, 'armeabi-v7a/libxml.a'))
    lib_path_x86 = os.path.join(install_dir_android_x86, 'lib/libxml2.a')
    shutil.copyfile(
        lib_path_x86,
        os.path.join(libraries_android_root, 'x86/libxml.a'))

    _copy_headers_from_install(install_dir_android_x86, root_project_path)


def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    env = build_utils.get_autotools_linux_env()

    install_dir = os.path.join(working_directory_path, 'gen/install_linux')
    build_utils.build_with_autotools(
        source_folder_path,
        # exclude python bindings as make install requires sudo
        # also exclude lzma support as it requires lzma library
        ['--disable-shared', '--enable-static', '--without-python', '--without-lzma'],
        install_dir,
        env=env) 

    lib_path = os.path.join(install_dir, 'lib/libxml2.a')
    shutil.copyfile(
        lib_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/linux/libxml.a'))

    _copy_headers_from_install(install_dir, root_project_path)


def _copy_headers_from_install(install_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libxml')
    build_utils.copy_folder_recursive(
        os.path.join(install_folder_path, 'include/libxml2/libxml'),
        include_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/libxml')
    build_utils.copy_files(
        os.path.join(source_folder_path, 'include/libxml'),
        include_path,
        '*.h')
