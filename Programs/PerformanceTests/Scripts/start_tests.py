#!/usr/bin/env python

# start application on device or PC(Mac) 
# then start server inside this script to listen unit test log output
# example:
# win32: 
# > cd to dava.framework/Projects/UnitTests/Reports
# > python start_unit_tests.py
# mac os x: 
# make DerivedData relative to project go to Xcode->Preferences->Location->DerivedData select relative
# build project
# > cd to dava.framework/Projects/UnitTests/Reports
# > python start_unit_tests.py
# android:
# deploy application to device first
# > python start_unit_tests.py android
# iOS:
# deploy application on device first
# "Enable UI Automation" must be turned on in the "Developer" section of the "Settings" app on device
# > python start_unit_tests.py ios

import subprocess
import argparse
import re
import sys
import os.path
import glob
import signal


def get_postfix(platform):
    if platform == 'win32':
        return '.exe'
    elif platform == 'darwin':
        return '.app'
    else:
        return ''

PRJ_NAME_BASE = "PerformanceTests"
PRJ_POSTFIX = get_postfix(sys.platform)

parser = argparse.ArgumentParser(description='Start tests')
parser.add_argument('--branch', default = 'development')
parser.add_argument('--platform', default = 'ios', choices=['ios', 'android'])
parser.add_argument('--build', default='release', choices=['release', 'debug'])

parser.add_argument('--not-install', dest='not-install', nargs='?', const=True)
parser.add_argument('--without-ui', dest='without-ui', nargs='?', const=True)

parser.add_argument('--chooser', nargs='?', const=True)
parser.add_argument('--test')
parser.add_argument('--universal-test', dest='universal-test')

parser.add_argument('--test-time', dest='test-time')
parser.add_argument('--test-frames', dest='test-frames')
parser.add_argument('--frame-delta', dest='frame-delta')

parser.add_argument('--statistic-start-time', dest='statistic-start-time')
parser.add_argument('--statistic-end-time', dest='statistic-end-time')

parser.add_argument('--debug-frame', dest='debug-frame')
parser.add_argument('--max-delta', dest='max-delta')

args = vars(parser.parse_args())

BUILD_PARAMS = "branch " + args['branch'] + " platform " + args['platform'] + " build " + args['build']
TEST_PARAMS = ""

if args['chooser']:
    TEST_PARAMS += "-chooser"

if args['without-ui']:
    TEST_PARAMS += " -without-ui "

if args['statistic-start-time']:
    TEST_PARAMS += " -statistic-start-time " + args['statistic-start-time']

    if args['statistic-end-time']:
        TEST_PARAMS += " -statistic-end-time " + args['statistic-end-time']

if args['universal-test']:
    TEST_PARAMS += " -universal-test ";

    if args['universal-test'] != "All":
        TEST_PARAMS += args['universal-test'];

if args['test'] and args['test'] != "All":    
    TEST_PARAMS += " -test " + args['test']

    if args['test-time']:
        TEST_PARAMS += " -test-time " + args['test-time']

    if args['test-frames']:
        TEST_PARAMS += " -test-frames " + args['test-frames']

    if args['frame-delta']:
        TEST_PARAMS += " -frame-delta " + args['frame-delta']   
        
    if args['debug-frame']:
        TEST_PARAMS += " -debug-frame " + args['debug-frame']

    if args['max-delta']:
        TEST_PARAMS += " -max-delta " + args['max-delta']

start_on_android = False
start_on_ios = False

if args['platform'] == "ios":
    if args['build'] == 'release':
        buildPath = "../_build/Release-iphoneos/"
    else:
        buildPath = "../_build/Debug-iphoneos/"

print "Build params : " + BUILD_PARAMS 
print "Performance tests command line params : " + TEST_PARAMS

if args['platform'] == "android":
    start_on_android = True
elif args['platform']  == "ios":
    start_on_ios = True

sub_process = None

def start_performance_tests_on_android_device():
    global sub_process
    # if screen turned off
    device_state = subprocess.check_output(['adb', 'shell', 'dumpsys', 'power'])
    if device_state.find("mScreenOn=false") != -1:
        # turn screen on
        subprocess.check_call(['adb', 'shell', 'input', 'keyevent', '26'])
    # unlock device screen
    subprocess.check_call(['adb', 'shell', 'input', 'keyevent', '82'])
    # clear log before start tests
    subprocess.check_call(["adb", "logcat", "-c"])
    # start adb logcat and gather output DO NOT filter by TeamcityOutput tag
    # because we need interrupt gather log when unittests process finished
    sub_process = subprocess.Popen(
        ["adb", "logcat", "-s", "TeamcityOutput"],
        stdout=subprocess.PIPE)
    # start unittests on device
    subprocess.Popen(
        ["adb", "shell", "am", "start", "-es", TEST_PARAMS, "-n", "com.dava.performancetests/com.dava.engine.DavaActivity"])
    return sub_process


