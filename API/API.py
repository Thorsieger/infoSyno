#!/usr/bin/env python
# encoding: utf-8
import json
import ssl
import logging
import logging.handlers
from datetime import datetime
from flask import Flask, request, abort
from flask_restful import Api, Resource, reqparse
from functools import wraps
from pymongo import MongoClient
from gevent import pywsgi
from gevent import monkey; monkey.patch_all()
import time

rackName = 'c5'

app = Flask(__name__)
api = Api(app)
parser = reqparse.RequestParser()

# Database connexion
client = MongoClient("mongodb://127.0.0.1:27018")
db = client.mymongodb

# Rsyslog

logger = logging.getLogger('SynoApiLogger')
logger.setLevel(logging.DEBUG)

def rsyslog(type,texte):
    
    #handler = logging.handlers.SysLogHandler(address = ('0.0.0.0',514), facility="local3")
    handler = logging.handlers.SysLogHandler(address = ('0.0.0.0',10514), facility="local3", socktype=logging.handlers.socket.SOCK_STREAM)
    logger.addHandler(handler)

    if type == "info":
        logger.info(texte)
        print(texte)
    elif type == "warn":
        logger.warning(texte)
    elif type == "error":
        logger.error(texte)
    handler.close()
    

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

#definition des endpoints
class state(Resource):
    @require_appkey
    def get(self):
        rsyslog("info","petit ping")
        db.logs.insert_one({'date': str(datetime.now()),'command': 'ping' ,'NAS': 0, 'result': 'ok'})
        return ({'rack' : rackName, 'state' : 'online'}), 200

class allNAS(Resource):
    @require_appkey
    def get(self):
        naslist = []
        rsyslog("info","allNAS")
        db.logs.insert_one({'date': str(datetime.now()),'command': 'getAll' ,'NAS': 0, 'result': 'ok'})
        for x in db.rack.distinct('_id'):
            naslist.append(NAS.get(self,x))
        return {'rack' : rackName,'NAScount' : db.rack.count_documents({}), 'NAS' : naslist}

class NAS(Resource):
    @require_appkey
    def get(self,NAS_id):
        rsyslog("info","oneNAS"+NAS_id)
        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None :
            db.logs.insert_one({'date': str(datetime.now()),'command': 'getNas' ,'NAS': NAS_id, 'result': 'notfound'})
            return 'NOT found', 404
        else:
            #appel I2C pour obtenir l'état : NASList[NAS_id].addr + ajout static data (I2C addr)
            nas_data['state'] = 'on' #en fait non il faut get l'etat
            db.logs.insert_one({'date': str(datetime.now()),'command': 'getNAS' ,'NAS': NAS_id, 'result': 'on'}) #state
            return nas_data

    @require_appkey
    def post(self,NAS_id):
        parser.add_argument('name')
        parser.add_argument('addr')
        parser.add_argument('type')
        args = parser.parse_args()

        if db.rack.find_one({"_id":NAS_id}) is not None:
            db.logs.insert_one({'date': str(datetime.now()),'command': 'postNas' ,'NAS': NAS_id, 'result': 'alreadyexist'})           
            return 'FORBIDDEN - ID EXIST', 403
        if db.rack.find_one({"addr":int(args['addr'])}) is not None:
            db.logs.insert_one({'date': str(datetime.now()),'command': 'postNas' ,'NAS': NAS_id, 'result': 'I2C_used'})           
            return 'FORBIDDEN - I2C ALREADY USE', 403
        newNAS = {'_id':NAS_id, 'name': args['name'],'type': args['type'],'addr': int(args['addr'])}
        #sauvegarde en database
        db.rack.insert_one(newNAS)
        newNAS['state'] = 'undefined'
        db.logs.insert_one({'date': str(datetime.now()),'command': 'postNas' ,'NAS': NAS_id, 'result': 'ok'})           
        return newNAS, 201

    @require_appkey
    def patch(self, NAS_id):
        parser.add_argument('name')
        parser.add_argument('addr')
        parser.add_argument('type')
        args = parser.parse_args()

        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None:
            db.logs.insert_one({'date': str(datetime.now()),'command': 'patchNas' ,'NAS': NAS_id, 'result': 'notfound'})           
            return 'NAS not found', 404
        else:
            if args['addr'] is not None :
                if db.rack.find_one({"addr":int(args['addr'])}) == int(args['addr']):
                    db.logs.insert_one({'date': str(datetime.now()),'command': 'patchNas' ,'NAS': NAS_id, 'result': 'I2C_used'})           
                    return 'FORBIDDEN - I2C ALREADY USE', 403
            nas_data['name'] = args['name'] if args['name'] is not None else nas_data['name']
            nas_data['addr'] = args['addr'] if args['addr'] is not None else nas_data['addr']
            nas_data['type'] = args['type'] if args['type'] is not None else nas_data['type']
            #update database
            db.rack.update({'_id':NAS_id}, {'name': nas_data['name'],'type': nas_data['type'],'addr': int(nas_data['addr'])})
            nas_data['state'] = 'undefined'
            db.logs.insert_one({'date': str(datetime.now()),'command': 'patchNas' ,'NAS': NAS_id, 'result': 'ok'})
            return nas_data, 200

    @require_appkey
    def delete(self, NAS_id):
        if db.rack.find_one({"_id":NAS_id}) is None:
            db.logs.insert_one({'date': str(datetime.now()),'command': 'deleteNas' ,'NAS': NAS_id, 'result': 'notfound'})
            return 'NOT found', 404
        else:
            #update database
            db.rack.delete_one({'_id':NAS_id})
            db.logs.insert_one({'date': str(datetime.now()),'command': 'deleteNas' ,'NAS': NAS_id, 'result': 'ok'})
            return '', 204

