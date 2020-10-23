import os
import shutil
import build_config
import build_utils

libcurl_version = '7.53.1'

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
    if target == 'android':
        return ['openssl']
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


def _download_and_extract(working_directory_path):
    source_folder_path = os.path.join(working_directory_path, 'libcurl_source')

    url = 'https://curl.haxx.se/download/curl-' + libcurl_version + '.tar.gz'
    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        build_utils.get_url_file_name_no_ext(url))

    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, working_directory_path, patch_name):
    # Apply fixes
    build_utils.apply_patch(
        os.path.abspath(patch_name), working_directory_path)


def _build_win32(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    vc14_solution_file_path = os.path.join(source_folder_path, 'projects/Windows/VC14/lib/libcurl.sln')

    override_props_file=os.path.abspath('override_win32.props')
    msbuild_args=["/p:ForceImportBeforeCppTargets={}".format(override_props_file)]
    toolset=build_config.get_msvc_toolset_ver_win32()

    build_utils.build_vs(vc14_solution_file_path, 'LIB Debug', 'Win32', 'libcurl', toolset=toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc14_solution_file_path, 'LIB Release', 'Win32', 'libcurl', toolset=toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc14_solution_file_path, 'LIB Debug', 'x64', 'libcurl', toolset=toolset, msbuild_args=msbuild_args)
    build_utils.build_vs(vc14_solution_file_path, 'LIB Release', 'x64', 'libcurl', toolset=toolset, msbuild_args=msbuild_args)

    libs_win_root = os.path.join(root_project_path, 'Libs/lib_CMake/win')

    shutil.copyfile(
        os.path.join(source_folder_path,'build/Win32/VC14/LIB Debug/libcurld.lib'),
        os.path.join(libs_win_root, 'x86/Debug/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/Win32/VC14/LIB Release/libcurl.lib'),
        os.path.join(libs_win_root, 'x86/Release/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/Win64/VC14/LIB Debug/libcurld.lib'),
        os.path.join(libs_win_root, 'x64/Debug/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/Win64/VC14/LIB Release/libcurl.lib'),
        os.path.join(libs_win_root, 'x64/Release/libcurl.lib'))

    _copy_headers(source_folder_path, root_project_path)


def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path, 'patch_win10.diff')

    vc14_solution_file_path = os.path.join(source_folder_path, 'projects/Windows/VC14/lib/libcurl.sln')

    build_utils.build_vs(vc14_solution_file_path, 'LIB Debug - LIB OpenSSL', 'Win32', 'libcurl')
    build_utils.build_vs(vc14_solution_file_path, 'LIB Release - LIB OpenSSL', 'Win32', 'libcurl')
    build_utils.build_vs(vc14_solution_file_path, 'LIB Debug - LIB OpenSSL', 'x64', 'libcurl')
    build_utils.build_vs(vc14_solution_file_path, 'LIB Release - LIB OpenSSL', 'x64', 'libcurl')
    build_utils.build_vs(vc14_solution_file_path, 'LIB Debug - LIB OpenSSL', 'ARM', 'libcurl')
    build_utils.build_vs(vc14_solution_file_path, 'LIB Release - LIB OpenSSL', 'ARM', 'libcurl')

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/Win32/VC14/LIB Debug - LIB OpenSSL/libcurld.lib'),
        os.path.join(root_project_path, 'Libs/lib_CMake/win10/Win32/Debug/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/Win32/VC14/LIB Release - LIB OpenSSL/libcurl.lib'),
        os.path.join(root_project_path, 'Libs/lib_CMake/win10/Win32/Release/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/Win64/VC14/LIB Debug - LIB OpenSSL/libcurld.lib'),
        os.path.join(root_project_path, 'Libs/lib_CMake/win10/x64/Debug/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/Win64/VC14/LIB Release - LIB OpenSSL/libcurl.lib'),
        os.path.join(root_project_path, 'Libs/lib_CMake/win10/x64/Release/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/ARM/VC14/LIB Debug - LIB OpenSSL/libcurld.lib'),
        os.path.join(root_project_path, 'Libs/lib_CMake/win10/arm/Debug/libcurl.lib'))

    shutil.copyfile(
        os.path.join(source_folder_path, 'build/ARM/VC14/LIB Release - LIB OpenSSL/libcurl.lib'),
        os.path.join(root_project_path, 'Libs/lib_CMake/win10/arm/Release/libcurl.lib'))

    _copy_headers(source_folder_path, root_project_path)


def _build_macos(working_directory_path, root_project_path):
    build_curl_run_dir = os.path.join(working_directory_path, 'gen/build_osx')

    if not os.path.exists(build_curl_run_dir):
        os.makedirs(build_curl_run_dir)

    build_curl_args = [
        './build_curl', 
        '--libcurl-version',
        libcurl_version,
        '--osx-sdk-version',
        '10.12',
        '--arch', 
        'x86_64', 
        '--run-dir', 
        build_curl_run_dir]

    if (build_utils.verbose):
        build_curl_args.append('--verbose')

    build_utils.run_process(
        build_curl_args,
        process_cwd='curl-ios-build-scripts-master')

    # copy libs
    shutil.copyfile(
        os.path.join(build_curl_run_dir, 'curl/osx/lib/libcurl.a'),
        os.path.join(root_project_path, os.path.join('Libs/lib_CMake/mac/libcurl_macos.a')))

    # copy headers from original archive
    source_folder_path = _download_and_extract(working_directory_path)
    _copy_headers(source_folder_path, root_project_path)    


