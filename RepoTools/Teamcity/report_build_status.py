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

    stash_api.argparse_add_argument( arg_parser )
    team_city_api.argparse_add_argument( arg_parser )

    # branch or commit
    arg_parser.add_argument( '--branch' )
    arg_parser.add_argument( '--commit' )

    arg_parser.add_argument( '--status', required = True, choices=[ 'INPROGRESS', 'SUCCESSFUL', 'FAILED' ] )
    arg_parser.add_argument( '--configuration_name', required = True )

    arg_parser.add_argument( '--build_url' )

    arg_parser.add_argument( '--abbreviated_build_name', default = 'false', choices=[ 'true', 'false' ] )
    arg_parser.add_argument( '--reported_status', default = 'true', choices=[ 'true', 'false' ] )

    arg_parser.add_argument( '--root_build_id' )

    arg_parser.add_argument( '--description' )


    return arg_parser.parse_args()


def main():

    args = __parser_args()

    if args.reported_status == 'false':
        return

    stash = stash_api.init_args(args)
    teamcity = team_city_api.init_args(args)

    stash    = stash_api.ptr()
    teamcity = team_city_api.ptr()

    commit = None

    if args.commit == None:
        pull_requests_number = common_tool.get_pull_requests_number( args.branch )
        if pull_requests_number != None:
            branch_info = stash.get_pull_requests_info(pull_requests_number)
            commit = branch_info['fromRef']['latestCommit']
    else:
        commit = args.commit

    if commit != None :

        build_url = args.build_url
        if args.root_build_id :
            build_status = teamcity.get_build_status(args.root_build_id)
            build_url = build_status['webUrl']

        assert (build_url != None ), "build_url == None"

        configuration_info = teamcity.configuration_info(args.configuration_name)


        if args.description == None:
            build_status = teamcity.get_build_status(args.root_build_id)
            description = build_status['statusText']
        else:
            description = args.description

        build_name = None
        if args.abbreviated_build_name == None:
            build_name = configuration_info['config_path']
        else:
            build_name = configuration_info['name']

        stash.report_build_status( args.status,
                                   args.configuration_name,
                                   build_name,
                                   build_url,
                                   commit,
                                   description=description)
    else:
        common_tool.flush_print( 'Is not a pull requests [{}] or commit [{}]  '.format( args.branch, args.commit ) )

if __name__ == '__main__':
    main()
