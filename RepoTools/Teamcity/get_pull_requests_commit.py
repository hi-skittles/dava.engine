#!/usr/bin/env python
import sys
import os
import common_tool
import subprocess
import re
import argparse
import requests
import json

def main(args):

    args

    branch = re.search(r'\d+', args.branch)

    print(branch)

    if branch:
        url = "https://stash-dava.wargaming.net/rest/api/1.0/projects/DF/repos/dava.framework/pull-requests/" + branch.group()
        response = requests.get(url, auth=(args.stash_login, args.stash_password))
        data = json.loads(response.text)
        commit = data['fromRef']['latestCommit']
    else:
        commit_log = subprocess.check_output("git log -1", shell=True)
        commit_log =  commit_log.split("\n")
        commit_log = [x.lstrip() for x in commit_log if x != '']
        commit = [s for s in commit_log if 'commit' in s]
        commit = ''.join(commit).split(' ')[1]

    if commit:
        common_tool.flush_print_teamcity_set_parameter( 'env.from_commit', commit )

if __name__ == '__main__':
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument( '--branch', required = True )
    arg_parser.add_argument( '--stash_login', required = True )
    arg_parser.add_argument( '--stash_password', required = True )
    args = arg_parser.parse_args()

    main(args)
