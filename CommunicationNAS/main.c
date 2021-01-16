#include "SynoCommand.h"
#include "IPC.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_NAS 45
#define TAILLE_MAC 15

int msgid;
struct sigaction action;

void hand() {
	msgctl(msgid, IPC_RMID, 0);
	exit(0);
}

char AllMacAddress[NUM_NAS][TAILLE_MAC] = {0};
int nasState[NUM_NAS] = {0};

void *getTTY(void *threadid){
    char address[14] = "/dev/ttyUSB";
    char mac[TAILLE_MAC] = {0};
    char buf[3] = {0};

    long id = (long)threadid;
    sprintf(buf,"%ld",id);
    strcat(address,buf);

    int co=0;
    do
    {
        co = connexion(address);
    } while (!co);
    
    nasId(address,mac);
    strcpy(AllMacAddress[id],mac);
    printf("Connected to NAS id: %ld\n",id);
    pthread_exit(NULL);
}

void repondreAPI(char namedpipe[50], char* message){
    int fd = open(namedpipe, O_WRONLY);
    write(fd, message, strlen(message));
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

    if(nasState[cpt]==1){
        repondreAPI(namedpipe,"NAS is rebooting");
        pthread_exit(NULL);
    }
    else if (nasState[cpt]==2)
    {
        repondreAPI(namedpipe,"Error connexion to NAS : trying for more than 30 mins : retrying");
        nasState[cpt] = 3;
        int result = connexion(address);
        if(result==-1)nasState[cpt] = 2;
        else nasState[cpt] = 0;
        pthread_exit(NULL);
    }else if(nasState[cpt]==3){
        repondreAPI(namedpipe,"Trying to reconnect");
        pthread_exit(NULL);
    }
    
    //execute command
    /*if(strcmp(command,"info")==0){

        char type[50] = {0};
        nasType(address,type);
        repondreAPI(namedpipe,type);

    }*/
    if(strcmp(command,"state")==0){

        if(available(address)>0)repondreAPI(namedpipe,"NAS available");
        else repondreAPI(namedpipe,"NAS not available");
        
    }else if(strcmp(command,"reboot")==0){

        if(reboot(address)){
            repondreAPI(namedpipe,"rebooting");
            nasState[cpt] = 1;
            sleep(60);//wait for reboot
            int result = connexion(address);
            if(result==-1)nasState[cpt] = 2;
            else nasState[cpt] = 0;
        }else{repondreAPI(namedpipe,"error rebooting");}

    }else if(strcmp(command,"softreset")==0){

        if(softreset(address))repondreAPI(namedpipe,"softreset ok");
        else repondreAPI(namedpipe,"error softreset");

    }else if(strcmp(command,"hardreset")==0){

        if(hardReset(address)){
            repondreAPI(namedpipe,"hardreset ok");
            nasState[cpt] = 1;
            sleep(60);//wait for reboot
            int result = connexion(address);
            if(result==-1)nasState[cpt] = 2;
            else nasState[cpt] = 0;
        }else{repondreAPI(namedpipe,"error hardreset");}
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
        printf("message received : %s\n",messageIPC);
        pthread_create(&thread_gestion, NULL,gestionAppel,messageIPC);
    }
        
    return 0;
}