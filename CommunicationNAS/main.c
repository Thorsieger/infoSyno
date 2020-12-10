#include "SynoCommand.h"
#include "IPC.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_NAS 45

int msgid;
struct sigaction action;

void hand(int sig) {
	if (sig==SIGINT) {
		msgctl(msgid, IPC_RMID, 0);
		exit(0);
	}
}

char AllMacAddress[46][15] = {0};

void *getTTY(void *threadid){
    char address[14] = "/dev/ttyUSB";
    char buf[3];
    long id;
    id = (long)threadid;
    sprintf(buf,"%ld",id);
    strcat(address,buf);

    int co = connexion(address);

    char mac[15] = {0};
    if(co<=0) pthread_exit(NULL);
    nasId(address,mac);
    strcpy(AllMacAddress[id],mac);
    pthread_exit(NULL);
}

//fonction d'initialisation qui (sur tous les ttyUSB) se connecte puis va chercher les @MAC (call api endpoint pour stockage)
//connexion("tty");
//getNasId(fd)

//demandé au lancement de l'api
//une biblio avec les fonctions de base + des programmes pour les fonctions haut niveau (appel séparé) <= a voir si nécessaire/utile

int main(void){
    //Pour quitter proprement
    action.sa_handler = hand;
	sigaction(SIGINT, &action, NULL);

    //Pour récupérer tout les liens NAS <-> tty
    pthread_t threads[NUM_NAS];
    for (long i = 0; i < NUM_NAS; i++)
    {
        pthread_create(&threads[i], NULL,getTTY,(void *)i);
    }
    
    msgid = createIPCm_serveur(CLE_MSG);
    while(1){
        char messageIPC[IPC_TAILLE_MSG] = {0};
        rcvIPCm(msgid, messageIPC, 1);
        
        char command[50] = {0};
        char macAddr[15] = {0};
        char pip[5] = {0};
        strcpy(command,strtok(messageIPC,";"));
        strcpy(macAddr,strtok(NULL,";"));
        strcpy(pip,strtok(NULL,";"));

        //if(strcmp).... pour les commandes
        //create thread ou child

        char address[14] = "/dev/ttyUSB";
        int cpt = 0;
        char buf[3];

        while(cpt<45 && strcmp(macAddr,AllMacAddress[cpt]))cpt++;

        sprintf(buf,"%d",cpt);
        strcat(address,buf);

        char type[50] = {0};
        if(strcmp(command,"reboot")==0){
            nasType(address,type);
            printf("%s\n",type);

        }

    }

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