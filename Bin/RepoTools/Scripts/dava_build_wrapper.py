#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
import codecs
import os
import argparse
import subprocess

ansi = not sys.platform.startswith("win")

def setup_console(sys_enc='utf-8', use_colorama=True):
    """
    Set sys.defaultencoding to `sys_enc` and update stdout/stderr writers to corresponding encoding

    .. note:: For Win32 the OEM console encoding will be used istead of `sys_enc`
    """
    global ansi
    reload(sys)
    try:
        if sys.platform.startswith("win"):
            import ctypes
            enc = "cp%d" % ctypes.windll.kernel32.GetOEMCP()
        else:
            enc = (sys.stdout.encoding if sys.stdout.isatty() else
                        sys.stderr.encoding if sys.stderr.isatty() else
                            sys.getfilesystemencoding() or sys_enc)

        sys.setdefaultencoding(sys_enc)

        if sys.stdout.isatty() and sys.stdout.encoding != enc:
            sys.stdout = codecs.getwriter(enc)(sys.stdout, 'replace')

        if sys.stderr.isatty() and sys.stderr.encoding != enc:
            sys.stderr = codecs.getwriter(enc)(sys.stderr, 'replace')

        if use_colorama and sys.platform.startswith("win"):
            try:
                from colorama import init
                init()
                ansi = True
            except:
                pass

    except:
        pass

class LogParser():

    error = []
    warning = []

    def teamcity_strip_chars(self, line):
        return line.replace("|", "||")\
            .replace("'", "|'")\
            .replace("\n", "|n")\
            .replace("\r", "")\
            .replace("[", "|[")\
            .replace("]", "|]")

    def check_error_in_line(self, line):
        if 'error:' in line or ': error' in line or ': fatal error' in line:
            return True

    def get_error(self, process, error, teamcity_log):

        if sys.platform == 'darwin':
            error_details = process.stdout.readline()
            error_details_position = process.stdout.readline()
        else:
            error_details = ''
            error_details_position = ''

        error = error.strip()

        error_message = '\x1b[0;31m{}\x1b[00m\n'.format(error) + error_details +\
                        '\x1b[0;32m{}\x1b[00m'.format(error_details_position)

        self.error.append(error_message)

        if teamcity_log == True:
            error = self.teamcity_strip_chars(error)

            teamcity_error = "##teamcity[message text='{0}' errorDetails='{1}' status='{2}']\n"\
                .format(error, error_details.strip(), 'ERROR')
            teamcity_error = teamcity_error + "##teamcity[buildProblem description='{}']\n"\
                .format(error)

            return teamcity_error

    def check_warning_in_line(self, line):
        if 'warning:' in line or (': warning' in line and ': warning treated as error' not in line):
            return True

    def get_warning(self, process, warning, teamcity_log):

        if sys.platform == 'darwin':
            error_details = process.stdout.readline()
            error_details_position = process.stdout.readline()

            if '^' not in error_details_position:
                """ If this line no have position information then just write to console """
                error_details_position = ''
                sys.stdout.write(error_details_position)
        else:
            error_details = ''
            error_details_position = ''

        warning = warning.strip()

        error_message = '\x1b[0;33m{0}\x1b[00m\n{1}'.format(warning, error_details) +\
                        '\x1b[0;32m{}\x1b[00m'.format(error_details_position)

        self.warning.append(error_message)

        if teamcity_log == True:
            warning = self.teamcity_strip_chars(warning)
            teamcity_warning = "##teamcity[message text='{0}' status='{1}']\n".format(warning, 'WARNING')

            return teamcity_warning

    def report(self):
        """ Final report about all errors and warnings """
        sys.stdout.write("Final result:\n")

        if self.warning:
            print '\n\x1b[0;33mWarnings:\x1b[00m'
            for warning_item in set(self.warning):
                sys.stdout.write(warning_item)
        else:
            print '\n\x1b[0;33mWarnings: 0\x1b[00m'

        if self.error:
            print '\n\x1b[0;31mErrors:\x1b[00m'
            for error_item in set(self.error):
                sys.stdout.write(error_item)
        else:
            print '\x1b[0;31mErrors: 0\x1b[00m'

        sys.stdout.flush()


    def parse(self, options):
        BuildWrapper(options)

class BuildWrapper():

    sub_process = None

    param = []

    def __init__(self, arg):
        self.arg = arg
        self.incredi_build = True if arg.incrediBuild == 'true' else False
        self.config = arg.config
        self.teamcity_log = True if arg.teamcityLog == 'true' else False
        self.path_to_root = arg.pathToRoot
        self.path_to_dava = arg.pathToDava
        self.path_to_build = arg.pathToBuild
        self.native_options = arg.nativeOptions if arg.nativeOptions else ''

        # Mac OS X platform
        if sys.platform == 'darwin':
            self.param = [
                self.path_to_dava + os.path.normpath('/Bin/CMakeMac/CMake.app/Contents/bin/cmake'),
                '--build',
                os.path.normpath(self.path_to_build),
                '--config',
                self.config,
                '--',
            ]

            shell = False

        # Windows 32 platform
        if sys.platform == 'win32':
            self.param = [
                self.path_to_dava + os.path.normpath('/Bin/CMakeWin32/bin/cmake.exe'),
                '--build',
                os.path.normpath(self.path_to_build),
                '--config',
                self.config,
                '--',
            ]

            shell = True

        if arg.nativeOptions:
            self.param.append(self.native_options)

        self.__execute(shell)

    def __execute(self, shell=True):

        print(self.param)

        sub_process = subprocess.Popen(self.param, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=shell)

        sub_process_continue = True
        parse = LogParser()

        sys.stdout.write("##teamcity[progressStart 'Build started...']\n")
        while sub_process.poll() is None:
            try:
                line = sub_process.stdout.readline()
                line = self.pathCleaning(line)

                if sub_process_continue:
                    if parse.check_error_in_line(line):
                        line = parse.get_error(sub_process, line, self.teamcity_log)

                    if parse.check_warning_in_line(line):
                        line = parse.get_warning(sub_process, line, self.teamcity_log)

                sys.stdout.write(line)
                sys.stdout.flush()

            except IOError as err:
                sys.stdout.write(err.message)

        parse.report()

        sys.stdout.write("##teamcity[progressFinish 'Build finished...']\n")
        sys.stdout.write("\n")
        sys.stdout.flush()

        sys.exit(sub_process.returncode)

    def pathCleaning(self, line):
        """ Clear path before output """
        if self.path_to_root:
            line = line.replace(self.path_to_root, '..')
        else:
            line = line.replace(self.path_to_dava.rsplit(os.sep, 1)[0], '..')
        return line


def main():

    setup_console()

    parser = argparse.ArgumentParser()
    parser.add_argument('--incrediBuild', default='false', choices=['true', 'false'])
    parser.add_argument('--config', required=True)
    parser.add_argument('--teamcityLog', default='false', choices=['true', 'false'])
    parser.add_argument('--pathToRoot')
    parser.add_argument('--pathToDava', required=True)
    parser.add_argument('--pathToBuild', required=True)
    parser.add_argument('--nativeOptions', help='Native options for cmake, for example: --nativeOptions="/p:Platform=x86"')

    options = parser.parse_args()

    parser = LogParser()
    parser.parse(options)


main()