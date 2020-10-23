import os
import sys
import subprocess
import re
import argparse
import convert_graphics as cg

g_default_context = {'file' : 'rev_data_source_tools.txt', 
                     'gpu' : cg.g_default_gpu,
                     'configId': 'local',
                     'write': False
                     }

def get_input_context():
    global g_default_context
    parser = argparse.ArgumentParser(description='Check updated source.')
    parser.add_argument('--file', nargs='?', default = g_default_context['file'])
    parser.add_argument('--gpu', nargs='?', choices = cg.g_allowed_gpu, default = g_default_context['gpu'])
    parser.add_argument('--configId', nargs='?', default = g_default_context['configId'])
    parser.add_argument('--write', action='store_true', default=False)

    return vars(parser.parse_args())

def executeSubprocessCommand(commandParams):
    print "subprocess.Popen " + "[%s]" % ", ".join(map(str, commandParams))
    p = subprocess.Popen(commandParams, stdout=subprocess.PIPE, stderr=subprocess.PIPE)#, shell=False)
    (stdout, stderr) = p.communicate()
    print stderr
    return stdout

def get_revision_of_dirs(list_dirs):
    revisions=[]
    for direcrory in sorted(list_dirs):
        os.chdir(direcrory)
        print os.getcwd() , list_dirs[direcrory]
        check_rev=''
        if list_dirs[direcrory] == 'svn':
            log = executeSubprocessCommand(["svn", "info"])
            check_rev = re.search('Last Changed Rev:\s[0-9]+', log)
            if check_rev:
                revisions.append(re.search('[\d]+', check_rev.group(0)).group(0))
            else:
                revisions.append('none')
        else:
            commit = executeSubprocessCommand(["git", "log", "-1", "--pretty=tformat:%H", "./" ])
            revisions.append(commit.strip())
    return revisions

def write_rev_in_file(file, revisions):
    writeToFile = context['write']
    if writeToFile:
        f = open(file, 'w')
        for rev in revisions:
            f.write(rev + '\n')
        f.close()
        sys.exit(0)
    print 'need to start convert'
    sys.exit(1)

def do(context=g_default_context):

    print context

    file_name = context['file']
    gpu=context['gpu']
    configId=context['configId']

    revisions_file = os.path.join(os.getcwd(), file_name)
    check_dirs={ os.path.realpath('../DataSource/'):'svn', os.path.realpath('../Tools/ResEditor/'):'svn', os.path.realpath('../../dava.framework/Tools/Bin/'):'git'}

    new_revisions = get_revision_of_dirs(check_dirs)
    new_revisions.append(gpu)
    new_revisions.append(configId)

    if os.path.isfile(revisions_file):
        old_revisions = open(revisions_file).readlines()
        if len(new_revisions) != len(old_revisions):
            write_rev_in_file(revisions_file, new_revisions)
        for i in range(len(new_revisions)):
            if str(old_revisions[i]).strip() != str(new_revisions[i]).strip():
                print ' old rev: ' + str(old_revisions[i]) + ' new rev: ' + str(new_revisions[i])
                write_rev_in_file(revisions_file, new_revisions)
    else:
        print 'file ' + file_name + ' doesn\'t exist'
        write_rev_in_file(revisions_file, new_revisions)

if __name__ == '__main__':
    context = get_input_context()
    do(context)