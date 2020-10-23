# Windows builds require perl to be in PATH


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
    # Win32 and Win10 uses different sources - maintained by Microsoft
    return {'win32': 'https://github.com/Microsoft/openssl/archive/OpenSSL_1_0_2k.tar.gz',
            'win10': 'https://github.com/Microsoft/openssl/archive/OpenSSL_1_0_2k_WinRT.tar.gz',
            'others': 'https://www.openssl.org/source/openssl-1.1.0e.tar.gz'}


def _download_and_extract(working_directory_path, target=''):
    if target == 'win32_x86':
        source_folder_path = os.path.join(working_directory_path, 'openssl_source_win32_x86')
        url = get_download_info()['win32']
        inner_dir = 'openssl-' + build_utils.get_url_file_name_no_ext(url)
    elif target == 'win32_x64':
        source_folder_path = os.path.join(working_directory_path, 'openssl_source_win32_x64')
        url = get_download_info()['win32']
        inner_dir = 'openssl-' + build_utils.get_url_file_name_no_ext(url)
    elif target == 'win10':
        source_folder_path = os.path.join(
            working_directory_path, 'openssl_source_win10')
        url = get_download_info()['win10']
        inner_dir = 'openssl-' + build_utils.get_url_file_name_no_ext(url)
    else:
        source_folder_path = os.path.join(
            working_directory_path, 'openssl_source')
        url = get_download_info()['others']
        inner_dir = build_utils.get_url_file_name_no_ext(url)

    build_utils.download_and_extract(
        url,
        working_directory_path,
        source_folder_path,
        inner_dir_name=inner_dir)

    return source_folder_path


@build_utils.run_once
def _patch_sources(source_folder_path, patch_name):
    build_utils.apply_patch(
        os.path.abspath(patch_name), source_folder_path)


_configure_args = [
    'no-whirlpool',
    'no-asm',
    'no-cast',
    'no-idea',
    'no-camellia',
    'no-comp',
    'no-hw',
    'no-engine',
    'no-dso']

_configure_args_win32 = "" # all configure args are in patch_win32.diff

def _build_win32(working_directory_path, root_project_path):

    libraries_win_root = os.path.join(root_project_path, 'Libs/lib_CMake/win')

    # x86
    source_folder_path_x86 = _download_and_extract(working_directory_path, 'win32_x86')
    shutil.copyfile('build-msvc.cmd',
                    os.path.join(source_folder_path_x86, 'build-msvc.cmd'))

    build_utils.run_process(
        'build-msvc.cmd',
        process_cwd=source_folder_path_x86,
        shell=True,
        environment=build_utils.get_win32_vs_x86_env())

    libeay_path_x86_debug = os.path.join(source_folder_path_x86, 'out32.dbg/libeay32.lib')
    ssleay_path_x86_debug = os.path.join(source_folder_path_x86, 'out32.dbg/ssleay32.lib')
    libeay_path_x86_release = os.path.join(source_folder_path_x86, 'out32/libeay32.lib')
    ssleay_path_x86_release = os.path.join(source_folder_path_x86, 'out32/ssleay32.lib')

    shutil.copyfile(libeay_path_x86_debug, os.path.join(libraries_win_root, 'x86/Debug/libeay32.lib'))
    shutil.copyfile(ssleay_path_x86_debug, os.path.join(libraries_win_root, 'x86/Debug/ssleay32.lib'))
    shutil.copyfile(libeay_path_x86_release, os.path.join(libraries_win_root, 'x86/Release/libeay32.lib'))
    shutil.copyfile(ssleay_path_x86_release, os.path.join(libraries_win_root, 'x86/Release/ssleay32.lib'))

    #x64
    source_folder_path_x64 = _download_and_extract(working_directory_path, 'win32_x64')
    shutil.copyfile('build-msvc.cmd',
                    os.path.join(source_folder_path_x64, 'build-msvc.cmd'))

    build_utils.run_process(
        ['build-msvc.cmd', 'x64'],
        process_cwd=source_folder_path_x64,
        shell=True,
        environment=build_utils.get_win32_vs_x64_env())

    libeay_path_x64_debug = os.path.join(source_folder_path_x64, 'out32.dbg/libeay32.lib')
    ssleay_path_x64_debug = os.path.join(source_folder_path_x64, 'out32.dbg/ssleay32.lib')
    libeay_path_x64_release = os.path.join(source_folder_path_x64, 'out32/libeay32.lib')
    ssleay_path_x64_release = os.path.join(source_folder_path_x64, 'out32/ssleay32.lib')

    shutil.copyfile(libeay_path_x64_debug, os.path.join(libraries_win_root, 'x64/Debug/libeay32.lib'))
    shutil.copyfile(ssleay_path_x64_debug, os.path.join(libraries_win_root, 'x64/Debug/ssleay32.lib'))
    shutil.copyfile(libeay_path_x64_release, os.path.join(libraries_win_root, 'x64/Release/libeay32.lib'))
    shutil.copyfile(ssleay_path_x64_release, os.path.join(libraries_win_root, 'x64/Release/ssleay32.lib'))

    # headers 
    headers_x86_src = os.path.join(source_folder_path_x86, 'inc32/openssl')
    headers_x64_src = os.path.join(source_folder_path_x64, 'inc32/openssl')
    headers_x86_dst = _headers_include_path(root_project_path, 'win32/x86')
    headers_x64_dst = _headers_include_path(root_project_path, 'win32/x64')
    build_utils.copy_folder_recursive(headers_x86_src, headers_x86_dst)
    build_utils.copy_folder_recursive(headers_x64_src, headers_x64_dst)

