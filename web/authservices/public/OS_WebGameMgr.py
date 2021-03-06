from cgi import parse_qs, escape
import xml.etree.ElementTree as ET

from collections import OrderedDict
import http.client

from BaseService import BaseService

import redis

import simplejson as json

from lib.Exceptions.OS_BaseException import OS_BaseException
from lib.Exceptions.OS_CommonExceptions import *

class OS_WebGameMgr(BaseService):
    def test_required_params(self, input, params):
        for param in params:
            if param not in input:
                raise OS_MissingParam("param")

        return True

    def test_user_session(self, session_key, userid):
        send_data = {'session_key': session_key, 'userid': userid, 'mode': 'test_session'}
        params = json.dumps(send_data)
        
        headers = {
            "Content-type": "application/json",
            "Accept": "text/plain",
            "APIKey": self.BACKEND_PRIVATE_APIKEY
        }
        conn = http.client.HTTPConnection(self.LOGIN_SERVER)

        conn.request("POST", self.LOGIN_SCRIPT, params, headers)
        response = json.loads(conn.getresponse().read())

        return response['valid'] and "admin" in response and response['admin']
    def process_request(self, login_options):
        required_params = ['userid', 'session_key']
        if not self.test_required_params(login_options, required_params):
            raise OS_MissingParam("required params")
        if not self.test_user_session(login_options["session_key"], login_options["userid"]):            
            raise OS_Auth_InvalidCredentials()


        headers = {
            "Content-type": "application/json",
            "Accept": "text/plain",
            "APIKey": self.BACKEND_PRIVATE_APIKEY
        }
        conn = http.client.HTTPConnection(self.GAME_MGR_SERVER)

        conn.request("POST", self.GAME_MGR_SCRIPT, json.dumps(login_options), headers)
        response = json.loads(conn.getresponse().read())
        return response
    def run(self, env, start_response):
        # the environment variable CONTENT_LENGTH may be empty or missing
        try:
            request_body_size = int(env.get('CONTENT_LENGTH', 0))
        except (ValueError):
            request_body_size = 0

        # When the method is POST the variable will be sent
        # in the HTTP request body which is passed by the WSGI server
        # in the file like wsgi.input environment variable.
        request_body = json.loads(env['wsgi.input'].read(request_body_size))
       # d = parse_qs(request_body)

        try:
            response = self.process_request(request_body)
        except OS_BaseException as e:
            response = e.to_dict()
        except Exception as error:
            response = {"error": repr(error)}

        #if 'error' in response:
        #   start_response('400 BAD REQUEST', [('Content-Type','application/json')])
        # else:
        start_response('200 OK', [('Content-Type','application/json')])

        return response
