#!/usr/bin/env python2.7

import argparse
import imp
import os
import sys
import traceback
import shutil
import stat
import time


# Allow importing from Private folder
sys.path.append('Private')
import build_utils
import build_config


# Path for temporary output produced by builders
output_path = os.path.abspath('output')
# Imported builders dict to avoid multiple imports
imported_builders = {}
# List of invoked builders to avoid multiple invokes
invoked_builders = {}
# Host platform: win32, darwin, linux
# For linux sys.platform contains 'linux2' on python 2.x and 'linux' on python 3.x
# Use host_platform not sys.platform
host_platform = None


def rmtree_error(operation, name, exc):
    os.chmod(name, stat.S_IWRITE)
    os.unlink(name)


def remove_folder(folder_path):
    if os.path.exists(folder_path) and os.path.isdir(folder_path):
        shutil.rmtree(folder_path, onerror=rmtree_error)


def import_library_builder_module(name):
    # Import only once

    if name not in imported_builders:
        builder_file_path = os.path.join(name, 'build.py')
        # Change imported module name to avoid clashes with built-in modules
        imported_builders[name] = imp.load_source(
            'dava_builder_' + name, builder_file_path)

    return imported_builders[name]


def remember_built_library_result(fn):
    def wrapper(name, targets, skip_dependencies):
        result = fn(name, targets, skip_dependencies)
        invoked_builders[name] = result
        return result
    return wrapper


@remember_built_library_result
def build_library(name, targets, skip_dependencies):
    # Check if we already invoked builder for this library
    # (it could be declared as a dependency for another library)
    if name in invoked_builders:
        return invoked_builders[name]

    print 'Started processing library: \"{}\"'.format(name)

    # Get builder module
    builder = import_library_builder_module(name)

    current_platform = host_platform
    supported_targets = builder.get_supported_targets(current_platform)

    # Check if it should be built on current platform
    if not supported_targets:
        print ('Skipping library \"{}\" since it can\'t or shouldn\'t be built'
               'on current platform '
               '(build.py reported no supported targets)').format(name)
        return False

    # Check dependencies
    if not skip_dependencies:
        dependencies = get_dependencies_for_library(builder, targets)
        for dependency in dependencies:
            print 'Found dependency \"{}\" for library \"{}\"'.format(
                dependency, name)
            if build_library(dependency, targets, skip_dependencies) is False:
                print ('Skipping library \"{}\" since its dependency (\"{}\") '
                       'couldn\' be built').format(name, dependency)
                return False

    # Build

    library_working_dir = os.path.join(output_path, name)
    project_root_path = os.path.abspath('..')

    if not os.path.exists(library_working_dir):
        os.makedirs(library_working_dir)

    # Change working directory to builder's one
    os.chdir(name)

    result = True

    try:
        for target in targets:
            if target in supported_targets:
                print ('Building library \"{}\" for target \"{}\"').format(
                    name, target)
                builder.build_for_target(
                    target,
                    library_working_dir,
                    project_root_path)
            else:
                print ('Skipping target \"{}\" since library \"{}\" '
                       'does not support it '
                       'on current platform').format(target, name)
    except Exception:
        print 'Couldn\'t build library \"{}\". Exception details:'.format(name)
        traceback.print_exc()
        result = False

    # Revert working dir changes
    os.chdir('..')

    return result


def get_all_libraries():
    return [folder_name for folder_name in os.listdir(os.getcwd())
            if os.access(os.path.join(folder_name, 'build.py'), os.F_OK)]


def get_dependencies_for_library(builder_module, targets):
    dependencies = []
    for target in targets:
        target_deps = builder_module.get_dependencies_for_target(target)
        for target_dep in target_deps:
            if target_dep not in dependencies:
                dependencies.append(target_dep)

    return dependencies


