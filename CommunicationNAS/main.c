#include "SynoCommand.h"
#include "IPC.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_NAS 45
#define TAILLE_MAC 15

int msgid;
struct sigaction action;

void hand(int sig) {
	if (sig==SIGINT) {
		msgctl(msgid, IPC_RMID, 0);
		exit(0);
	}
}

char AllMacAddress[NUM_NAS][TAILLE_MAC] = {0};

void *getTTY(void *threadid){
    char address[14] = "/dev/ttyUSB";
    char mac[TAILLE_MAC] = {0};
    char buf[3] = {0};

    long id = (long)threadid;
    sprintf(buf,"%ld",id);
    strcat(address,buf);

    int co = connexion(address);

    if(co<=0) pthread_exit(NULL);
    nasId(address,mac);
    strcpy(AllMacAddress[id],mac);
    pthread_exit(NULL);
}

void repondreAPI(char namedpipe[50], char* message){
    int fd = open(namedpipe, O_WRONLY);
    write(fd, message, sizeof(message));
    close(fd);
}

void *gestionAppel(void *info){

    //récupération infos
    char command[50] = {0};
    char macAddr[TAILLE_MAC] = {0};
    char namedpipe[50] = {0};
    strcpy(command,strtok((char*)info,";"));
    strcpy(macAddr,strtok(NULL,";"));
    strcpy(namedpipe,strtok(NULL,";"));
    free(info);

    //get the tty from @Mac
    char address[14] = "/dev/ttyUSB";
    int cpt = 0;
    char buf[3];

    while(cpt<45 && strcmp(macAddr,AllMacAddress[cpt]))cpt++;
    if(cpt==45){repondreAPI(namedpipe,"notfound");pthread_exit(NULL);}

    sprintf(buf,"%d",cpt);
    strcat(address,buf);

    //execute command
    if(strcmp(command,"info")==0){

        char type[50] = {0};
        nasType(address,type);
        repondreAPI(namedpipe,type);

    }else if(strcmp(command,"reboot")==0){

        int result = reboot(address);
        if(result)repondreAPI(namedpipe,"rebooting");
        else repondreAPI(namedpipe,"error rebooting");
        connexion(address);

    }else if(strcmp(command,"softreset")==0){

        int result = softreset(address);
        if(result)repondreAPI(namedpipe,"softreset ok");
        else repondreAPI(namedpipe,"error softreset");

    }else if(strcmp(command,"hardreset")==0){

        int result = hardReset(address);
        if(result)repondreAPI(namedpipe,"hardreset ok");
        else repondreAPI(namedpipe,"error rebooting");
        connexion(address);
    }
    
    pthread_exit(NULL);
}

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
    
    //gestion appel API
    msgid = createIPCm_serveur(CLE_MSG);
    while(1){
        char* messageIPC = malloc(IPC_TAILLE_MSG * sizeof(char));
        pthread_t thread_gestion;

        rcvIPCm(msgid, messageIPC, 1);
        pthread_create(&thread_gestion, NULL,gestionAppel,messageIPC);
    }
        
    return 0;
}