def _build_ios(working_directory_path, root_project_path):
    build_curl_run_dir = os.path.join(working_directory_path, 'gen/build_ios')

    if not os.path.exists(build_curl_run_dir):
        os.makedirs(build_curl_run_dir)

    build_curl_args = [
        './build_curl',
        '--libcurl-version',
        libcurl_version,
        '--sdk-version',
        '10.2',
        '--arch',
        'armv7,armv7s,arm64',
        '--run-dir',
        build_curl_run_dir]

    if (build_utils.verbose):
        build_curl_args.append('--verbose')

    build_utils.run_process(
        build_curl_args, process_cwd='curl-ios-build-scripts-master')

    # copy libs
    shutil.copyfile(
        os.path.join(build_curl_run_dir, 'curl/ios-appstore/lib/libcurl.a'),
        os.path.join(root_project_path, os.path.join('Libs/lib_CMake/ios/libcurl_ios.a')))

    # copy headers from original archive
    source_folder_path = _download_and_extract(working_directory_path)    
    _copy_headers(source_folder_path, root_project_path)


def _build_android(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, working_directory_path, 'patch_android.diff')

    # copy headers
    _copy_headers(source_folder_path, root_project_path)

    env = os.environ.copy()
    original_path_var = env["PATH"]

    # ARM
    toolchain_path_arm = build_utils.android_ndk_get_toolchain_arm()

    env_arm = build_utils.get_autotools_android_arm_env(toolchain_path_arm)
    install_dir_android_arm = os.path.join(working_directory_path, 'gen/install_android_arm')
    configure_args = [
        '--host=arm-linux-androideabi',
        '--enable-threaded-resolver',
        '--enable-ipv6',
        '--disable-shared',
        '--disable-rtsp',
        '--disable-ftp',
        '--disable-file',
        '--disable-ldap',
        '--disable-ldaps',
        '--disable-rtsp',
        '--disable-dict',
        '--disable-telnet',
        '--disable-tftp',
        '--disable-pop3',
        '--disable-imap',
        '--disable-smtp',
        '--disable-gopher',
        '--with-ssl=' + os.path.abspath(os.path.join(working_directory_path, '../openssl/gen/install_android_arm/'))]
    
    build_utils.build_with_autotools(
        source_folder_path,
        configure_args,
        install_dir_android_arm,
        env_arm)
    
    # x86
    toolchain_path_x86 = build_utils.android_ndk_get_toolchain_x86()

    env_x86 = build_utils.get_autotools_android_x86_env(toolchain_path_x86)
    install_dir_android_x86 = os.path.join(working_directory_path, 'gen/install_android_x86')
    configure_args = [
        '--host=i686-linux-android',
        '--enable-threaded-resolver',
        '--enable-ipv6',
        '--disable-shared',
        '--disable-rtsp',
        '--disable-ftp',
        '--disable-file',
        '--disable-ldap',
        '--disable-ldaps',
        '--disable-rtsp',
        '--disable-dict',
        '--disable-telnet',
        '--disable-tftp',
        '--disable-pop3',
        '--disable-imap',
        '--disable-smtp',
        '--disable-gopher',
        '--with-ssl=' + os.path.abspath(os.path.join(working_directory_path,'../openssl/gen/install_android_x86/'))]
    build_utils.build_with_autotools(
        source_folder_path,
        configure_args,
        install_dir_android_x86, env_x86)

    # intermediate libs
    lib_android_arm_itm = os.path.join(install_dir_android_arm, 'lib/libcurl.a')
    lib_android_x86_itm = os.path.join(install_dir_android_x86, 'lib/libcurl.a')

    # ready libs
    libs_android_root = os.path.join(root_project_path, 'Libs/lib_CMake/android')
    lib_android_arm = os.path.join(libs_android_root, 'armeabi-v7a/libcurl.a')
    lib_android_x86 = os.path.join(libs_android_root, 'x86/libcurl.a')

    shutil.copyfile(lib_android_arm_itm, lib_android_arm)
    shutil.copyfile(lib_android_x86_itm, lib_android_x86)


def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)

    env = build_utils.get_autotools_linux_env()
    install_dir = os.path.join(working_directory_path, 'gen/install_linux')
    openssl_install_dir = os.path.abspath(os.path.join(working_directory_path, '../openssl/gen/install_linux/'))
    
    configure_args = [
        '--disable-shared',
        '--enable-threaded-resolver',
        '--enable-ipv6',
        '--disable-rtsp',
        '--disable-ftp',
        '--disable-file',
        '--disable-ldap',
        '--disable-ldaps',
        '--disable-rtsp',
        '--disable-dict',
        '--disable-telnet',
        '--disable-tftp',
        '--disable-pop3',
        '--disable-imap',
        '--disable-smtp',
        '--disable-gopher',		
        '--with-ssl=' + openssl_install_dir]
    build_utils.build_with_autotools(
        source_folder_path,
        configure_args,
        install_dir,
        env)

    shutil.copyfile(os.path.join(install_dir, 'lib/libcurl.a'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/linux/libcurl.a'))

    _copy_headers(source_folder_path, root_project_path)


def _copy_headers(source_folder_path, root_project_path):
    include_path = os.path.join(root_project_path, 'Libs/include/curl')
    build_utils.copy_files(os.path.join(source_folder_path, 'include/curl'), include_path, '*.h')
