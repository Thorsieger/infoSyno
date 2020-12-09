#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/shm.h>

#define IPC_TAILLE_MSG 40
#define CLE_MSG (key_t)102

#define IPC_TAILLE_MRY 1024
#define CLE_MRY (key_t)300

#define MESS_ERREUR "non reconnu"

typedef struct {
    long type;
    char text[IPC_TAILLE_MSG];
} Msg_IPC;

//IPC message
int createIPCm_serveur(key_t);
int joinIPCm_serveur(key_t);
void sendIPCm(int, char[IPC_TAILLE_MSG], long);
void rcvIPCm(int, char[IPC_TAILLE_MSG], long);
int deleteIPCm_serveur(int);

//IPC share memory
int createIPCshm_serveur(key_t);
int createIPCshm_client(key_t);
void sendIPCshm(int, char[IPC_TAILLE_MRY]);
void rcvIPCshm(int, char[IPC_TAILLE_MRY]);
int deleteIPCshm_serveur(int);
    