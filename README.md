# InfoSyno

**InfoSyno** est un projet de monitoring de NAS Synology en Datacenter à travers une API


### Contenu
A terme, sur ce repo, vous pourrez trouver le code de contrôle de l'API ainsi que les micro-code de récupération des données sur les NAS

### Pour faire fonctionne le code 

Python 3 + les bibliothèques python : flask, flask_restful, functools, gevent et pymongo
Il faut également lancer mongodb en local : mongod --dbpath ./Documents/.../data/ --port 27018

Il est possible de tester les endpoints grâce à postman (en installant postman agent).

Pour le code C, il faut le compiler avec la commande :
gcc IPC.c SynoCommand.c main.c -o syno_test -lpthread -Wall -Wextra