if start_on_ios:
    # ../build/ios-deploy -d --noninteractive -b ../build/UnitTests.app
    if args['not-install']:
        sub_process = subprocess.Popen(["./ios-deploy", "-a", TEST_PARAMS, "-m", "--noninteractive", "-b", buildPath +
                                    PRJ_NAME_BASE + PRJ_POSTFIX],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(PRJ_NAME_BASE + PRJ_POSTFIX + " run")
    else:   
        sub_process = subprocess.Popen(["./ios-deploy", "-a", TEST_PARAMS, "-d", "--noninteractive", "-b", buildPath +
                                        PRJ_NAME_BASE + PRJ_POSTFIX],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print("copy " + PRJ_NAME_BASE + PRJ_POSTFIX + " on device and run")
elif start_on_android:
    sub_process = start_performance_tests_on_android_device()

app_exit_code = None

branch = args['branch']
branch = branch.replace("/", "_")

if not os.path.exists("../artifacts"):
    os.makedirs("../artifacts")

files = glob.glob("../artifacts/*.txt")
for f in files:
    if os.path.isfile(f):
        os.remove(f)

if not os.path.exists("../artifacts/materials"):
    os.makedirs("../artifacts/materials")

device_name = "device_unrecognized"
test_name = ""

continue_process_stdout = True

while continue_process_stdout:
    try:
        line = sub_process.stdout.readline()
        if line != "":

            if(line.find("##teamcity") != -1):

                line = line.replace("(lldb) ", "") 

                # parse test name and device in ##teamcity[message text='']
                if line.find("device") != -1:
                    device_name = line.split("text")[1].split("'")[1].split("|")[0].split("device_")[1]

                if line.find("testStarted") != -1:

                    test_name = line.split("name")[1].split("'")[1].replace(": ", "_")

                    if 'frame_delta_file' in locals():
                        frame_delta_file.close()
                    if 'statistic_file' in locals():
                        statistic_file.close()

                    frame_delta_file = open("../artifacts/frame_delta" + "_test_" + test_name + "_branch_" + branch + "_device_" + device_name + ".txt", "w")
                    statistic_file = open("../artifacts/statistic" + "_test_" + test_name + "_branch_" + branch + "_device_" + device_name + ".txt", "w")

                if line.find("MaterialsTest") != -1:

                    material_test_file = open("../artifacts/materials/material_test.txt", "w")

                if line.find("MaterialSubtestName:") != -1:

                    subtest_name = line.split("Name:")[1].split("'")[0].split("|")[0]
                    material_test_file.write(subtest_name + "\n")

                    if 'subtest_delta_file' in locals():
                        subtest_delta_file.close()

                    subtest_delta_file = open("../artifacts/materials/" + subtest_name + ".txt", "w")

                if line.find("buildStatisticValue") != -1:

                    key = line.split("key")[1].split("'")[1]
                    value = line.split("value")[1].split("'")[1]

                    #write material frame delta
                    if line.find("Material_frame_delta") != -1:
                        subtest_delta_file.write(value + "\n")

                    # write Frame_delta build statistic to file
                    elif line.find("Frame_delta") != -1:
                        frame_delta_file.write(value + "\n")

                    # append info to build statistic keys for compare on teamcity
                    else:

                        # write loading time, fps and memory statistic to file
                        if line.find("fps") != -1 or line.find("memory") != -1 or line.find("Loading") != -1:
                            statistic_file.write(key + " " + value + "\n")

                        #write material metrics
                        if line.find("Material_test_time") != -1:
                            material_test_file.write("Test time : " + value + "\n")

                        if line.find("Material__elapsed_test_time") != -1:
                            material_test_file.write("Test elapsed time : " + value + "\n\n")

                        #per frame metrics in ms
                        if line.find("frame") != -1:
                            value = str(float(value) * 1000)

                        key = test_name + "_" + key + "_branch_" + branch + "_" + device_name

                        sys.stdout.write("##teamcity[buildStatisticValue key='" + key + "' value='" + value + "']" + "\n")
                        sys.stdout.flush()    

                else:
                    sys.stdout.write(line)
                    sys.stdout.flush()
                        

                if line.find("Finish all tests.") != -1:    # this text marker helps to detect good \
                                        
                    app_exit_code = 0

                    frame_delta_file.close()
                    statistic_file.close()

                    if "material_test_file" in locals():
                        material_test_file.close()

                    if "subtest_delta_file" in locals():    
                        subtest_delta_file.close()

                    if start_on_android:
                        # we want to exit from logcat process because sub_process.stdout.readline() will block
                        # current thread
                        if sys.platform == "win32":
                            sub_process.send_signal(signal.CTRL_C_EVENT)
                        else:
                            sub_process.send_signal(signal.SIGINT)
                        continue_process_stdout = False
        else:
            continue_process_stdout = False
    except IOError as err:
        sys.stdout.write(err.message)
        sys.stdout.flush()

if app_exit_code is None:
    app_exit_code = sub_process.poll()

sys.exit(app_exit_code)