class softreset(Resource):
    @require_appkey
    def post(self,NAS_id):
        nas_data = rack.find_one({"_id":NAS_id})
        if nas_data is None:
            db.logs.insert_one({'date': str(datetime.now()),'command': 'softreset' ,'NAS': NAS_id, 'result': 'notfound'})
            return 'NAS not found', 404
        else:
            #if ['state'] != 'on':
            #    return 'CONFLICT',409
            nas_data['state'] = "softreseting"
            #todo : envoie ordre
            db.logs.insert_one({'date': str(datetime.now()),'command': 'softreset' ,'NAS': NAS_id, 'result': 'ok'})
            return nas_data,202

class hardreset(Resource):
    @require_appkey
    def post(self,NAS_id):
        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None:
            db.logs.insert_one({'date': str(datetime.now()),'command': 'hardreset' ,'NAS': NAS_id, 'result': 'notfound'})
            return 'NAS not found', 404
        else:
            #if ['state'] != 'on':
            #    return 'CONFLICT',409
            nas_data['state'] = "hardreseting"
            #todo : envoie ordre
            db.logs.insert_one({'date': str(datetime.now()),'command': 'hardreset' ,'NAS': NAS_id, 'result': 'ok'})
            return nas_data,202

class reboot(Resource):
    @require_appkey
    def post(self,NAS_id):
        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None:
            db.logs.insert_one({'date': str(datetime.now()),'command': 'reboot' ,'NAS': NAS_id, 'result': 'notfound'})
            return 'NAS not found', 404
        else:
            #if ['state'] != 'on':
            #    return 'CONFLICT',409
            nas_data['state'] = "rebooting"
            #todo : envoie ordre
            db.logs.insert_one({'date': str(datetime.now()),'command': 'reboot' ,'NAS': NAS_id, 'result': 'ok'})
            return nas_data,202

#logs
class logs(Resource):
    @require_appkey
    def get(self,NAS_id):
        nas_logs = db.logs.find({"NAS":NAS_id}).sort('date',-1)
        logNAS =[]
        for x in nas_logs:
            x.pop('_id')
            logNAS.append(x)
        if not logNAS:
            #logs.insert_one({'date': str(datetime.now()),'command': 'log' ,'NAS': NAS_id, 'result': 'notfound'})
            return 'no data', 204
        return logNAS, 200

    @require_appkey
    def delete(self,NAS_id):
        if db.logs.delete_many({"NAS":NAS_id}).deleted_count == 0:
            return 'no data', 204
        return '', 204

class alllogs(Resource):
    @require_appkey
    def get(self):
        nas_logs = db.logs.find().sort([('NAS',1),('date',-1)])
        logNAS =[]
        for x in nas_logs:
            x.pop('_id')
            logNAS.append(x)
        if not logNAS:
            #logs.insert_one({'date': str(datetime.now()),'command': 'log' ,'NAS': NAS_id, 'result': 'notfound'})
            return 'no data', 204
        return logNAS, 200
    def delete(self):
        if db.logs.delete_many({}).deleted_count == 0:
            return 'no data', 204
        return '', 204


#Ajout des endpoints à l'API
api.add_resource(state,'/')
api.add_resource(allNAS,'/NASs', '/NASs/')
api.add_resource(NAS, '/NASs/<NAS_id>', '/NASs/<NAS_id>/')
api.add_resource(softreset, '/NASs/<NAS_id>/softreset','/NASs/<NAS_id>/softreset/')
api.add_resource(hardreset, '/NASs/<NAS_id>/hardreset','/NASs/<NAS_id>/hardreset/')
api.add_resource(reboot, '/NASs/<NAS_id>/reboot','/NASs/<NAS_id>/reboot/')
api.add_resource(logs,'/NASs/<NAS_id>/logs','/NASs/<NAS_id>/logs/')
api.add_resource(alllogs,'/NASs/logs','/NASs/logs/')

if __name__ == '__main__':
    #server = pywsgi.WSGIServer(('127.0.0.1',5000),app, keyfile='server.key', certfile='server.crt')
    #server.serve_forever()
    app.run(host='127.0.0.1',debug=True, ssl_context='adhoc')