def _build_win10(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path, 'win10')

    build_utils.run_process(
        ['ms\\do_vsprojects14.bat'],
        process_cwd=source_folder_path,
        shell=True)

    vs_solution_path = 'vsout/NT-Universal-10.0-Static-Unicode/NT-Universal-10.0-Static-Unicode.vcxproj'

    build_utils.build_vs(
        os.path.join(source_folder_path, vs_solution_path), 'Debug', 'Win32')
    build_utils.build_vs(
        os.path.join(source_folder_path, vs_solution_path), 'Release', 'Win32')
    build_utils.build_vs(
        os.path.join(source_folder_path, vs_solution_path), 'Debug', 'x64')
    build_utils.build_vs(
        os.path.join(source_folder_path, vs_solution_path), 'Release', 'x64')
    build_utils.build_vs(
        os.path.join(source_folder_path, vs_solution_path), 'Debug', 'ARM')
    build_utils.build_vs(
        os.path.join(source_folder_path, vs_solution_path), 'Release', 'ARM')

    build_utils.run_process(
        ['ms\\do_packwinuniversal.bat'],
        process_cwd=source_folder_path,
        shell=True)

    package_folder_path = os.path.join(source_folder_path, 'vsout/package')
    libs_folder_path = os.path.join(
        package_folder_path, 'lib/Universal/10.0/Static/Unicode')

    libraries_win10_root = os.path.join(root_project_path, 'Libs/lib_CMake/win10')

    libssl_path_x86_debug = os.path.join(
        libs_folder_path, 'Debug/Win32/ssleay32.lib')
    libcrypto_path_x86_debug = os.path.join(
        libs_folder_path, 'Debug/Win32/libeay32.lib')
    shutil.copyfile(
        libssl_path_x86_debug,
        os.path.join(libraries_win10_root, 'Win32/Debug/ssleay32.lib'))
    shutil.copyfile(
        libcrypto_path_x86_debug,
        os.path.join(libraries_win10_root, 'Win32/Debug/libeay32.lib'))

    libssl_path_x86_release = os.path.join(
        libs_folder_path, 'Release/Win32/ssleay32.lib')
    libcrypto_path_x86_release = os.path.join(
        libs_folder_path, 'Release/Win32/libeay32.lib')
    shutil.copyfile(
        libssl_path_x86_release,
        os.path.join(libraries_win10_root, 'Win32/Release/ssleay32.lib'))
    shutil.copyfile(
        libcrypto_path_x86_release,
        os.path.join(libraries_win10_root, 'Win32/Release/libeay32.lib'))

    libssl_path_x64_debug = os.path.join(
        libs_folder_path, 'Debug/x64/ssleay32.lib')
    libcrypto_path_x64_debug = os.path.join(
        libs_folder_path, 'Debug/x64/libeay32.lib')
    shutil.copyfile(
        libssl_path_x64_debug,
        os.path.join(libraries_win10_root, 'x64/Debug/ssleay32.lib'))
    shutil.copyfile(
        libcrypto_path_x64_debug,
        os.path.join(libraries_win10_root, 'x64/Debug/libeay32.lib'))

    libssl_path_x64_release = os.path.join(
        libs_folder_path, 'Release/x64/ssleay32.lib')
    libcrypto_path_x64_release = os.path.join(
        libs_folder_path, 'Release/x64/libeay32.lib')
    shutil.copyfile(
        libssl_path_x64_release,
        os.path.join(libraries_win10_root, 'x64/Release/ssleay32.lib'))
    shutil.copyfile(
        libcrypto_path_x64_release,
        os.path.join(libraries_win10_root, 'x64/Release/libeay32.lib'))

    libssl_path_arm_debug = os.path.join(
        libs_folder_path, 'Debug/arm/ssleay32.lib')
    libcrypto_path_arm_debug = os.path.join(
        libs_folder_path, 'Debug/arm/libeay32.lib')
    shutil.copyfile(
        libssl_path_arm_debug,
        os.path.join(libraries_win10_root, 'arm/Debug/ssleay32.lib'))
    shutil.copyfile(
        libcrypto_path_arm_debug,
        os.path.join(libraries_win10_root, 'arm/Debug/libeay32.lib'))

    libssl_path_arm_release = os.path.join(
        libs_folder_path, 'Release/arm/ssleay32.lib')
    libcrypto_path_arm_release = os.path.join(
        libs_folder_path, 'Release/arm/libeay32.lib')
    shutil.copyfile(
        libssl_path_arm_release,
        os.path.join(libraries_win10_root, 'arm/Release/ssleay32.lib'))
    shutil.copyfile(
        libcrypto_path_arm_release,
        os.path.join(libraries_win10_root, 'arm/Release/libeay32.lib'))

    _copy_headers(package_folder_path, root_project_path, 'uwp')


