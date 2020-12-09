#include "IPC.h"

int createIPCm_serveur(key_t cle_msg) {
    int msgid;
	if ((msgid = msgget(cle_msg, IPC_CREAT|IPC_EXCL|0666)) == -1) {
		perror("error on createIPCm_serveur\n"); exit(1);
	}
	return msgid;
}

int joinIPCm_serveur(key_t cle_msg) {
    int msgid;
	if ((msgid = msgget(cle_msg, 0)) == -1) {
		perror("error on joinIPCm_serveur\n"); exit(1);
	}
	return msgid;
}

void sendIPCm(int msgid, char text[IPC_TAILLE_MSG], long type) {
    Msg_IPC message;
    strcpy(message.text, text);
    message.type = type;
    msgsnd(msgid, &message, IPC_TAILLE_MSG, 0);
}

void rcvIPCm(int msgid, char text[IPC_TAILLE_MSG], long type) {
    Msg_IPC message;
    msgrcv(msgid, &message, IPC_TAILLE_MSG, type, 0);
    strcpy(text,message.text);
}

int deleteIPCm_serveur(int msgid) {
    return msgctl(msgid, IPC_RMID, 0);
}

int createIPCshm_serveur(key_t cle_msg){
    int shmid;
    if((shmid=shmget(cle_msg,  IPC_TAILLE_MRY*sizeof(int),IPC_CREAT|IPC_EXCL|0666))==1){
        perror("shmget");
        exit(2);
    }
    return shmid;
}

int createIPCshm_client(key_t cle_msg){
    int shmid;
    if(( shmid = shmget(cle_msg, IPC_TAILLE_MRY * sizeof(int), 0666))==1){
        perror ("shmget");
        exit(2);
    }
    return shmid;
}

void sendIPCshm(int shmid, char data[IPC_TAILLE_MRY]){
    int* adr;
    if((adr = (int *)shmat(shmid, 0, 0))==(int*)(-1)) {
        perror("shmat");
        exit(2);
    }

    //write data

    for(int i=0; i<IPC_TAILLE_MRY;i++){
        adr[i] = data[i];
    }

    shmdt(adr);
}

void rcvIPCshm(int shmid, char data[IPC_TAILLE_MRY]){
    int* adr;
    if((adr = (int *)shmat(shmid, 0, SHM_RDONLY))==(int*)(-1)) {
        perror("shmat");
        exit(2);
    }

    //read data

    for (int i = 0; i < IPC_TAILLE_MRY; i++){
        data[i] = adr[i];
    }
    
    shmdt(adr);
}

int deleteIPCshm_serveur(int shmid){
    return shmctl(shmid,IPC_RMID,NULL);
}