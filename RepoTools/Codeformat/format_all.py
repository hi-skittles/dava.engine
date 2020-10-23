#!/usr/bin/env python

import fnmatch
import os
import subprocess
import argparse
import multiprocessing

parser = argparse.ArgumentParser()
parser.add_argument("-t", "--teamcity-notify", help="print list of non-formatted files in Teamcity format", action="store_true")
args = parser.parse_args()

def process(file):
    if args.teamcity_notify:
        check_format(file)
    else:
        format(file)

def check_format(file):
    proc = subprocess.Popen([cwd+'/'+execName, '-output-replacements-xml', '--style=file', file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if stdout.find("<replacement ") > -1:
        errorMsg = "##teamcity[message text=\'" + "%s not formatted" % file + "\' errorDetails=\'\' status=\'" + "ERROR" + "\']\n"
        print errorMsg

def format(file):
    proc = subprocess.Popen([cwd+'/'+execName, '-i', '--style=file', file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    proc.communicate()

cwd = os.getcwd()
if os.name == 'nt':
    execName = 'clang-format.exe'
else:
    execName = 'clang-format'

sources = ['../../Sources/Internal', '../../Modules', '../../Programs']
exclude = ['*/Libs/*', '*/Modules/CEFWebview/*']
extensions = ['cpp', 'h', 'mm', 'unittest']

def main():
    pool = multiprocessing.Pool()
    fileList = []
    for source in sources:
        for root, dirnames, filenames in os.walk(source):
            root = os.path.abspath(root)
            if any((fnmatch.fnmatch(root, x) for x in exclude)):
                # Skip directories from exclude patterns list
                continue
            for ext in extensions:
                for filename in fnmatch.filter(filenames, '*.'+ext):
                    fileList.append(os.path.join(root, filename))
    pool.map(process, fileList)

if __name__ == '__main__':
    multiprocessing.freeze_support() #here if program needs to be frozen
    main()  # execute this only when run directly, not when imported!
