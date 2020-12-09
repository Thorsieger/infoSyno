#include "SynoCommand.h"
#include "IPC.h"

#include <stdio.h>
#include <stdlib.h>

//fonction d'initialisation qui (sur tous les ttyUSB) se connecte puis va chercher les @MAC (call api endpoint pour stockage)
//connexion("tty");
//getNasId(fd)

//demandé au lancement de l'api
//une biblio avec les fonctions de base + des programmes pour les fonctions haut niveau (appel séparé) <= a voir si nécessaire/utile

int main(void){
    int msgid = createIPCm_serveur(CLE_MSG);
    deleteIPCm_serveur(msgid);
    int test1 = connexion("/dev/ttyUSB0");

    char macAddr[15] = {0};
    char type[50] = {0};

    nasId("/dev/ttyUSB0",macAddr);
    nasType("/dev/ttyUSB0",type);

    printf("Mac : %s\nType : %s\n",macAddr,type);

    int test2 = 0;
    //test2 = reboot("/dev/ttyUSB0");
    
    printf("connexion ? : %d \n reboot ? : %d\n", test1,test2);
    return 0;
}