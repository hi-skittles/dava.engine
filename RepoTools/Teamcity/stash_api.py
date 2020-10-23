
import sys
import requests
import json

__Stash = None

class StashRequest:
    def __init__(self, stash_url,stash_api_version, stash_project, stash_repo_name, login, password ):

        self.__headers      = {'Content-Type': 'application/json'}
        self.__session      = requests.Session()
        self.__session.auth = (login, password)
        self.__base_url     = ''.join((stash_url, '/rest/api/{}/projects/{}/repos/{}/'.format( stash_api_version, stash_project, stash_repo_name ) ) )
        self.__commits_url  = stash_url + '/rest/build-status/1.0/commits/'

    def __request(self, url, data=None):

        try:
            url = ''.join((self.__base_url, url))
            request_method = 'GET'
            if data:
                request_method = 'POST'
            response = self.__session.request(request_method, url, headers=self.__headers, data = data )
            response.raise_for_status()
            return response

        except:
            print "Unexpected error:", sys.exc_info()[0]
            raise

    def get_pull_requests_info(self, pull_requests ):
        response = self.__request("pull-requests/{}".format(pull_requests))
        return json.loads( response.content )

    def get_pull_requests_changes(self, pull_requests ):
        response = self.__request("pull-requests/{}/changes?limit=99999".format(pull_requests))
        return json.loads( response.content )

    def get_commits(self, pull_requests ):
        response = self.__request("pull-requests/{}/commits".format(pull_requests))
        return json.loads( response.content )

    def report_build_status( self, state, key, name, url, commit_id, description = "" ):

        build_status_dict = {
            "state": state,
            "key": key,
            "name": name,
            "url": url,
            "description": description
        }

        try:
            commits_url = self.__commits_url + commit_id
            response = requests.post( commits_url, params=None, auth=self.__session.auth,
                                     headers={'Content-Type': 'application/json'}, data=json.dumps( build_status_dict ) )

        except Exception as e:
            print "Unexpected error:", e
            raise


def argparse_add_argument( arg_parser ):
    arg_parser.add_argument( '--stash_api_version', default = '1.0' )
    arg_parser.add_argument( '--stash_project', default = 'DF' )
    arg_parser.add_argument( '--stash_repo_name', default = 'dava.framework' )

    arg_parser.add_argument( '--stash_url', required = True )

    arg_parser.add_argument( '--stash_login', required = True )
    arg_parser.add_argument( '--stash_password', required = True )


def init( stash_url,stash_api_version, stash_project, stash_repo_name, login, password ):
    global __Stash
    __Stash = StashRequest( stash_url,
                            stash_api_version,
                            stash_project,
                            stash_repo_name,
                            login,
                            password )
    return __Stash

def init_args( args ):
    global __Stash
    __Stash = StashRequest( args.stash_url,
                            args.stash_api_version,
                            args.stash_project,
                            args.stash_repo_name,
                            args.stash_login,
                            args.stash_password )
    return __Stash

def ptr():
    return __Stash