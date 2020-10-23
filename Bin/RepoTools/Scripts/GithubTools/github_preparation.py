#!/usr/bin/env python
import sys
import os
import argparse
import subprocess


class Preparation:
    system_list = set(['.gitattributes', '.gitignore', '.gitmodules'])
    white_list = set()
    clear_list = set(['Bin/RepoTools/Scripts/GithubTools/white_list.txt', 'Bin/RepoTools/Scripts/GithubTools/github_preparation.py'])

    def __init__(self, base_path):
        self.base_path = os.path.abspath(base_path)
        self.read_white_list()

        self.read_white_list()
        self.do()

    def read_white_list(self):
        try:
            files_list = open(os.path.abspath(self.base_path + '/Bin/RepoTools/Scripts/GithubTools/white_list.txt'))
            for line in files_list:
                full_path = os.path.abspath(self.base_path + line.partition('#')[0].replace('\n', '')).replace("!", "")
                path = line.partition('#')[0].replace('\n', '').replace("!", "")[1:]
                if "!" in line:
                    if path:
                        self.clear_list.add(path)
                else:
                    if os.path.exists(path):
                        if path != self.base_path:
                            self.white_list.add(path)

        except IOError:
            print "##teamcity[message text='Unable to open file white_list.txt' status='ERROR']"
            sys.exit(3)

    def execute_cmd(self, cmd):
        sys.stdout.write(cmd + '\n')
        sys.stdout.flush()
        try:
            pass
            cmd_log = subprocess.check_output(cmd, shell=True, cwd=self.base_path)
            sys.stdout.write(cmd_log)
            sys.stdout.flush()
        except subprocess.CalledProcessError as cmd_except:
            print "##teamcity[message text='CMD Error' errorDetails='CMD: {cmd} ERROR: {error}' status='ERROR']".format(cmd=cmd, error=cmd_except.output)
            sys.exit(3)

    def do(self):
        os.chdir(self.base_path)

        for path in self.system_list:
            cmd = "git add {path}".format(path=path)
            self.execute_cmd(cmd)

        for path in self.white_list:
            cmd = "git add -f {path}".format(path=path)
            self.execute_cmd(cmd)

        for path in self.clear_list:
            cmd = "git reset {path}".format(path=path)
            self.execute_cmd(cmd)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Utility add files to the repository from the whitelist.')
    parser.add_argument('--path', required=True, help='Example: --path /var/git/repo')

    args = parser.parse_args()

    preparation = Preparation(args.path)