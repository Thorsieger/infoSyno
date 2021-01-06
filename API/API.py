#!/usr/bin/env python
# encoding: utf-8
from gevent import pywsgi
from gevent import monkey; monkey.patch_all()
from datetime import datetime
from flask import Flask, request, abort
from flask_restful import Api, Resource, reqparse
from functools import wraps
from pymongo import MongoClient
import sysv_ipc
import logging
import logging.handlers
import tempfile
import json
import ssl
import time

rackName = 'c5'
IPC_Key = 100

app = Flask(__name__)
api = Api(app)
parser = reqparse.RequestParser()

# Database connexion
client = MongoClient("mongodb://127.0.0.1:27018")
db = client.mymongodb

# Rsyslog

logger = logging.getLogger('SynoApiLogger_'+rackName)
logger.setLevel(logging.DEBUG)

def rsyslog(type,texte,ID,result):
    
    #handler = logging.handlers.SysLogHandler(address = ('0.0.0.0',514), facility="local3")
    handler = logging.handlers.SysLogHandler(address = ('0.0.0.0',10514), facility="local3", socktype=logging.handlers.socket.SOCK_STREAM)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(message)s')

    handler.setFormatter(formatter)
    logger.addHandler(handler)
    
    data = texte + " - id=" + str(ID) + " : " + result

    if type == "info":
        logger.info(data)
    elif type == "warn":
        logger.warning(data)
    elif type == "error":
        logger.error(data)

    logger.removeHandler(handler)

    db.logs.insert_one({'date': str(datetime.now()),'command': texte,'NAS': ID, 'result': result})

#IPC_Message
def sendCommand(NAS_id,command):
    fp  = tempfile.NamedTemporaryFile(mode = "w+t")
    mac = db.rack.find_one({"_id":NAS_id})['addr']
    message = command+ ";" + mac + ";" + fp.name + ";"
    try:
        sysv_ipc.MessageQueue(IPC_Key).send(message)
    except:
        return "error can't send order"

    date = time.time()
    while time.time()<date+5: #timeout 5s
        fp.seek(0)
        response = fp.read()
        if response:
            break
        response = "timeout"
    fp.close()
    return response


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
        rsyslog("info","ping",0,"online")
        return ({'rack' : rackName, 'state' : 'online'}), 200

class allNAS(Resource):
    @require_appkey
    def get(self):
        naslist = []
        rsyslog("info","allNAS",0,"ok")
        for x in db.rack.distinct('_id'):
            naslist.append(NAS.get(self,x))
        return {'rack' : rackName,'NAScount' : db.rack.count_documents({}), 'NAS' : naslist}

class NAS(Resource):
    @require_appkey
    def get(self,NAS_id):
        
        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None :
            rsyslog("warn","getNAS",NAS_id,"notfound")
            return 'NOT found', 404
        else:
            nas_data['state'] = sendCommand(NAS_id,"state")
            rsyslog("info","getNAS",NAS_id,nas_data['state'])
            return nas_data

    @require_appkey
    def post(self,NAS_id):
        parser.add_argument('name')
        parser.add_argument('addr')
        args = parser.parse_args()

        if db.rack.find_one({"_id":NAS_id}) is not None:
            rsyslog("error","postNAS",NAS_id,"alreadyexist")           
            return 'FORBIDDEN - ID EXIST', 403
        if db.rack.find_one({"addr":args['addr']}) is not None:
            rsyslog("error","postNAS",NAS_id,"MAC_used")      
            return 'FORBIDDEN - MAC ALREADY USE', 403
        newNAS = {'_id':NAS_id, 'name': args['name'],'addr': args['addr']}
        #sauvegarde en database
        db.rack.insert_one(newNAS)
        newNAS['state'] = 'add'
        rsyslog("info","postNAS",NAS_id,"ok")  
        return newNAS, 201

    @require_appkey
    def patch(self, NAS_id):
        parser.add_argument('name')
        parser.add_argument('addr')
        args = parser.parse_args()

        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None:
            rsyslog("warn","patchNAS",NAS_id,"notfound")
            return 'NAS not found', 404
        else:
            if args['addr'] is not None :
                if db.rack.find_one({"addr":args['addr']}) == args['addr']:
                    rsyslog("error","patchNAS",NAS_id,"MAC_used")     
                    return 'FORBIDDEN - MAC ALREADY USE', 403
            nas_data['name'] = args['name'] if args['name'] is not None else nas_data['name']
            nas_data['addr'] = args['addr'] if args['addr'] is not None else nas_data['addr']
            #update database
            db.rack.update({'_id':NAS_id}, {'name': nas_data['name'],'addr': nas_data['addr']})
            nas_data['state'] = 'updated'
            rsyslog("info","patchNAS",NAS_id,"ok")
            return nas_data, 200

    @require_appkey
    def delete(self, NAS_id):
        if db.rack.find_one({"_id":NAS_id}) is None:
            rsyslog("warn","deleteNAS",NAS_id,"notfound")
            return 'NOT found', 404
        else:
            #update database
            db.rack.delete_one({'_id':NAS_id})
            rsyslog("info","deleteNAS",NAS_id,"ok")
            return '', 204

class softreset(Resource):
    @require_appkey
    def post(self,NAS_id):
        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None:
            rsyslog("warn","softreset",NAS_id,"notfound")
            return 'NAS not found', 404
        else:
            nas_data['state'] = sendCommand(NAS_id,"softreset")
            rsyslog("info","softreset",NAS_id,nas_data['state'])
            return nas_data,202

class hardreset(Resource):
    @require_appkey
    def post(self,NAS_id):
        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None:
            rsyslog("warn","hardreset",NAS_id,"notfound")
            return 'NAS not found', 404
        else:
            nas_data['state'] = sendCommand(NAS_id,"hardreset")
            rsyslog("info","hardreset",NAS_id,nas_data['state'])
            return nas_data,202

class reboot(Resource):
    @require_appkey
    def post(self,NAS_id):
        nas_data = db.rack.find_one({"_id":NAS_id})
        if nas_data is None:
            rsyslog("warn","reboot",NAS_id,"notfound")
            return 'NAS not found', 404
        else:
            nas_data['state'] = sendCommand(NAS_id,"reboot")
            rsyslog("info","reboot",NAS_id,nas_data['state'])
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
            #rsyslog("warn","logs",NAS_id,"notfound")
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
            #rsyslog("warn","log",NAS_id,"notfound")
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