#!/usr/bin/env python
import sys
import os
import argparse
import json
import time
import team_city_api
import json
import ast

def __parser_args():
    arg_parser = argparse.ArgumentParser()

    team_city_api.argparse_add_argument( arg_parser )

    arg_parser.add_argument( '--configuration_name', required = True )
    arg_parser.add_argument( '--branch', required = True )

    arg_parser.add_argument( '--queue_at_top', default = 'false', choices = [ 'true', 'false' ] )
    arg_parser.add_argument( '--properties' ) #'id1:2323,id2:ferwerwer,id3:dvdsf3434,...'
    arg_parser.add_argument( '--agent_name' )

    return arg_parser.parse_args()


def main():

    args = __parser_args()

    teamcity = team_city_api.init_args(args)

    triggering_options = []

    if args.queue_at_top == 'true':
        triggering_options = [ 'queueAtTop' ]

    properties = {}
    if args.properties:
        properties = args.properties.replace(":", "\":\"")
        properties = properties.replace(",", "\",\"")
        properties = "{{\"{0}\"}}".format( properties )
        properties = json.loads( properties )

    agent_id = None
    if args.agent_name:
        agent_id = teamcity.agent_info_by_name( args.agent_name )['id']

    run_build_result = teamcity.run_build( args.configuration_name, args.branch, properties, triggering_options, agent_id )

    print run_build_result

if __name__ == '__main__':
    main()
