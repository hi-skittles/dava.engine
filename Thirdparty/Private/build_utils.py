import urllib2
import zipfile
import os
import subprocess
import glob
import shutil
import tarfile
import sys
import build_config

# These values should be set by root build.py before using this module
verbose = False
output_folder_path = ''
dava_folder_path = ''
suppress_build_warnings=False   # Do not output compiler warning messages while building libraries
                                # (currently supported only for Visual Studio projects)


def print_verbose(msg):
    if verbose:
        print msg.rstrip()


def download(url, file_name):
    path = os.path.dirname(file_name)
    if not os.path.exists(path):
        os.makedirs(path)

    u = urllib2.urlopen(url)
    f = open(file_name, 'wb')
    meta = u.info()

    content_length = meta.getheaders("Content-Length")

    if content_length:
        file_size = int(content_length[0])
    else:
        file_size = 0

    print "Downloading %s (%s bytes) from %s ..." % (file_name, file_size, url)

    file_size_dl = 0
    block_sz = 65536
    while True:
        buffer = u.read(block_sz)
        if not buffer:
            break

        file_size_dl += len(buffer)
        f.write(buffer)

        if file_size > 0:
            status = r"%10d  [%3.2f%%]" % (file_size_dl, file_size_dl * 100. / file_size)
            status = status + chr(8)*(len(status)+1)
            print status,

    f.close()


def download_if_doesnt_exist(url, file_name):
    if not os.path.exists(file_name):
        download(url, file_name)


def unzip_inplace(path):
    print "Unarchiving %s ..." % (path)

    extension = os.path.splitext(path)[1]
    if extension == '.zip':
        ref = zipfile.ZipFile(path, 'r')
    elif extension == '.gz' or extension == '.tgz' or extension=='.bz2' or extension=='.tar':
        ref = tarfile.open(path, 'r')
    ref.extractall(os.path.dirname(path))
    ref.close()


def apply_patch(patch, working_dir = '.'):
    print "Applying patch %s" % patch
    proc = subprocess.Popen(["git", "apply", "--ignore-whitespace", patch], stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd = working_dir)
    for line in proc.stdout:
        print_verbose(line)
    proc.wait()
    if proc.returncode != 0:
        raise RuntimeError('Failed to apply patch with return code %s' % proc.returncode)


def build_vs(project, configuration, platform='Win32', target = None, toolset = None, env=None, msbuild_args=None):
    print "Building %s for %s (%s) ..." % (project, configuration, platform)
    args = [build_config.get_msbuild_path_win32(), project, "/m", "/p:Configuration="+configuration, '/p:Platform=' + platform]
    if suppress_build_warnings:
        args.append('/consoleloggerparameters:ErrorsOnly')

    if not toolset is None:
        args.append('/p:PlatformToolset=' + toolset)
    if (not target is None):
        args.append('/target:' + target)
    if (not env is None):
        args.append('/p:useenv=true')

    if msbuild_args is not None:
        args.extend(msbuild_args)

    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    for line in proc.stdout:
        print_verbose(line)
    proc.wait()

    if proc.returncode != 0:
        raise RuntimeError('msbuild failed with return code %s' % proc.returncode)


def build_xcode_target(project, target, configuration, xcodebuild_args):
    print "Building %s for %s ..." % (project, configuration)
    args=["xcodebuild", "-project", project, "-target", target, "-configuration", configuration, "build"]
    if xcodebuild_args is not None:
        args.extend(xcodebuild_args)
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for line in proc.stdout:
        print_verbose(line)
    proc.wait()
    if proc.returncode != 0:
        print "Failed with return code %s" % proc.returncode
        raise


def build_xcode_alltargets(project, configuration, xcodebuild_args):
    print "Building %s for %s ..." % (project, configuration)
    args=["xcodebuild", "-project", project, "-alltargets", "-configuration", configuration, "build"]
    if xcodebuild_args is not None:
        args.extend(xcodebuild_args)
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for line in proc.stdout:
        print_verbose(line)
    proc.wait()
    if proc.returncode != 0:
        print "Failed with return code %s" % proc.returncode
        raise

