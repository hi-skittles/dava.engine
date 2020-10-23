#!/usr/bin/env python
import sys
import os
import fnmatch
import argparse
import subprocess
import shutil
import json
import re
import time
import io

from string import Template
from collections import namedtuple

CoverageMinimum    = 80.0

class FileCover():
    def __init__(self, fileName, coverLines):
        self.file = fileName
        self.coverLines = coverLines

def enum(**enums):
    return type('Enum', (), enums)

def find_files(pattern, dirs):
    result = []
    for path in dirs:
        for root, dirs, files in os.walk(path):
            for name in files:
                if fnmatch.fnmatch(name, pattern):
                    result.append(os.path.join(root, name))
    return result

def get_exe( pathExecute ):

    pathExecute = os.path.realpath( pathExecute )

    if os.name == 'nt':
        return pathExecute
    else:
        # mac os
        dirName  = os.path.dirname ( pathExecute )
        baseName = os.path.basename( pathExecute )
        baseName, extApp = os.path.splitext(baseName)

        if extApp == '.app' :
            return '{0}/{1}.app/Contents/MacOS/{1}'.format( dirName, baseName )
        else:
            return pathExecute

def check_sting_in_file( str, fname ):
    with open(fname) as dataf:
        return any(str in line for line in dataf)

def retrieve_name(var):
    import inspect
    callers_local_vars = inspect.currentframe().f_back.f_locals.items()
    return [var_name for var_name, var_val in callers_local_vars if var_val is var][0]

def configure_file( file_path_template, file_path_out, values_string_list, values_obj ) :
    dicts = {}

    for item in values_string_list :
        item_value  = '' 
        vl_name = retrieve_name(values_obj)
        if hasattr( values_obj, item ) :
            exec("%s = %s" % ('item_value', '{0}.{1}'.format(vl_name,item))) 

        dicts.update( { item : item_value } )  

    with open( file_path_template ) as file_template, \
         open( file_path_out, 'w' ) as file_generated :  
            template = Template( file_template.read() )
            file_generated.write( template.safe_substitute( dicts ) )

class CheckTimeDependence():
    def __init__(self, pathExecute, timeFile ):

        self.pathExecute = pathExecute
        self.timeFile   = timeFile

    def create_time_file( self ):

        dirTimeFile = os.path.dirname( self.timeFile ) 
        if not os.path.isdir( dirTimeFile ):
            os.makedirs( dirTimeFile ) 
        with open(self.timeFile, "w") as  file:
            file.write( time.ctime(os.path.getmtime(self.pathExecute)) )

    def is_updated( self ):

        timeExecute     = time.ctime(os.path.getmtime(self.pathExecute))
        timeExecuteOld  = ''

        if os.path.isfile( self.timeFile ) :
            with open(self.timeFile) as f:
                timeExecuteOld = f.readline()

        return timeExecute != timeExecuteOld


