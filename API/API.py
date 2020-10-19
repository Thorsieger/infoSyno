#!/usr/bin/env python
# encoding: utf-8
import json
from flask import Flask
from flask_restful import Api, Resource, reqparse

app = Flask(__name__)
api = Api(app)

parser = reqparse.RequestParser()

NAScount = 4

#load from database
NASList = {
  '1' : {'name': '1U','type' : 'RS819' , 'addr': 1, 'state': 'on'},
  '2' : {'name': '2U','type' : 'RS217', 'addr': 2, 'state': 'rebooting'},
  '3' : {'name': '3U','type' : 'RS820+', 'addr': 3, 'state': 'on'},
  '4' : {'name': '4U','type' : 'RS820RP+', 'addr': 4, 'state': 'on'},
}


class state(Resource):
    def get(self):
        return ({'rack' : 'c5', 'state' : 'online'}), 200

class NASs(Resource):
    def get(self):
        naslist = []
        for x in range(1,NAScount+1):
            naslist.append(NAS.get(self,str(x)))
        return {'rack' : 'c5','NAScount' : NAScount, 'NAS' : naslist}

class NAS(Resource):
    def get(self,NAS_id):
        if NAS_id not in NASList:
            return 'NOT found', 404
        else:
            #appel I2C pour obtenir l'état : NASList[NAS_id].addr + ajout static data (I2C addr)
            return NASList[NAS_id]

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
        #sauvegarde en database
        return NASList[NAS_id], 201

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

    def delete(self, NAS_id):
        if NAS_id not in NASList:
            return 'NOT found', 404
        else:
            del NASList[NAS_id]
            #update database
            return '', 204


api.add_resource(state,'/')
api.add_resource(NASs,'/NASs', '/NASs/')
api.add_resource(NAS, '/NASs/<NAS_id>', '/NASs/<NAS_id>/')

if __name__ == '__main__':
  app.run(debug=True)