def build_make_target(output_folder_path, target):
    print "Building target %s in %s ..." % (target, output_folder_path)
    # make <target> -C <output_folder_path>
    proc = subprocess.Popen(["make", target, '-C', output_folder_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for line in proc.stdout:
        print_verbose(line)
    proc.wait()
    if proc.returncode != 0:
        print "Failed with return code %s" % proc.returncode
        raise

def copy_files(from_dir, to_dir, wildcard):
    print "Copying %s from %s to %s" % (wildcard, from_dir, to_dir)

    # First create destination dir if it does not exist, else
    # shutil.copy will copy all files into one with name 'to_dir'
    if not os.path.exists(to_dir):
        os.makedirs(to_dir)

    for file in glob.glob(from_dir+"/"+wildcard):
        shutil.copy(file, to_dir)

def copy_files_by_name(from_dir, to_dir, filenames):
    # First create destination dir if it does not exist, else
    # shutil.copy will copy all files into one with name 'to_dir'
    if not os.path.exists(to_dir):
        os.makedirs(to_dir)

    for file in filenames:
        print "Copying %s from %s to %s" % (file, from_dir, to_dir)

        src = os.path.join(from_dir, file)
        dst = os.path.join(to_dir, file)
        shutil.copy(src, dst)


def clean_copy_includes(from_dir, to_dir):
    print "Copying includes from %s to %s" % (from_dir, to_dir)
    if os.path.exists(to_dir) and os.path.isdir(to_dir):
        clear_files(to_dir, '*.h')
    copy_files(from_dir, to_dir, '*.h')


def clear_files(dir, wildcard):
    print "Deleting %s in %s" % (wildcard, dir)
    map(os.remove, glob.glob(wildcard))


def copy_folder_recursive(src, dest, ignore=None):
    if os.path.isdir(src):
        if not os.path.isdir(dest):
            os.makedirs(dest)
        files = os.listdir(src)
        if ignore is not None:
            ignored = ignore(src, files)
        else:
            ignored = set()
        for f in files:
            if f not in ignored:
                copy_folder_recursive(os.path.join(src, f),
                                    os.path.join(dest, f),
                                    ignore)
    else:
        shutil.copyfile(src, dest)


def cmake_build(solution_folder_path, configuration):
    run_process(build_config.get_cmake_executable() + " --build . --config " + configuration, process_cwd=solution_folder_path, shell=True)


def cmake_generate(output_folder_path, src_folder_path, cmake_generator, cmake_additional_args = []):
    if not os.path.exists(output_folder_path):
        os.makedirs(output_folder_path)

    cmd = [build_config.get_cmake_executable(), '-G', cmake_generator, src_folder_path]
    cmd.extend(cmake_additional_args)

    print 'Running CMake: {}, working directory: {}'.format(' '.join(cmd), output_folder_path)

    sp = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=output_folder_path)
    for line in sp.stdout:
        print_verbose(line)
    sp.wait()


def cmake_generate_build_vs(output_folder_path, src_folder_path, cmake_generator, sln_name, target, configuration, cmake_additional_args = [], msbuild_args=None):
    cmake_generate(output_folder_path, src_folder_path, cmake_generator, cmake_additional_args)
    build_vs(os.path.join(output_folder_path, sln_name), 'Debug', configuration, target, msbuild_args=msbuild_args)
    build_vs(os.path.join(output_folder_path, sln_name), 'Release', configuration, target, msbuild_args=msbuild_args)


def cmake_generate_build_xcode(output_folder_path, src_folder_path, cmake_generator, project, target, cmake_additional_args = [], xcodebuild_args=None):
    cmake_generate(output_folder_path, src_folder_path, cmake_generator, cmake_additional_args)
    build_xcode_target(os.path.join(output_folder_path, project), target, 'Release', xcodebuild_args)


def cmake_generate_build_make(output_folder_path, src_folder_path, cmake_generator, target, cmake_additional_args = []):
    cmake_generate(output_folder_path, src_folder_path, cmake_generator, cmake_additional_args)
    build_make_target(output_folder_path, target)


def cmake_generate_build_ndk(output_folder_path, src_folder_path, android_ndk_path, abi, cmake_additional_args = []):
    if not os.path.exists(output_folder_path):
        os.makedirs(output_folder_path)

    cmake_path = os.path.join(android_ndk_path, '../cmake/3.6.3155560/bin/cmake')
    cmake_toolchain = os.path.join(android_ndk_path, 'build/cmake/android.toolchain.cmake')

    cmd = [cmake_path,
        '-DANDROID_PLATFORM=' + build_config.get_android_platform(),
        '-DANDROID_STL=' + build_config.get_android_stl(),
        '-DANDROID_ARM_NEON=TRUE',
        '-DANDROID_ABI=' + abi,
        '-GAndroid Gradle - Unix Makefiles',
        '-DCMAKE_TOOLCHAIN_FILE=' + cmake_toolchain,
        '-DCMAKE_BUILD_TYPE=Release']

    if (sys.platform == 'win32'):
        cmd.extend(['-G', 'MinGW Makefiles', '-DCMAKE_MAKE_PROGRAM=' + os.path.join(android_ndk_path, 'prebuilt\\windows-x86_64\\bin\\make.exe')])
    cmd.extend(cmake_additional_args)
    cmd.append(src_folder_path)

    sp = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=output_folder_path)
    for line in sp.stdout:
        print_verbose(line)
    sp.wait()

    sp = subprocess.Popen([cmake_path, '--build', '.'], stdout=subprocess.PIPE, cwd=output_folder_path)
    for line in sp.stdout:
        print_verbose(line)
    sp.wait()