def print_info(library, targets):
    builder = import_library_builder_module(library)
    download_url = builder.get_download_info()
    supported_targets = {'win32': builder.get_supported_targets('win32'),
                         'darwin': builder.get_supported_targets('darwin'),
                         'linux': builder.get_supported_targets('linux')}
    dependencies = get_dependencies_for_library(
        import_library_builder_module(library),
        targets)
    print ('{}\nDownload url: {}\nSupported targets: {}\n'
           'Dependencies: {}\n').format(
                library,
                download_url,
                supported_targets,
                dependencies)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'libs',
        nargs='*',
        help='list of libraries to build (build all if none specified)')
    parser.add_argument(
        '-t',
        '--target',
        default='all',
        nargs='?',
        choices=all_targets,
        help='targets to build for (build for all targets if not specified)')
    parser.add_argument(
        '-sd',
        '--skip-dependencies',
        action='store_true',
        help='build without first building dependencies')
    parser.add_argument(
        '-i',
        '--info',
        action='store_true',
        help=('only show information about selected libraries'
              '(all if none was specified)'))
    parser.add_argument(
        '-v',
        '--verbose',
        action='store_true',
        help='print additional logs')
    parser.add_argument(
        '--no-clean',
        action='store_true',
        help='do not clean temporary output folder')
    parser.add_argument(
        '--ignore-errors',
        action='store_true',
        help='do not halt execution if a builder failed')
    parser.add_argument(
        '--suppress_build_warnings',
        action='store_true',
        help='Do not output compiler warning messages while building libraries')

    args = parser.parse_args()

    return args

if __name__ == "__main__":
    # Detect host platform
    host_platform = sys.platform
    # For linux sys.platform contains 'linux2' on python 2.x and 'linux' on python 3.x
    if host_platform.startswith('linux'):
        host_platform = 'linux'

    # List of all targets
    if host_platform == 'win32':
        all_targets = ['win32', 'win10', 'android']
    elif host_platform == 'darwin':
        all_targets = ['ios', 'macos', 'android']
    elif host_platform == 'linux':
        all_targets = ['android', 'linux']
    else:
        print 'Unknown platform: %s. Aborting' % (host_platform)
        sys.exit(1)

    # Setup and parse arguments

    args = parse_args()

    build_utils.verbose = args.verbose
    build_utils.suppress_build_warnings = args.suppress_build_warnings;
    build_utils.output_folder_path = output_path
    build_utils.dava_folder_path = os.path.abspath('..')

    build_config.configure(host_platform, build_utils.dava_folder_path)

    targets_to_process = all_targets if args.target == 'all' else [args.target]

    # Check that Android NDK is configured
    if 'android' in targets_to_process:
        ndk_path = build_utils.get_android_ndk_path()
        if ndk_path is None or not os.path.isdir(ndk_path):
            print ('Android target is specified, but Android NDK folder '
                   'couldn\'t be found (required for CMake). '
                   'Did you forget to put it into DavaConfig.in?\nAborting')
            sys.exit()

    # If specific libraries weren't specified, use all of them
    if args.libs != []:
        libraries_to_process = list(set(args.libs))
    else:
        libraries_to_process = get_all_libraries()

    # Import builders
    for lib in libraries_to_process:
        try:
            import_library_builder_module(lib)
        except IOError:
            print ('Couldn\'t import builder module for library named \'{}\', '
                   'misprint?\nAborting').format(lib)
            sys.exit()

    # Process

    if args.info:
        print 'Libraries:\n'
        for lib in libraries_to_process:
            print_info(lib, targets_to_process)
        print 'Total: {}'.format(len(libraries_to_process))
    else:
        # Clean previous run (if it was invoked with --no-clean)
        if not args.no_clean:
            remove_folder(output_path)

        # Build
        print 'Selected libraries: ' + str(libraries_to_process)
        print 'Selected targets: ' + str(targets_to_process) + '\n'
        failed = []
        start = time.time()
        for lib in libraries_to_process:
            result = build_library(lib, targets_to_process, args.skip_dependencies)
            if result is False:
                failed.append(lib)
                if not args.ignore_errors:
                    break
        end = time.time()

        if not failed:
            print '\nFinished. Successfully built: {} library(s)'.format(
                len(invoked_builders))
        else:
            print '\nFinished. Builders failed for these libraries: {}'.format(
                str(failed))

        time_spent = end - start
        print 'Time spent: ' + str(round(time_spent, 1)) + ' seconds'

        # Clean
        if not args.no_clean:
            remove_folder(output_path)