class CoverageReport():

    def __init__(self, arg ):

        self.arg                    = arg
        self.pathBuild              = arg.pathBuild
        self.pathExecute            = arg.pathExecute
        self.targetArgs             = arg.targetArgs
        self.pathReportOut          = arg.pathReportOut
        self.pathReportOutFull      = os.path.join( arg.pathReportOut, 'CoverageFull' ) 
        self.pathReportOutTests     = os.path.join( arg.pathReportOut, 'CoverageTests' )

        self.buildConfig            = arg.buildConfig
        self.notExecute             = arg.notExecute
        self.teamcityMode           = arg.teamcityMode

        self.coverageTmpPath        = os.path.join( arg.pathBuild, 'CoverageTmpData' )
    
        self.pathExecuteDir         = os.path.dirname ( self.pathExecute )
        self.executeName            = os.path.basename( self.pathExecute )
        self.executeName, ExecuteExt  = os.path.splitext( self.executeName )
        
        self.pathCoverageDir        = os.path.dirname (os.path.realpath(__file__))
        self.pathExecuteTime        = os.path.join( self.pathBuild, 'CMakeFiles/{0}.time'.format( self.executeName ) )


        self.tfExec                 = CheckTimeDependence( self.pathExecute, self.pathExecuteTime )


        self.coverFilePath          = os.path.join    ( self.pathExecuteDir,  'Tests.cover')
        self.pathLlvmCov            = os.path.join    ( self.pathCoverageDir, 'llvm-cov')
        self.pathLlvmProfdata       = os.path.join    ( self.pathCoverageDir, 'llvm-profdata')
        self.pathCallLlvmGcov       = os.path.join    ( self.pathCoverageDir, 'llvm-gcov.sh')
        self.pathLcov               = os.path.join    ( self.pathCoverageDir, 'lcov')
        self.pathCovInfoFull        = os.path.join    ( self.coverageTmpPath, 'cov_full.info')
        self.pathCovInfoTests       = os.path.join    ( self.coverageTmpPath, 'cov_tests.info')        
        self.pathGenHtml            = os.path.join    ( self.pathCoverageDir, 'genhtml')

        self.pathFullHtml           = os.path.join    ( self.pathReportOutFull,  'index.html')
        self.pathLocalHtml          = os.path.join    ( self.pathReportOutTests, 'index.html')

        self.pathRelativeFullHtml   = os.path.join    ( 'CoverageFull',       'index.html')
        self.pathRelativeLocalHtml  = os.path.join    ( 'CoverageTests',      'index.html')

        self.pathMixHtml            = os.path.join    ( self.pathReportOut,      'index.html')
        self.pathMixHtmlTemplate    = os.path.join    ( self.pathCoverageDir,    'mix_index_html.in')

        self.pathUnityPack          = ''
        self.testsCoverage          = {}
        self.testsCoverageFiles     = []

        self.mixHtmlValueStrList    = { 'full_title' , 'full_date', 'full_linesHit', 'full_linesTotal', 'full_linesCoverage',
                                        'full_funcHit', 'full_funcTotal', 'full_funcCoverage', 'full_coverLegendCovLo', 'full_coverLegendCovMed',
                                        'full_coverLegendCovHi', 'full_coverage_href', 'local_title', 'local_date', 'local_linesHit', 'local_linesTotal', 'local_linesCoverage',
                                        'local_funcHit', 'local_funcTotal', 'local_funcCoverage', 'local_coverLegendCovLo', 'local_coverLegendCovMed',
                                        'local_coverLegendCovHi', 'local_coverage_href', 
                                        'local_funcCoverageAtribute', 'local_linesCoverageAtribute', 'full_funcCoverageAtribute', 'full_linesCoverageAtribute' }

        if self.notExecute == 'false' and self.tfExec.is_updated() == True:
            self.__clear_old_gcda()
            self.tfExec.create_time_file()
            pathExecuteExt = get_exe( self.pathExecute )
            os.chdir( self.pathExecuteDir )
            self.__execute( [ pathExecuteExt, self.targetArgs ] )
    
        self.__processing_gcda_gcno_files()
        self.__load_json_cover_data()

    def __clear_old_gcda( self ):        
        for rootdir, dirs, files in os.walk( self.pathBuild ):
            for file in files:   
                if file.endswith( ('.gcda')  ):
                    os.remove( os.path.join(rootdir, file) ) 

    def __build_print( self, str ):        
        sys.stdout.write("{0}\n".format(str) )
        sys.stdout.flush()

    
    def __teamcity_print( self, str ):
        if self.teamcityMode == 'true' :
            self.__build_print( str )

    def __execute(self, param) :

        sub_process = subprocess.Popen(param, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        subProcessContinue = True

        while subProcessContinue:
            try:
                line = sub_process.stdout.readline()
                sys.stdout.write(line)
                sys.stdout.flush()
                if line == '':
                    subProcessContinue = False
                    continue
            except IOError as err:
                sys.stdout.write('error: [ {} ]'.format(err.message))
                sys.stdout.flush()

    def __load_json_cover_data( self ):
        if not os.path.isfile(self.coverFilePath) or not os.access(self.coverFilePath, os.R_OK):
            print 'ERROR : file {0} is missing or is not readable'.format( self.coverFilePath )
            return

        coverFile               = open(self.coverFilePath).read()
        jsonData                = json.loads(coverFile)
        testsFolders            = {} 

        self.pathUnityPack      = jsonData[ 'UnityFolder' ]
        self.testsCoverage      = {}
        self.testsCoverageFiles = []
        
        for file in jsonData[ 'CoverageFolders' ]:
            folders = jsonData[ 'CoverageFolders' ][ file ]
            testsFolders[ file ] = folders.split(' ')

        
        for test in jsonData[ 'Coverage' ]:
            testedFiles = jsonData[ 'Coverage' ][ test ].split(' ')
            for file in testedFiles:
                find_list = find_files( file, testsFolders[ file ] )
                if len( find_list ):
                    fileCover = FileCover( find_list[0], None )
                    self.testsCoverage.setdefault(test, []).append( fileCover )
                    self.testsCoverageFiles +=  [find_list[0]]
        print 'TRACE OUTPUT ', self.testsCoverageFiles

    def __processing_gcda_gcno_files( self ):
        
        if os.path.isdir( self.coverageTmpPath ):
            shutil.rmtree( self.coverageTmpPath )
                
        os.makedirs( self.coverageTmpPath ) 

        if self.buildConfig:
            pathConfigSegment = os.path.join(  '.build', self.buildConfig )   

        #coppy '.gcda','.gcno' files
        listCoverData = []
        for rootdir, dirs, files in os.walk( self.pathBuild ):
            if rootdir.find( self.coverageTmpPath ) == -1 :
                if not self.buildConfig or rootdir.find( pathConfigSegment ) != -1:
                    for file in files:   
                        if file.endswith( ('.gcda','.gcno')  ): 
                            listCoverData += [os.path.join(rootdir, file)]
        
        for file in listCoverData:
            baseName = os.path.basename( file )
            pathOutFile = os.path.join(self.coverageTmpPath, baseName)

            if os.path.exists(pathOutFile):
                os.remove(pathOutFile)

            shutil.copy(file, self.coverageTmpPath )


    def __error_log_coverage_file( self, test, file ):

        os.chdir( self.pathExecuteDir );

        pathExecuteExt = get_exe( self.pathExecute )
        param = [ self.pathLlvmCov, 
                  'show', 
                  pathExecuteExt, 
                  '-instr-profile={0}.profdata'.format(self.executeName), 
                  file  
                ] 
        print param
        sub_process = subprocess.Popen(param, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        subProcessContinue = True
        while subProcessContinue:
            try:
                line = sub_process.stdout.readline()
                if line == '':
                    subProcessContinue = False
                    continue

                split_line = line.split( )
                if len( split_line ) == 0:
                    continue
                
                coverValue = re.findall('\d+', split_line[0])
                if len( coverValue ) > 0 and int(coverValue[0]) == 0:
                    lineValue = re.findall('\d+', split_line[1])
                    print '{0}:{1}: warning: bad cover: {2}'.format(file,int(lineValue[0]),test)

            except IOError as err:
                sys.stdout.write(err.message)
                sys.stdout.flush()

    def __find_unity_pack_gcda( self, file ):

        if len( file ) == 0:
            return []

        unity_files = []
        for root, dirs, files in os.walk( self.pathUnityPack ):
            for name in files:
                unity_file = os.path.join(root, name)
                if( check_sting_in_file(file, unity_file) ) :

                    fileName = os.path.basename( unity_file )
                    fileName, fileExt = os.path.splitext( fileName )
                    return [ '{0}.gcda'.format(fileName) ]
        return []

    def __generate_mix_html( self ):
        import urllib
        import HTMLParser
        import fileinput
        from HTMLParser import HTMLParser
        from htmlentitydefs import name2codepoint 

        class MyHTMLParser(HTMLParser):

            def feed( self, html ):
                self.numData   = 0
                self.numAtr    = 0
                self.data      = []
                self.atributes = []                                                        
                HTMLParser.feed( self, html )

            def handle_starttag(self, tag, attrs):
                #print "Start tag:", self.numData, " :", tag
                if attrs:
                    #print "     class: ", self.numAtr, attrs[0][1]
                    self.numAtr += 1
                    atr = attrs[0][1]
                    self.atributes += [ atr ]                               

            def handle_data(self, data):
                #print "Data      ", self.numData, " :", data
                self.numData += 1
                self.data += [ data ]

            def get_data( self ):
                return self.data

            def get_atributes( self ):
                return self.atributes
               

        parser = MyHTMLParser()

        class ValueList: #pass
            def load( self, data, type, root ):

                data      = parser.get_data()
                atributes = parser.get_atributes()

                if type == 'full':
                    self.full_title = 'FULL coverage report'#data[10]
                    self.full_date = data[48]
                    self.full_coverLegendCovLo = data[65]
                    self.full_coverLegendCovMed = data[68]
                    self.full_coverLegendCovHi = data[71]
                    self.full_linesHit = data[38]
                    self.full_linesTotal = data[40]
                    self.full_linesCoverage = data[42]
                    self.full_funcHit = data[53]
                    self.full_funcTotal = data[55]
                    self.full_funcCoverage = data[57]
                    self.full_coverage_href = root.pathRelativeFullHtml 

                    self.full_linesCoverageAtribute = atributes[21]                    
                    self.full_funcCoverageAtribute = atributes[27]                    

                if type == 'local':
                    self.local_title = 'TESTS coverage report'#data[10]
                    self.local_date = data[48]
                    self.local_coverLegendCovLo = data[65]
                    self.local_coverLegendCovMed = data[68]
                    self.local_coverLegendCovHi = data[71]
                    self.local_linesHit = data[38]
                    self.local_linesTotal = data[40]
                    self.local_linesCoverage = data[42]
                    self.local_funcHit = data[53]
                    self.local_funcTotal = data[55]
                    self.local_funcCoverage = data[57]
                    self.local_coverage_href = root.pathRelativeLocalHtml

                    self.local_funcCoverageAtribute = atributes[21]
                    self.local_linesCoverageAtribute = atributes[27]

        vl = ValueList() 

        f = urllib.urlopen( self.pathFullHtml  )
        html = f.read()
        f.close()
        parser.feed( html )
        vl.load( parser, 'full', self )

        f = urllib.urlopen( self.pathLocalHtml )
        html = f.read()
        f.close()
        parser.feed( html )
        vl.load( parser, 'local', self )

        configure_file( self.pathMixHtmlTemplate, self.pathMixHtml, self.mixHtmlValueStrList, vl )

        defTopLink     = '<td width="35%" class="headerValue">top level</td>' 
        newTopLinkMix  = '<td width="35%" class="headerValue"><a href="{0}">top level</a> DAVA coverage</td>'.format( '../index.html' )

        for url in [ self.pathFullHtml, self.pathLocalHtml ]:
            filedata = None
            with  open(url, 'r') as file:
              filedata = file.read()
            filedata = filedata.replace(defTopLink, newTopLinkMix)
            with  open(url, 'w') as file:
              file.write(filedata)



    def generate_report_html( self ):

        self.__teamcity_print( '##teamcity[testStarted name=\'Generate cover html\']' )

        if os.path.isdir( self.pathReportOut ):
            shutil.rmtree( self.pathReportOut )
        else:
            os.makedirs(self.pathReportOut)      

        ###
        params = [ self.pathLcov,
                    '--directory',      self.coverageTmpPath,  
                    '--base-directory', self.pathExecuteDir,
                    '--gcov-tool',      self.pathCallLlvmGcov,
                    '--capture',   
                    '-o', self.pathCovInfoFull
                 ]        
        self.__build_print( params )                 
        self.__execute( params )
        
        ###        
        params = [ self.pathLcov,
                    '--extract', self.pathCovInfoFull, 
                    '-o', self.pathCovInfoTests
                 ] + self.testsCoverageFiles
        self.__build_print( params )         
        self.__execute( params ) 

        ###
        params = [ self.pathGenHtml,
                   self.pathCovInfoFull, 
                   '-o', self.pathReportOutFull,
                   '--legend'
                 ]
        self.__build_print( params )                 
        self.__execute( params) 

        params = [ self.pathGenHtml,
                   self.pathCovInfoTests, 
                   '-o', self.pathReportOutTests,
                   '--legend'
                 ]
        self.__build_print( params )                 
        self.__execute( params)         
        ###

        self.__teamcity_print( '##teamcity[testFinished name=\'Generate cover html\']' )
        
        self.__generate_mix_html()


    def generate_report_coverage( self ):

        self.__teamcity_print( '##teamcity[testStarted name=\'Coverage test\']' )

        eState = enum( UNDEF      =0, 
                       FIND_File  =1, 
                       FIND_Lines =2, 
                       FIND_Taken =3 )

        self.__execute( [ self.pathLlvmProfdata, 'merge', '-o', '{0}.profdata'.format(self.executeName), 'default.profraw' ] )
        
        os.chdir( self.coverageTmpPath )
        for test in self.testsCoverage:
            state = eState.FIND_File

            for fileCover in self.testsCoverage[ test ]:
                fileName          = os.path.basename( fileCover.file )
                fileName, fileExt = os.path.splitext( fileName )
                fileGcda     = '{0}.gcda'.format(fileName)
                fileGcdaList = [ ]
                self.__build_print( "Processing file: {0}".format( fileName ) )

                if  os.path.isfile( fileGcda ) :
                    fileGcdaList = [ fileGcda ]
                else:
                    fileGcdaList = self.__find_unity_pack_gcda( fileCover.file )

                if len( fileGcdaList ) == 0:
                    for rootdir, dirs, files in os.walk( self.coverageTmpPath ):
                        for file in files:   
                            if file.endswith( ('.gcda')  ): 
                                fileGcdaList += [os.path.join(rootdir, file)]

                for fileGcda in fileGcdaList:

                    params = [ self.pathLlvmCov, 'gcov',
                               '-f',  
                               '-b', 
                               fileGcda
                             ]

                    subProcess         = subprocess.Popen(params, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    subProcessContinue = True

                    while subProcessContinue:
                        try:
                            line = subProcess.stdout.readline()
                            split_line = line.split( )

                            if line == '':
                                subProcessContinue = False
                                continue

                            if len( split_line ) == 0:
                                continue

                            if   ( state == eState.FIND_File 
                                   and split_line[0] == 'File' 
                                   and split_line[1].replace('\'','') == fileCover.file ):
                                state = eState.FIND_Lines

                            elif ( state == eState.FIND_Lines 
                                   and split_line[0] == 'Lines' ):
                                fileCover.coverLines = float(re.findall("\d+\.\d+", split_line[1] )[0])
                                state = eState.FIND_File
                                subProcessContinue = False
                                subProcess.kill()

                        except IOError as err:
                            sys.stdout.write(err.message)
                            sys.stdout.flush()
        
        for test in self.testsCoverage:
            for fileCover in self.testsCoverage[ test ]:
                if CoverageMinimum > fileCover.coverLines:                
                    basename = os.path.basename( fileCover.file )
                    self.__build_print( '{0}:1: error: bad cover test {1} in file {2}: {3}% must be at least: {4}%'.format(fileCover.file,test,basename,fileCover.coverLines,CoverageMinimum) )
            self.__build_print( '' )
        
        if self.teamcityMode == 'false' :
            for test in self.testsCoverage:
                for fileCover in self.testsCoverage[ test ]:
                    if CoverageMinimum > fileCover.coverLines:                
                        basename = os.path.basename( fileCover.file )
                        self.__error_log_coverage_file( test, fileCover.file )
        else:
            for test in self.testsCoverage:
                for fileCover in self.testsCoverage[ test ]:
                    if CoverageMinimum > fileCover.coverLines:                
                        basename = os.path.basename( fileCover.file )
                        self.__teamcity_print( '##teamcity[testFailed name=\'Cover\' message=\'{0}\' details=\'file {1}: {2}% must be at least: {3}%\']'.format(test,basename,fileCover.coverLines,CoverageMinimum) )

        self.__teamcity_print( '##teamcity[testFinished name=\'Coverage test\']' )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( '--pathBuild', required = True )
    parser.add_argument( '--pathExecute', required = True )
    parser.add_argument( '--pathReportOut', required = True )
    parser.add_argument( '--buildConfig', choices=['Debug', 'Release','RelWithDebinfo'] )
    parser.add_argument( '--notExecute', default = 'false', choices=['true', 'false'] ) 
    parser.add_argument( '--teamcityMode', default = 'false', choices=['true', 'false'] )    
    parser.add_argument( '--buildMode', default = 'false', choices=['true', 'false'] )
    parser.add_argument( '--runMode', default = 'false', choices=['true', 'false'] )
    parser.add_argument( '--targetArgs', default = '')

    options = parser.parse_args()

    cov = CoverageReport( options )

    if options.buildMode == 'true' :
        cov.generate_report_coverage()
    elif options.runMode == 'true' :
        cov.generate_report_html()
    else:
        cov.generate_report_html()
        cov.generate_report_coverage() 

if __name__ == '__main__':
    main()


