def build_android_ndk(project_path, output_path, debug, ndk_additional_args = []):
    cmd = ['ndk-build', 'NDK_OUT=' + output_path]
    cmd.extend(ndk_additional_args)

    if debug:
        cmd.append('NDK_DEBUG=1')

    print 'Running ndk-build: {}, working directory: {}'.format(' '.join(cmd), project_path)

    if sys.platform == 'win32':
        sp = subprocess.Popen(cmd, stdout=subprocess.PIPE, cwd=project_path, shell=True)
    else:
        # To avoid swallowing arguments by shell on macOS
        sp = subprocess.Popen(' '.join(cmd), stdout=subprocess.PIPE, cwd=project_path, shell=True)

    for line in sp.stdout:
        print_verbose(line)
    sp.wait()


def get_android_ndk_path():
    config_file_path = os.path.join(dava_folder_path, 'DavaConfig.in')
    for line in open(config_file_path):
        splitted = line.strip().split('=')
        key = splitted[0].strip()
        if key == 'ANDROID_NDK':
            return splitted[1].strip()

    return None


def get_url_file_name(url):
    return url.split('/')[-1]


def get_url_file_name_no_ext(url):
    file_name = get_url_file_name(url)
    parts = file_name.split('.')
    # Handle special case for .tar.gz
    # TODO: a better way?
    ext = parts[-1]
    if ext == 'gz' or ext == 'bz2':
        last_index = -2
    else:
        last_index = -1
    return '.'.join(parts[:last_index])


def download_and_extract(download_url, working_directory_path, result_folder_path, inner_dir_name = None):
    download_data = (download_url, result_folder_path)

    try:
        if download_data in download_and_extract.cache:
            return result_folder_path
    except AttributeError:
        download_and_extract.cache = []

    # Download otherwise

    # Path to downloaded archive
    sources_filename = get_url_file_name(download_url)
    source_archive_filepath = os.path.join(working_directory_path, sources_filename)

    # Download & extract
    download(download_url, source_archive_filepath)
    unzip_inplace(source_archive_filepath)

    # Rename version-dependent folder name to simpler one
    # In case other builder will need to use this folder
    if inner_dir_name is not None:
        extracted_path = os.path.join(working_directory_path, inner_dir_name)
        if not os.path.exists(result_folder_path):
            shutil.move(extracted_path, result_folder_path)
        else:
            def deepmove(src, dst):
                if not os.path.exists(dst):
                    os.mkdir(dst)
                for item in os.listdir(src):
                    s = os.path.join(src, item)
                    d = os.path.join(dst, item)
                    if os.path.isdir(s):
                        deepmove(s, d)
                    else:
                        shutil.move(s, d)
                os.rmdir(src)
            deepmove(extracted_path, result_folder_path)

    download_and_extract.cache.append(download_data)


def run_process(args, process_cwd='.', environment=None, shell=False):
    print 'running process: ' + ' '.join(args)
    for output_line in _run_process_iter(args, process_cwd, environment, shell):
        print_verbose(output_line)


