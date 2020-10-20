#!/usr/bin/env python
# encoding: utf-8
import json
from flask import Flask, request, abort
from flask_restful import Api, Resource, reqparse
from functools import wraps

app = Flask(__name__)
api = Api(app)
parser = reqparse.RequestParser()

#Sécurisation par token
def require_appkey(view_function):
    @wraps(view_function)
    def decorated_function(*args, **kwargs):
        #with open('api.key', 'r') as apikey:
        #    key=apikey.read().replace('\n', '')
        if request.headers.get('key') and request.headers.get('key') == "key":
        #if request.headers.get('x-api-key') and request.headers.get('x-api-key') == key:
            return view_function(*args, **kwargs)
        else:
            abort(401)
    return decorated_function

#load from database
NASList = {
  '1' : {'name': '1U','type' : 'RS819' , 'addr': 1, 'state': 'on'},
  '2' : {'name': '2U','type' : 'RS217', 'addr': 2, 'state': 'on'},
  '3' : {'name': '3U','type' : 'RS820+', 'addr': 3, 'state': 'on'},
  '4' : {'name': '4U','type' : 'RS820RP+', 'addr': 4, 'state': 'on'},
}
NAScount = 4

#definition des endpoints
class state(Resource):
    @require_appkey
    def get(self):
        return ({'rack' : 'c5', 'state' : 'online'}), 200

class NASs(Resource):
    @require_appkey
    def get(self):
        naslist = []
        for x in NASList:
            print(x)
            naslist.append(NAS.get(self,x))
        return {'rack' : 'c5','NAScount' : NAScount, 'NAS' : naslist}

class NAS(Resource):
    @require_appkey
    def get(self,NAS_id):
        if NAS_id not in NASList:
            return 'NOT found', 404
        else:
            #appel I2C pour obtenir l'état : NASList[NAS_id].addr + ajout static data (I2C addr)
            return NASList[NAS_id]

    @require_appkey
    def post(self,NAS_id):
        parser.add_argument('name')
        parser.add_argument('addr')
        parser.add_argument('type')
        args = parser.parse_args()
        if NAS_id in NASList :
            return 'FORBIDDEN - ID EXIST', 403
        for nas in NASList :
            if NASList[nas]['addr'] == int(args['addr']):
                return 'FORBIDDEN - I2C ALREADY USE', 403
        NASList[NAS_id] = {
            'name': args['name'],
            'type': args['type'],
            'addr': int(args['addr']),
            'state': 'on',
        }
        global NAScount
        NAScount+=1
        #sauvegarde en database
        return NASList[NAS_id], 201

    @require_appkey
    def patch(self, NAS_id):
        parser.add_argument('name')
        parser.add_argument('addr')
        parser.add_argument('type')
        args = parser.parse_args()
        if NAS_id not in NASList:
            return 'NAS not found', 404
        else:
            if args['addr'] is not None :
                for nas in NASList :
                    if NASList[nas]['addr'] == int(args['addr']):
                        return 'FORBIDDEN - I2C ALREADY USE', 403
            nas = NASList[NAS_id]
            nas['name'] = args['name'] if args['name'] is not None else nas['name']
            nas['addr'] = args['addr'] if args['addr'] is not None else nas['addr']
            nas['type'] = args['type'] if args['type'] is not None else nas['type']
            #update database
            return nas, 200

    @require_appkey
    def delete(self, NAS_id):
        if NAS_id not in NASList:
            return 'NOT found', 404
        else:
            del NASList[NAS_id]
            global NAScount
            NAScount-=1
            #update database
            return '', 204

class softreset(Resource):
    @require_appkey
    def post(self,NAS_id):
        if NAS_id not in NASList:
            return 'NOT found', 404
        else:
            if NASList[NAS_id]['state'] is not 'on':
                return 'CONFLICT',409
            NASList[NAS_id]['state'] = "softreseting"
            #todo : envoie ordre
            return NASList[NAS_id],202

class hardreset(Resource):
    @require_appkey
    def post(self,NAS_id):
        if NAS_id not in NASList:
            return 'NOT found', 404
        else:
            if NASList[NAS_id]['state'] is not 'on':
                return 'CONFLICT',409
            NASList[NAS_id]['state'] = "hardreseting"
            #todo : envoie ordre
            return NASList[NAS_id],202

class reboot(Resource):
    @require_appkey
    def post(self,NAS_id):
        if NAS_id not in NASList:
            return 'NOT found', 404
        else:
            if NASList[NAS_id]['state'] is not 'on':
                return 'CONFLICT',409
            NASList[NAS_id]['state'] = "rebooting"
            #todo : envoie ordre
            return NASList[NAS_id],202

#Ajout des endpoints à l'API
api.add_resource(state,'/')
api.add_resource(NASs,'/NASs', '/NASs/')
api.add_resource(NAS, '/NASs/<NAS_id>', '/NASs/<NAS_id>/')
api.add_resource(softreset, '/NASs/<NAS_id>/softreset','/NASs/<NAS_id>/softreset/')
api.add_resource(hardreset, '/NASs/<NAS_id>/hardreset','/NASs/<NAS_id>/hardreset/')
api.add_resource(reboot, '/NASs/<NAS_id>/reboot','/NASs/<NAS_id>/reboot/')

if __name__ == '__main__':
  app.run(host='127.0.0.1',debug=True, ssl_context='adhoc')

#import ssl
#context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
#context.load_cert_chain("server.crt", "server.key")
#sl_context=context