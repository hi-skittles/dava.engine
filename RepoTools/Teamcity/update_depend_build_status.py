#!/usr/bin/env python
import sys
import os
import argparse
import json
import time
import stash_api
import team_city_api
import stash_api
import common_tool

def __parser_args():
    arg_parser = argparse.ArgumentParser()

    arg_parser.add_argument('--commit')
    arg_parser.add_argument('--costum_dependent_build')
    arg_parser.add_argument('--container_configuration_id')

    stash_api.argparse_add_argument( arg_parser )
    team_city_api.argparse_add_argument( arg_parser )

    return arg_parser.parse_args()

def main():

    args = __parser_args()

    stash = stash_api.init_args(args)
    teamcity = team_city_api.init_args(args)

    build_status = teamcity.get_build_status(args.container_configuration_id)

    if 'build_dependencies' in build_status:

        commit = args.commit

        if commit == None:
            for build in build_status['build_dependencies']:

                costum_build_properties = teamcity.get_build_properties(build['id'])

                if 'env.from_commit' in costum_build_properties:
                    commit = costum_build_properties[ 'env.from_commit' ]

                if commit != None:
                    break

        if commit == None or commit == '0':
            pull_requests_number = common_tool.get_pull_requests_number(build_status['branchName'])
            commit_list = stash.get_commits( pull_requests_number )
            commit = commit_list['values'][0]['id']

        for build in build_status['build_dependencies']:

            build_dependencies_status = None
            build_id = None
            request_configuration_id = None

            if args.costum_dependent_build == build['buildTypeId']:
                costum_build_properties = teamcity.get_build_properties( build['id'] )

                if 'request_configuration' in costum_build_properties:
                    request_configuration_id = costum_build_properties['request_configuration']

                if 'env.run_build_id' in costum_build_properties:
                    build_id = costum_build_properties['env.run_build_id']
                else:
                    build_id = build['id']
            else:
                build_id =  build['id']

            build_dependencies_status = teamcity.get_build_status( build_id )

            configuration_info = teamcity.configuration_info(build_dependencies_status['buildTypeId'])

            description = ''

            status = None

            if build_dependencies_status['state'] == 'running' :
                description = build_dependencies_status['branchName'] + ' In progress ...'
                status = 'INPROGRESS'

            if build_dependencies_status['state'] == 'queued':
                description = build_dependencies_status['branchName'] + ' Queued ...'
                status = 'INPROGRESS'

            if status == None and 'status' in build_dependencies_status :
                if build_dependencies_status['status'] == 'SUCCESS':
                    description = build_dependencies_status['branchName'] + ' Good job !'
                    status = 'SUCCESSFUL'
                elif build_dependencies_status['status'] == 'FAILURE':
                    description = build_dependencies_status['branchName'] + ' Need to work !'
                    status = 'FAILED'

            if status == None:
                description = build_dependencies_status['branchName']
                status = 'FAILED'

            configuration_name = None
            build_configuration_id = None

            if args.costum_dependent_build == build['buildTypeId']:
                configuration_name = configuration_info['config_path']
                build_configuration_id = request_configuration_id
            else:
                configuration_name = configuration_info['name']

            if build_configuration_id == None:
                build_configuration_id = build_dependencies_status['buildTypeId']

            configuration_history = teamcity.configuration_history( build['buildTypeId'], build['branchName'] )

            is_requesting = True

            if status == 'FAILED':
                if configuration_history != None and len( configuration_history ):

                    last_build = configuration_history[0]
                    for build_in_history in configuration_history:
                        if build_in_history['number'] > last_build['number']:
                            last_build = build_in_history['number']

                    if build['number'] < last_build['number']:
                        is_requesting =  ('status' in last_build and last_build['status'] != 'SUCCESS')

            if is_requesting == True:

                print 'Update: commit[ {} ] status[ {} ] configuration_name[ {} ]'.format(commit, status,  build_dependencies_status['buildTypeId'] )

                stash.report_build_status( status,
                                       build_configuration_id,
                                       configuration_name,
                                       build_dependencies_status['webUrl'],
                                       commit,
                                       description=description )


if __name__ == '__main__':
    main()