def _run_process_iter(args, process_cwd='.', environment=None, shell=False):
    if environment is None:
        sp = subprocess.Popen(args, shell=shell, stdout=subprocess.PIPE, cwd=process_cwd)
    else:
        sp = subprocess.Popen(args, shell=shell, stdout=subprocess.PIPE, cwd=process_cwd, env=environment)

    stdout_lines = iter(sp.stdout.readline, '')
    for stdout_line in stdout_lines:
        yield stdout_line

    sp.stdout.close()
    return_code = sp.wait()

    if return_code != 0:
        raise subprocess.CalledProcessError(return_code, args)


def android_ndk_get_toolchain_arm():
    install_dir = os.path.join(os.path.join(output_folder_path, 'common'), 'android_ndk_toolchain_arm')

    if 'installed' not in android_ndk_get_toolchain_arm.__dict__:
        android_ndk_get_toolchain_arm.installed = False

    if not android_ndk_get_toolchain_arm.installed:
        _android_ndk_make_toolchain('arm', install_dir)
        android_ndk_get_toolchain_arm.installed = True

    return install_dir


def android_ndk_get_toolchain_x86():
    install_dir = os.path.join(os.path.join(output_folder_path, 'common'), 'android_ndk_toolchain_x86')

    if 'installed' not in android_ndk_get_toolchain_x86.__dict__:
        android_ndk_get_toolchain_x86.installed = False

    if not android_ndk_get_toolchain_x86.installed:
        _android_ndk_make_toolchain('x86', install_dir)
        android_ndk_get_toolchain_x86.installed = True

    return install_dir


def _android_ndk_make_toolchain(arch, install_dir):
    android_ndk_root = get_android_ndk_path()

    exec_path = os.path.join(android_ndk_root, 'build/tools')

    cmd = ['python', 'make_standalone_toolchain.py', '--force', '--arch=' + arch, '--api=' + build_config.get_android_api_version(), '--stl=' + build_config.get_android_libc(), '--install-dir=' + install_dir]
    run_process(cmd, process_cwd=exec_path)


def get_xcode_developer_path():
    sp = subprocess.Popen(['xcode-select', '-print-path'], stdout=subprocess.PIPE)
    stdout, stderr = sp.communicate()
    if stderr is None:
        return stdout.strip()
    else:
        print 'Error while getting xcode developer path: ' + stderr
        return None


def make_fat_darwin_binary(input_files_pathes, output_file_path):
    output_directory_path = os.path.dirname(output_file_path)
    if not os.path.exists(output_directory_path):
        os.makedirs(output_directory_path)

    args = ['lipo', '-output', output_file_path, '-create']
    args.extend(input_files_pathes)
    run_process(args)


def get_autotools_macos_env():
    xcode_developer_path = get_xcode_developer_path()
    cc_path = os.path.join(xcode_developer_path, 'Toolchains/XcodeDefault.xctoolchain/usr/bin/clang')
    cxx_path = os.path.join(xcode_developer_path, 'Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++')
    sysroot_path = os.path.join(xcode_developer_path, 'Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk')

    env = os.environ.copy()
    env['CC'] = cc_path
    env['CXX'] = cxx_path
    env['CFLAGS'] = '-arch x86_64 -arch i386 -pipe -O2 -isysroot {}'.format(sysroot_path)
    env['CXXFLAGS'] = env['CFLAGS']
    env['LDFLAGS'] = '-arch x86_64 -arch i386 -isysroot {}'.format(sysroot_path)
    env['CPP'] = '{} -E'.format(env['CC'])
    env['CXXCPP'] = '{} -E'.format(env['CXX'])

    return env


def get_autotools_ios_env():
    xcode_developer_path = get_xcode_developer_path()
    cc_path = os.path.join(xcode_developer_path, 'Toolchains/XcodeDefault.xctoolchain/usr/bin/clang')
    cxx_path = os.path.join(xcode_developer_path, 'Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++')
    sysroot_path = os.path.join(xcode_developer_path, 'Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk')

    env = os.environ.copy()
    env['CC'] = cc_path
    env['CXX'] = cxx_path
    env['CFLAGS'] = '-arch armv7 -arch armv7s -arch arm64 -pipe -O2 -mios-version-min=6.0 -isysroot {}'.format(sysroot_path)
    env['CXXFLAGS'] = env['CFLAGS']
    env['LDFLAGS'] = '-arch armv7 -arch armv7s -arch arm64 -isysroot {}'.format(sysroot_path)
    env['CPP'] = '{} -E'.format(env['CC'])
    env['CXXCPP'] = '{} -E'.format(env['CXX'])

    return env