def _build_macos(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, 'patch.diff')

    install_dir = os.path.join(working_directory_path, 'gen/install_macos')
    macos_configure_args = list(_configure_args)
    macos_configure_args.insert(0, 'darwin64-x86_64-cc')
    build_utils.build_with_autotools(
        source_folder_path,
        macos_configure_args,
        install_dir,
        configure_exec_name='Configure',
        shell_prefix='perl')

    libssl_path = os.path.join(install_dir, 'lib/libssl.a')
    libcrypto_path = os.path.join(install_dir, 'lib/libcrypto.a')
    shutil.copyfile(
        libssl_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/mac/libssl.a'))
    shutil.copyfile(
        libcrypto_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/mac/libcrypto.a'))

    _copy_headers(install_dir, root_project_path, 'mac')


def _build_ios(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, 'patch.diff')

    install_dir_armv7 = os.path.join(
        working_directory_path, 'gen/install_ios_armv7')
    ios_configure_args = list(_configure_args)
    ios_configure_args.insert(0, 'ios-cross')
    build_utils.build_with_autotools(
        source_folder_path,
        ios_configure_args,
        install_dir_armv7,
        configure_exec_name='Configure',
        shell_prefix='perl',
        env=_get_ios_env())

    install_dir_arm64 = os.path.join(
        working_directory_path, 'gen/install_ios_arm64')
    ios_configure_args = list(_configure_args)
    ios_configure_args.insert(0, 'ios64-cross')
    build_utils.build_with_autotools(
        source_folder_path,
        ios_configure_args,
        install_dir_arm64,
        configure_exec_name='Configure',
        shell_prefix='perl',
        env=_get_ios_env())

    libssl_fat_path = os.path.join(
        working_directory_path, 'gen/fat_ios/libssl.a')
    libcrypto_fat_path = os.path.join(
        working_directory_path, 'gen/fat_ios/libcrypto.a')
    build_utils.make_fat_darwin_binary(
        [os.path.join(install_dir_armv7, 'lib/libssl.a'),
         os.path.join(install_dir_arm64, 'lib/libssl.a')],
        libssl_fat_path)
    build_utils.make_fat_darwin_binary(
        [os.path.join(install_dir_armv7, 'lib/libcrypto.a'),
         os.path.join(install_dir_arm64, 'lib/libcrypto.a')],
        libcrypto_fat_path)

    shutil.copyfile(
        libssl_fat_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/ios/libssl.a'))
    shutil.copyfile(
        libcrypto_fat_path,
        os.path.join(root_project_path, 'Libs/lib_CMake/ios/libcrypto.a'))

    _copy_headers(install_dir_armv7, root_project_path, 'ios')


def _build_android(working_directory_path, root_project_path):
    # https://wiki.openssl.org/index.php/Android

    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, 'patch.diff')

    install_dir_arm = os.path.join(
        working_directory_path, 'gen/install_android_arm')
    build_utils.build_with_autotools(
        source_folder_path,
        _configure_args,
        install_dir_arm,
        configure_exec_name='config',
        shell_prefix='perl',
        env=_get_android_env_arm(source_folder_path, root_project_path))

    install_dir_x86 = os.path.join(
        working_directory_path, 'gen/install_android_x86')
    build_utils.build_with_autotools(
        source_folder_path,
        _configure_args,
        install_dir_x86,
        configure_exec_name='config',
        shell_prefix='perl',
        env=_get_android_env_x86(source_folder_path, root_project_path))

    libssl_path_android_arm = os.path.join(
        install_dir_arm, 'lib/libssl.a')
    libcrypto_path_android_arm = os.path.join(
        install_dir_arm, 'lib/libcrypto.a')
    libssl_path_android_x86 = os.path.join(
        install_dir_x86, 'lib/libssl.a')
    libcrypto_path_android_x86 = os.path.join(
        install_dir_x86, 'lib/libcrypto.a')

    librariess_android_root = os.path.join(
        root_project_path, 'Libs/lib_CMake/android')

    shutil.copyfile(
        libssl_path_android_arm,
        os.path.join(librariess_android_root, 'armeabi-v7a/libssl.a'))
    shutil.copyfile(
        libcrypto_path_android_arm,
        os.path.join(librariess_android_root, 'armeabi-v7a/libcrypto.a'))
    shutil.copyfile(
        libssl_path_android_x86,
        os.path.join(librariess_android_root, 'x86/libssl.a'))
    shutil.copyfile(
        libcrypto_path_android_x86,
        os.path.join(librariess_android_root, 'x86/libcrypto.a'))

    _copy_headers(install_dir_arm, root_project_path, 'android')


def _build_linux(working_directory_path, root_project_path):
    source_folder_path = _download_and_extract(working_directory_path)
    _patch_sources(source_folder_path, 'patch.diff')

    env = build_utils.get_autotools_linux_env()
    install_dir = os.path.join(working_directory_path, 'gen/install_linux')

    configure_args=list(_configure_args)
    configure_args.append('linux-x86_64')

    build_utils.build_with_autotools(
        source_folder_path,
        configure_args,
        install_dir,
        env=env,
        configure_exec_name='Configure',
        shell_prefix='perl',
        make_targets=['depend', 'all', 'install'],
        postclean=False)

    shutil.copyfile(os.path.join(install_dir, 'lib/libssl.a'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/linux/libssl.a'))
    shutil.copyfile(os.path.join(install_dir, 'lib/libcrypto.a'),
                    os.path.join(root_project_path, 'Libs/lib_CMake/linux/libcrypto.a'))

    _copy_headers(install_dir, root_project_path, 'linux')


def _get_ios_env():
    xcode_developer_path = build_utils.get_xcode_developer_path()

    env = os.environ.copy()
    env['CROSS_COMPILE'] = os.path.join(
        xcode_developer_path, 'Toolchains/XcodeDefault.xctoolchain/usr/bin/')
    env['CROSS_TOP'] = os.path.join(
        xcode_developer_path, 'Platforms/iPhoneOS.platform/Developer/')
    env['CROSS_SDK'] = 'iPhoneOS.sdk'

    return env


def _get_android_env(
        source_folder_path,
        root_project_path,
        android_target,
        machine,
        arch,
        toolchain_folder,
        cross_compile,
        crystax_libs_folder):
    # Python version of setenv.sh
    # (from https://wiki.openssl.org/index.php/Android)

    android_ndk_root = build_utils.get_android_ndk_path()
    platform_path = '{}/platforms/{}/arch-{}'.format(
        android_ndk_root, android_target, arch)
    eabi_path = '{}/toolchains/{}/prebuilt/darwin-x86_64/bin'.format(
        android_ndk_root, toolchain_folder)
    fips_sig_path = '{}/util/incore'.format(source_folder_path)

    env = os.environ.copy()
    env['SYSTEM'] = 'android'
    env['MACHINE'] = machine
    env['ARCH'] = arch
    env['CROSS_SYSROOT'] = platform_path
    env['CROSS_COMPILE'] = cross_compile
    env['PATH'] = '{}:{}'.format(eabi_path, env['PATH'])
    env['FIPS_SIG'] = fips_sig_path

    return env


def _get_android_env_arm(source_folder_path, root_project_path):
    return _get_android_env(
        source_folder_path,
        root_project_path,
        build_config.get_android_platform(),
        'armv7',
        'arm',
        'arm-linux-androideabi-4.9',
        'arm-linux-androideabi-',
        'armeabi-v7a')


def _get_android_env_x86(source_folder_path, root_project_path):
    return _get_android_env(
        source_folder_path,
        root_project_path,
        build_config.get_android_platform(),
        'i686',
        'x86',
        'x86-4.9',
        'i686-linux-android-',
        'x86')

def _headers_include_path(root_project_path, target_folder):
    include_subpath = os.path.join(
        os.path.join('Libs/openssl/include', target_folder), 'openssl')
    return os.path.join(root_project_path, include_subpath)

def _copy_headers(install_path, root_project_path, target_folder):
    include_path = _headers_include_path(root_project_path, target_folder)
    build_utils.copy_files(
        os.path.join(install_path, 'include/openssl'), include_path, '*.h')
