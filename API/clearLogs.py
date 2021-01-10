from pymongo import MongoClient, errors
import datetime

client = MongoClient("mongodb://127.0.0.1:27018", serverSelectionTimeoutMS=5000)
db = client.mymongodb

datelimite = datetime.datetime.now() - datetime.timedelta(days=7)#date de la semaine dernière
d = db.logs.delete_many({'date': { '$lt' :str(datelimite)}})#suppression des logs plus vieux que cette date