def get_autotools_linux_env():
    env = os.environ.copy()
    env['CC'] = '/usr/local/bin/clang'
    env['CXX'] = '/usr/local/bin/clang++'
    return env

def get_autotools_android_arm_env(toolchain_path, enable_stl=False):
    cross_path = os.path.join(toolchain_path, 'bin/arm-linux-androideabi')
    sysroot_path = os.path.join(toolchain_path, 'sysroot')

    env = os.environ.copy()
    env['CPP'] = '{}-cpp'.format(cross_path)
    env['AR'] = '{}-ar'.format(cross_path)
    env['AS'] = '{}-as'.format(cross_path)
    env['NM'] = '{}-nm'.format(cross_path)
    env['CC'] = '{}-clang'.format(cross_path)
    env['CXX'] = '{}-clang++'.format(cross_path)
    env['LD'] = '{}-ld'.format(cross_path)
    env['RANLIB'] = '{}-ranlib'.format(cross_path)
    env['CFLAGS'] = '--sysroot={} -I{}/usr/include -O2'.format(sysroot_path, sysroot_path)
    env['CPPFLAGS'] = env['CFLAGS']
    env['LDFLAGS'] = '--sysroot={} -L{}/usr/lib -L{}/lib'.format(sysroot_path, sysroot_path, os.path.join(toolchain_path, 'arm-linux-androideabi'))

    if enable_stl:
        env['LDFLAGS'] += ' -l' + build_config.get_android_stl()

    return env


def get_autotools_android_x86_env(toolchain_path, enable_stl=False):
    cross_path = os.path.join(toolchain_path, 'bin/i686-linux-android')
    sysroot_path = os.path.join(toolchain_path, 'sysroot')

    env = os.environ.copy()
    env['CPP'] = '{}-cpp'.format(cross_path)
    env['AR'] = '{}-ar'.format(cross_path)
    env['AS'] = '{}-as'.format(cross_path)
    env['NM'] = '{}-nm'.format(cross_path)
    env['CC'] = '{}-clang'.format(cross_path)
    env['CXX'] = '{}-clang++'.format(cross_path)
    env['LD'] = '{}-ld'.format(cross_path)
    env['RANLIB'] = '{}-ranlib'.format(cross_path)
    env['CFLAGS'] = '--sysroot={} -I{}/usr/include -O2'.format(sysroot_path, sysroot_path)
    env['CPPFLAGS'] = env['CFLAGS']
    env['LDFLAGS'] = '--sysroot={} -L{}/usr/lib -L{}/lib'.format(sysroot_path, sysroot_path, os.path.join(toolchain_path, 'i686-linux-android'))

    if enable_stl:
        env['LDFLAGS'] += ' -l' + build_config.get_android_stl()

    return env

def get_win32_vs_x86_env():
    return _get_vs_env(build_config.get_vs_vc_path_win32(), 'x86', build_config.get_msvc_sdk_version_win32())


def get_win32_vs_x64_env():
    return _get_vs_env(build_config.get_vs_vc_path_win32(), 'amd64', build_config.get_msvc_sdk_version_win32())


def get_win10_vs_x86_env():
    return _get_vs_env(build_config._get_vs_vc_path_win10(), 'x86')


def get_win10_vs_x64_env():
    return _get_vs_env(build_config._get_vs_vc_path_win10(), 'x86_amd64')


def get_win10_vs_arm_env():
    return _get_vs_env(build_config._get_vs_vc_path_win10(), 'x86_arm')


def _get_vs_env(vs_path, arch, sdk_ver=None):
    vsvars_path = '{}/vcvarsall.bat'.format(vs_path)

    if sdk_ver is not None:
        cmd = [vsvars_path, arch, sdk_ver, '&', 'set']
    else:
        cmd = [vsvars_path, arch, '&', 'set']
    sp = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    output = sp.communicate()[0]
    output = output.split("\r\n")

    env = os.environ.copy()
    for line in output:
        parts = line.strip().split('=')
        if len(parts) == 2:
            key = parts[0].strip().upper()
            value = parts[1].strip()
            env[key] = value

    return env


def mkpath(*args):
    path = os.path.join(*args)
    if not os.path.exists(path):
        os.makedirs(path)
    return path


