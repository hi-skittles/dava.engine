#!/usr/bin/env python
import subprocess
import sys
import os.path
import os
import signal
import argparse

class TestsRunner():

    def __execute( self, param ) :

        sys.stdout.write( 'execute : {}\n'.format(param) )
        sys.stdout.flush()

        returncode = subprocess.call(param)
        
        return returncode

    def __get_app_postfix( self ):
        if sys.platform == 'win32':
            return '.exe'
        elif sys.platform == 'darwin':
            return '.app'
        else:
            return ''

    def __init__( self, arg ):
        self.selfTestPrograms   = {  'ResourceEditor'           : [ ['--selftest'],               '' ]
                                    ,'QuickEd'                  : [ ['--selftest'],               '' ]
                                    ,'ResourceArchiver'         : [ ['--selftest'],               'console' ]
                                    ,'TexConverter'             : [ ['--selftest', '--teamcity'], 'console' ]
                                  }
                                    
        self.prjNameBase        = 'UnitTests'
        self.prjName            = self.prjNameBase + self.__get_app_postfix()

        self.davaRoot           = arg.davaRoot
        self.buildDir           = arg.buildDir
        self.platform           = arg.platform 
        self.sdk_dir            = arg.sdk_dir 
        self.coverage_artifacts = arg.coverage_artifacts 

        self.appDir             = os.path.join (arg.buildDir, 'app' ) 
        self.appOtherDir        = os.path.join (arg.buildDir, 'app_other' )

        self.programsDir        = os.path.join ( arg.davaRoot, 'Programs')

        self.run_at_teamcity    = True


    def start_self_tests( self ):

        if self.platform != 'ANDROID' and not os.path.exists(self.appDir):
            sys.stdout.write( 'not exists folder [ {} ]\n'.format(self.appDir) )
            return

        if self.platform != 'ANDROID':
            os.chdir( self.appDir )

        sys.stdout.write( 'start SelfTests - {}\n'.format(self.selfTestPrograms) )

        for program, params in self.selfTestPrograms.iteritems():
            
            app_path = None

            if self.platform == 'WIN':
                app_path = os.path.join (self.appDir,'{}.exe'.format(program))

            elif self.platform == 'MACOS':
                if  params[1] == 'console':
                    app_path = os.path.join (self.appDir, program )                    
                else:
                    app_path = os.path.join (self.appDir, '{}.app'.format(program),'Contents', 'MacOS', program)

            if app_path != None and not os.path.exists(app_path) : 
                sys.stdout.write( 'not exists program [ {} ]\n'.format(app_path) )
                continue

            if app_path != None:
                sys.stdout.write('##teamcity[testStarted name=\'Self test {}\']\n'.format(program))
                sys.stdout.flush()

                returncode = self.__execute( [app_path] + params[0] )

                if returncode != 0 :
                    message = 'SelfTest {} failed - {}'.format(program,returncode)
                    sys.stdout.write('##teamcity[testFailed name=\'Self test {}\' message=\'{}\']\n'.format(program, message))
                
                sys.stdout.write('##teamcity[testFinished name=\'Self test {}\']\n'.format(program))
                sys.stdout.flush()
 


    def start_unit_tests_sub_process_win( self ):
        
        app_path = os.path.join (self.appOtherDir, self.prjName )
        
        if not os.path.exists(app_path):
            sys.stdout.write( 'not exists program [ {} ]'.format(app_path) )
            return None

        commandLine = [ app_path ]
        if self.run_at_teamcity == True:
            commandLine.extend(["-teamcity"])
        sub_process = subprocess.Popen(commandLine, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return sub_process


    def start_unit_tests_sub_process_macos( self ):
        app_path = os.path.join (self.appOtherDir, self.prjName, 'Contents', 'MacOS', self.prjNameBase )

        if not os.path.exists(app_path):
            sys.stdout.write( 'not exists program [ {} ]'.format(app_path) )
            return None

        commandLine = [app_path]
        if self.run_at_teamcity == True:
            commandLine.extend(["-teamcity"])

        sub_process = subprocess.Popen(commandLine, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return sub_process


    def start_unit_tests_sub_process_ios( self ):
        app_path = os.path.join (self.appOtherDir, self.prjName )
        
        if not os.path.exists(app_path):
            sys.stdout.write( 'not exists program [ {} ]'.format(app_path) )
            return None

        ios_deploy_path = os.path.join (self.davaRoot, 'Programs','UnitTests','Scripts','ios-deploy' )
        commandLine = [ios_deploy_path, "-d", "--noninteractive", "-b", app_path]
        if self.run_at_teamcity == True:
            commandLine.extend(["-a", "-teamcity"]) 
        sub_process = subprocess.Popen(commandLine, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return sub_process

    def start_unit_tests_sub_process_android( self ):
        adb_path = os.path.join (self.sdk_dir,'platform-tools','adb')

        unit_tests_apk_path = os.path.join ( self.programsDir, 'UnitTests', 'Platforms', 'Android' )
        unit_tests_apk_path = os.path.join ( unit_tests_apk_path, 'UnitTests', 'build', 'outputs', 'apk' )
        unit_tests_apk_path = os.path.join ( unit_tests_apk_path, 'UnitTests-fat-release.apk' )

        if not os.path.exists(unit_tests_apk_path):
            sys.stdout.write( 'not exists program [ {} ]'.format(unit_tests_apk_path))
            return None

        self.__execute( [adb_path, 'uninstall', 'com.dava.unittests'] )
        self.__execute( [adb_path, '-d', 'install', '-r', unit_tests_apk_path ] )

        # if screen turned off
        device_state = subprocess.check_output([adb_path, 'shell', 'dumpsys', 'power'])
        if device_state.find("mScreenOn=false") != -1:
            # turn screen on
            subprocess.check_call([adb_path, 'shell', 'input', 'keyevent', '26'])
        # unlock device screen
        subprocess.check_call([adb_path, 'shell', 'input', 'keyevent', '82'])
        # clear log before start tests
        subprocess.check_call([adb_path, "logcat", "-c"])
        # start adb logcat and gather output DO NOT filter by TeamcityOutput tag
        # because we need interrupt gather log when unittests process finished
        sub_process = subprocess.Popen(
            [adb_path, "logcat", "-s", "TeamcityOutput", "AndroidRuntime:E", "ActivityManager:W"],
            stdout=subprocess.PIPE)
        # start unittests on device
        commandLine = [adb_path, "shell", "am", "start", "-n", "com.dava.unittests/com.dava.engine.DavaActivity"]
        if self.run_at_teamcity == True:
            commandLine.extend(["-es", "-teamcity"]) 

        subprocess.Popen(commandLine)
        return sub_process

    def start_unit_tests_sub_process_winuap( self ):
        name = []

        for root, dirs, files in os.walk(self.appOtherDir):
            for file in files:
                if file.endswith(".appxbundle") and self.prjNameBase in file:
                     name.append(os.path.join(root, file))

        package_name = name[0]
        arch = sys.argv[2]

        uwp_runner_path = os.path.join (self.davaRoot, 'Bin','UWPRunner.exe' )

        if not os.path.exists(uwp_runner_path):
            sys.stdout.write( 'not exists program [ {} ]'.format(uwp_runner_path) )
            return None

        commandLine = [uwp_runner_path, 
                                        '--package', package_name, 
                                        '--arch', arch,
                                        '--tc_test', '--dava_app']
        if self.run_at_teamcity == True:
            commandLine.extend(['--cmd_line', '-teamcity']) 

        sub_process = subprocess.Popen(commandLine, stdout=subprocess.PIPE, stderr=subprocess.PIPE)    
        return sub_process

    def start_unit_tests_sub_process_linux( self ):
        return None

    def start_unit_tests( self ):
        sys.stdout.write('##teamcity[testStarted name=\'UnitTests\']\n')
        sys.stdout.flush()

        if self.platform != 'ANDROID' and not os.path.exists(self.appOtherDir):
            sys.stdout.write( 'not exists folder [ {} ]\n'.format(self.appOtherDir) )
            return

        if self.platform != 'ANDROID':
            os.chdir( self.appOtherDir )

        sub_process = None

        if self.platform == 'WIN':
            sub_process = self.start_unit_tests_sub_process_win()
        elif self.platform == 'MACOS':
            sub_process = self.start_unit_tests_sub_process_macos()

        elif self.platform == 'IOS': 
            sub_process = self.start_unit_tests_sub_process_ios()

        elif self.platform == 'ANDROID':
            sub_process = self.start_unit_tests_sub_process_android()

        elif self.platform == 'WINUAP':
            sub_process = self.start_unit_tests_sub_process_winuap()

        elif self.platform == 'LINUX':
            sub_process = self.start_unit_tests_sub_process_linux()

        if sub_process != None:

            app_exit_code = 0
            continue_process_stdout = True

            while continue_process_stdout:
                try:
                    line = sub_process.stdout.readline()
                    if line != '':
                        teamcity_line_index = line.find("##teamcity")
                        if teamcity_line_index != -1:
                            teamcity_line = line[teamcity_line_index:]
                            sys.stdout.write(teamcity_line)
                            sys.stdout.flush()
                        if line.find("Finish all tests.") != -1:    # this text marker helps to detect good \
                                                                    #  finish tests on ios device (run with lldb)
                            if self.platform == 'ANDROID':
                                # we want to exit from logcat process because sub_process.stdout.readline() will block
                                # current thread
                                if sys.platform == "win32":
                                    sub_process.send_signal(signal.CTRL_C_EVENT)
                                else:
                                    sub_process.send_signal(signal.SIGINT)
                                continue_process_stdout = False
                        if line.find("E/AndroidRuntime") != -1:
                            sys.stdout.write(line)
                            sys.stdout.flush()
                        if line.find("Force finishing activity com.dava.unittests") != -1 or \
                           line.find("end=assert=msg") != -1:
                            app_exit_code = 1
                            sys.stdout.write(line)
                            sys.stdout.flush()
                            
                            if self.platform == 'ANDROID':
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

            if self.platform == 'MACOS':
                pathCoverageDir      = os.path.realpath( os.path.join(self.davaRoot, 'RepoTools', 'coverage') )
                pathHtmlReportScript = os.path.join( pathCoverageDir, 'coverage_report.py' )   
                pathBuild            = self.buildDir
                pathExecute          = os.path.realpath( os.path.join( os.getcwd(), '{0}.app'.format( self.prjNameBase ) ) )
                pathReportOut        = os.path.join( pathBuild, 'Coverage')    

                buildMode            = None

                if self.coverage_artifacts == 'true':
                    buildMode = 'false'
                else:
                    buildMode = 'true'

                params = [  'python', pathHtmlReportScript,
                            '--pathBuild', pathBuild,  
                            '--pathExecute', pathExecute,                   
                            '--pathReportOut', pathReportOut,
                            '--buildConfig', 'RelWithDebinfo',
                            '--notExecute' , 'true',
                            '--teamcityMode' , 'true', 
                            '--buildMode', buildMode
                             ]

                subprocess.call(params)

            if app_exit_code == 1:
                sys.exit( self.app_exit_code )

            if sub_process.returncode != 0:
                sys.exit( sub_process.returncode )



        sys.stdout.write('##teamcity[testFinished name=\'UnitTests\']\n')
        sys.stdout.flush()


    def start( self ):
        self.start_self_tests()        
        self.start_unit_tests()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( '--davaRoot', default = '@DAVA_ROOT_DIR@' ) 
    parser.add_argument( '--buildDir', default = '@CMAKE_BINARY_DIR@' )    
    parser.add_argument( '--platform', default = '@DAVA_PLATFORM_CURRENT@' )    
    parser.add_argument( '--sdk_dir',  default = '' )
    parser.add_argument( '--coverage_artifacts', default = 'true', choices=['true', 'false'] )    

    arg = parser.parse_args()

    tests_runner = TestsRunner( arg )

    tests_runner.start()


if __name__ == '__main__':
    main()