# Default builders


def build_and_copy_libraries_win32_cmake(
        gen_folder_path, source_folder_path, root_project_path,
        solution_name, target_name,
        built_lib_name_debug, built_lib_name_release,
        result_lib_name_x86_debug, result_lib_name_x86_release,
        result_lib_name_x64_debug, result_lib_name_x64_release,
        cmake_additional_args = [], target_lib_subdir = '',
        msbuild_args=None,
        output_libs_path = 'Libs/lib_CMake',
        output_lib_folder='win',
        static_runtime=False):
    # Folders for the library to be built into
    build_x86_folder = os.path.join(gen_folder_path, 'build_win32_x86')
    build_x64_folder = os.path.join(gen_folder_path, 'build_win32_x64')

    if static_runtime:
        override_file=os.path.abspath(os.path.join(os.getcwd(), '../cmake/msvc_static_runtime.cmake'))
        cmake_additional_args.append('-DCMAKE_USER_MAKE_RULES_OVERRIDE={}'.format(override_file))

    # Generate & build
    cmake_generate_build_vs(build_x86_folder, source_folder_path, build_config.get_cmake_generator_win32_x86(), solution_name, target_name, 'Win32', cmake_additional_args, msbuild_args)
    cmake_generate_build_vs(build_x64_folder, source_folder_path, build_config.get_cmake_generator_win32_x64(), solution_name, target_name, 'x64', cmake_additional_args, msbuild_args)

    # Move built files into Libs/lib_CMake
    # TODO: update pathes after switching to new folders structure

    lib_path_x86_debug = os.path.join(build_x86_folder, os.path.join(target_lib_subdir, 'Debug', built_lib_name_debug))
    lib_path_x86_release = os.path.join(build_x86_folder, os.path.join(target_lib_subdir, 'Release', built_lib_name_release))
    lib_path_x64_debug = os.path.join(build_x64_folder, os.path.join(target_lib_subdir, 'Debug', built_lib_name_debug))
    lib_path_x64_release = os.path.join(build_x64_folder, os.path.join(target_lib_subdir, 'Release', built_lib_name_release))

    shutil.copyfile(lib_path_x86_debug, os.path.join(mkpath(root_project_path, output_libs_path, output_lib_folder, 'x86', 'Debug'), result_lib_name_x86_debug))
    shutil.copyfile(lib_path_x86_release, os.path.join(mkpath(root_project_path, output_libs_path, output_lib_folder, 'x86', 'Release'), result_lib_name_x86_release))
    shutil.copyfile(lib_path_x64_debug, os.path.join(mkpath(root_project_path, output_libs_path, output_lib_folder, 'x64', 'Debug'), result_lib_name_x64_debug))
    shutil.copyfile(lib_path_x64_release, os.path.join(mkpath(root_project_path, output_libs_path, output_lib_folder, 'x64', 'Release'), result_lib_name_x64_release))

    return (build_x86_folder, build_x64_folder)


def build_and_copy_libraries_win10_cmake(
        gen_folder_path, source_folder_path, root_project_path,
        solution_name, target_name,
        built_lib_name_debug, built_lib_name_release,
        result_lib_name_x86_debug, result_lib_name_x86_release,
        result_lib_name_x64_debug, result_lib_name_x64_release,
        result_lib_name_arm_debug, result_lib_name_arm_release,
        cmake_additional_args = [],
        output_libs_path = 'Libs/lib_CMake'):
    # Folders for the library to be built into
    build_win10_x86_folder = os.path.join(gen_folder_path, 'build_win10_x86')
    build_win10_x64_folder = os.path.join(gen_folder_path, 'build_win10_x64')
    build_win10_arm_folder = os.path.join(gen_folder_path, 'build_win10_arm')

    cmake_win10_flags = ['-DCMAKE_SYSTEM_NAME=WindowsStore', '-DCMAKE_SYSTEM_VERSION=10.0']
    cmake_additional_args.extend(cmake_win10_flags)

    # Generate & build
    cmake_generate_build_vs(build_win10_x86_folder, source_folder_path, build_config.get_cmake_generator_win10_x86(), solution_name, target_name, 'Win32', cmake_additional_args)
    cmake_generate_build_vs(build_win10_x64_folder, source_folder_path, build_config.get_cmake_generator_win10_x64(), solution_name, target_name, 'x64', cmake_additional_args)
    cmake_generate_build_vs(build_win10_arm_folder, source_folder_path, build_config.get_cmake_generator_win10_arm(), solution_name, target_name, 'ARM', cmake_additional_args)

    # Move built files into Libs/lib_CMake
    # TODO: update pathes after switching to new folders structure

    lib_path_win10_x86_debug = os.path.join(build_win10_x86_folder, os.path.join('Debug', built_lib_name_debug))
    lib_path_win10_x86_release = os.path.join(build_win10_x86_folder, os.path.join('Release', built_lib_name_release))
    lib_path_win10_x64_debug = os.path.join(build_win10_x64_folder, os.path.join('Debug', built_lib_name_debug))
    lib_path_win10_x64_release = os.path.join(build_win10_x64_folder, os.path.join('Release', built_lib_name_release))
    lib_path_win10_arm_debug = os.path.join(build_win10_arm_folder, os.path.join('Debug', built_lib_name_debug))
    lib_path_win10_arm_release = os.path.join(build_win10_arm_folder, os.path.join('Release', built_lib_name_release))

    shutil.copyfile(lib_path_win10_x86_debug, os.path.join(mkpath(root_project_path, output_libs_path, 'win10', 'Win32', 'Debug'), result_lib_name_x86_debug))
    shutil.copyfile(lib_path_win10_x86_release, os.path.join(mkpath(root_project_path, output_libs_path, 'win10', 'Win32', 'Release'), result_lib_name_x86_release))
    shutil.copyfile(lib_path_win10_x64_debug, os.path.join(mkpath(root_project_path, output_libs_path, 'win10', 'x64', 'Debug'), result_lib_name_x64_debug))
    shutil.copyfile(lib_path_win10_x64_release, os.path.join(mkpath(root_project_path, output_libs_path, 'win10', 'x64', 'Release'), result_lib_name_x64_release))
    shutil.copyfile(lib_path_win10_arm_debug, os.path.join(mkpath(root_project_path, output_libs_path, 'win10', 'arm', 'Debug'), result_lib_name_arm_debug))
    shutil.copyfile(lib_path_win10_arm_release, os.path.join(mkpath(root_project_path, output_libs_path, 'win10', 'arm', 'Release'), result_lib_name_arm_release))

    return (build_win10_x86_folder, build_win10_x64_folder, build_win10_arm_folder)


def build_and_copy_libraries_macos_cmake(
        gen_folder_path, source_folder_path, root_project_path,
        project_name, target_name,
        built_lib_name_release,
        result_lib_name_release,
        cmake_additional_args = [],
        target_lib_subdir='',
        output_libs_path='Libs/lib_CMake'):
    build_folder_macos = os.path.join(gen_folder_path, 'build_macos')

    xcodebuild_args=[
        'MACOSX_DEPLOYMENT_TARGET='+build_config.get_macos_deployment_target(),
    ]
    cmake_generate_build_xcode(build_folder_macos, source_folder_path, build_config.get_cmake_generator_macos(), project_name, target_name, cmake_additional_args, xcodebuild_args=xcodebuild_args)

    # Move built files into Libs/lib_CMake
    # TODO: update pathes after switching to new folders structure

    lib_path_macos_release = os.path.join(build_folder_macos, os.path.join(target_lib_subdir, 'Release', built_lib_name_release))

    shutil.copyfile(lib_path_macos_release, os.path.join(mkpath(root_project_path, output_libs_path, 'mac'), result_lib_name_release))

    return build_folder_macos


def build_and_copy_libraries_ios_cmake(
        gen_folder_path, source_folder_path, root_project_path,
        project_name, target_name,
        built_lib_name_release,
        result_lib_name_release,
        cmake_additional_args = [],
        output_libs_path='Libs/lib_CMake'):
    build_folder_ios = os.path.join(gen_folder_path, 'build_ios')

    toolchain_filepath = os.path.join(root_project_path, 'Sources/CMake/Toolchains/ios.toolchain.cmake')
    cmake_additional_args.append('-DCMAKE_TOOLCHAIN_FILE=' + toolchain_filepath)

    cmake_generate_build_xcode(build_folder_ios, source_folder_path, build_config.get_cmake_generator_macos(), project_name, target_name, cmake_additional_args)

    # Move built files into Libs/lib_CMake
    # TODO: update pathes after switching to new folders structure

    lib_path_ios_release = os.path.join(build_folder_ios, os.path.join('Release-iphoneos', built_lib_name_release))

    shutil.copyfile(lib_path_ios_release, os.path.join(mkpath(root_project_path, output_libs_path, 'ios'), result_lib_name_release))

    return build_folder_ios


def build_and_copy_libraries_android_cmake(
        gen_folder_path, source_folder_path, root_project_path,
        built_lib_name_release,
        result_lib_name_release,
        arm_abi='armeabi-v7a',
        cmake_additional_args = [],
        output_libs_path='Libs/lib_CMake'):
    build_android_armeabiv7a_folder = os.path.join(gen_folder_path, 'build_android_armeabiv7a')
    build_android_x86_folder = os.path.join(gen_folder_path, 'build_android_x86')

    android_ndk_folder_path = get_android_ndk_path()

    cmake_generate_build_ndk(build_android_armeabiv7a_folder, source_folder_path, android_ndk_folder_path, arm_abi, cmake_additional_args)
    cmake_generate_build_ndk(build_android_x86_folder, source_folder_path, android_ndk_folder_path, 'x86', cmake_additional_args)

    # Move built files into Libs/lib_CMake
    # TODO: update pathes after switching to new folders structure

    lib_path_android_armeabiv7a = os.path.join(build_android_armeabiv7a_folder, built_lib_name_release)
    lib_path_android_x86 = os.path.join(build_android_x86_folder, built_lib_name_release)

    shutil.copyfile(lib_path_android_armeabiv7a, os.path.join(mkpath(root_project_path, output_libs_path, 'android', 'armeabi-v7a'), result_lib_name_release))
    shutil.copyfile(lib_path_android_x86, os.path.join(mkpath(root_project_path, output_libs_path, 'android', 'x86'), result_lib_name_release))

    return (build_android_x86_folder, build_android_armeabiv7a_folder)

def build_and_copy_libraries_linux_cmake(
        gen_folder_path,
        source_folder_path,
        root_project_path,
        target,
        lib_name,
        cmake_additional_args = [],
        target_lib_subdir=''):

    build_folder = os.path.join(gen_folder_path, 'build_linux')
    cmake_additional_args.append('-DCMAKE_C_COMPILER=clang')
    cmake_additional_args.append('-DCMAKE_CXX_COMPILER=clang++')
    cmake_generate_build_make(build_folder, source_folder_path, build_config.get_cmake_generator_linux(), target, cmake_additional_args)

    # Move built files into Libs/lib_CMake
    # TODO: update pathes after switching to new folders structure
    source_dir = os.path.join(build_folder, target_lib_subdir)
    target_dir = os.path.join(root_project_path, 'Libs/lib_CMake/linux')

    shutil.copyfile(os.path.join(source_dir, lib_name),
                    os.path.join(target_dir, lib_name))
    return build_folder

def build_with_autotools(source_folder_path,
                         configure_args,
                         install_dir,
                         env=None,
                         configure_exec_name='configure',
                         make_exec_name='make',
                         make_targets=['all', 'install'],
                         shell_prefix='sh',
                         postclean=True):
    if isinstance(configure_exec_name, list):
        if sys.platform == 'win32':
            cmd = list(configure_exec_name)
        else:
            cmd = ['./{}'.format(configure_exec_name[0])]
            cmd.extend(configure_exec_name[1:])
            cmd.insert(0, shell_prefix)
    else:
        if sys.platform == 'win32':
            cmd = [configure_exec_name]
        else:
            cmd = ['./{}'.format(configure_exec_name)]
            cmd.insert(0, shell_prefix)

    cmd.extend(configure_args)
    if install_dir is not None:
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)
        cmd.append('--prefix=' + install_dir)

    enable_shell = sys.platform == 'win32'

    run_process(cmd, process_cwd=source_folder_path, environment=env, shell=enable_shell)

    if postclean:
        make_targets.append('clean')

    for target in make_targets:
        cmd = [make_exec_name, target]
        run_process(cmd, process_cwd=source_folder_path, environment=env, shell=enable_shell)


def run_once(fn):
    def wrapper(*args, **kwargs):
        if not wrapper.did:
            fn(*args, **kwargs)
            wrapper.did = True
    wrapper.did = False
    return wrapper
