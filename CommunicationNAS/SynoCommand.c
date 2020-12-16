#include "SynoCommand.h"
#include <stdio.h>

int initSerialConnexion(char* tty){
    struct termios options;
    
    int fd = open(tty, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd<0)return -1;

    fcntl(fd, F_SETFL, 0);
    
    tcgetattr(fd, &options);
    options.c_cflag     |= (CLOCAL | CREAD);
    options.c_cflag     |=  CS8;
    options.c_cflag     &= ~CRTSCTS;
    options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag     &= ~OPOST;
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 5;
    
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    
    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

void stopSerialConnexion(int fd){
    close(fd);
}

void sendSerialCommand(int fd, char* command, int commandlength, char buffer[RESPONSE_LENGTH]){
    tcflush(fd,TCIOFLUSH);
    write(fd,command,commandlength);
    usleep ((commandlength + RESPONSE_LENGTH) * 100); //attente de 100µs pour chaque caractère envoyé + recu
    read(fd,buffer,RESPONSE_LENGTH);
}

int connexionCompteInfomaniak(int fd){
    char response1[RESPONSE_LENGTH] = {0};
    char response2[RESPONSE_LENGTH] = {0};
    char response3[RESPONSE_LENGTH] = {0};
    char* command1 = "Infomaniak\n";
    char* command2 = "sudo su -\n";
    char pwd[10] = "glopglop\n";

    sendSerialCommand(fd,command1, strlen(command1), response1);
    if(strncmp(response1+strlen(command1),"\r\nPassword: ",13)!=0) return 0;// ou si réponse == admin@ ou Infomaniak@ (err passage root)
    sendSerialCommand(fd,pwd, strlen(pwd), response3);

    if(strncmp(response3+10,"Infomaniak@",11)!=0) return 0;

    sleep(1);
    sendSerialCommand(fd,command2, strlen(command2), response1);//sudo su
    sendSerialCommand(fd,pwd, strlen(pwd), response1);//pwd
    sleep(2);//Il faut attendre un peu que le mot de passe soit accepté
    sendSerialCommand(fd,"\3", 1, response2);//ctrl c
    if(strncmp(response2+8,"root@",5)!=0) return 0;
    return 1;
}

int connexionCompteAdmin(int fd){

    char response1[RESPONSE_LENGTH] = {0};
    char response2[RESPONSE_LENGTH] = {0};
    char* command1 = "admin\n";
    char* command2 = "sudo su -\n";
    char* pwd = "\n";

    
    sendSerialCommand(fd,command1, strlen(command1), response1);
    if(!(strncmp(response1,"admin\r\nPassword: ",17)==0 || strncmp(response1,"admin\r\r\nPassword: ",18)==0)) return 0;
    sendSerialCommand(fd,pwd, strlen(pwd), response1);
    if(strncmp(response1,"\r\n -- admin: /var/services/homes/admin",38)!=0) return 0;
    sleep(1);

    //Passage en root
    sendSerialCommand(fd,command2, strlen(command2), response2);//sudo su
    sendSerialCommand(fd,pwd, strlen(pwd), response2);//blank pwd
    sleep(2);//Il faut attendre un peu que le mot de passe soit accepté
    sendSerialCommand(fd,"\3", 1, response2);//ctrl c

    if(strncmp(response2+8,"root@",5)!=0) return 0;
    return 1;
}

void creationCompte(int fd){
    char response[RESPONSE_LENGTH] = {0};

    char* testaccount = "ls /etc/sudoers.d/\n";
    sendSerialCommand(fd,testaccount, strlen(testaccount), response);
    if(strncmp(response + strlen(testaccount),"\nInfomaniak",11)==0) return;

    char* command1 = "echo \'Infomaniak:$6$ySRYvFaW6$k041gHAOqJOf3thxGKWj4zqi0/ohIT3paw.cI5XgZENWw3GZARUtQez9dsaOv1u7oPnpM/0y7dE2WdGLT6G7Z.:18597:0:99999:7:::\' >> /etc/shadow\n";
    char* command2 = "echo \'Infomaniak:x:8:99::/:/bin/sh\' >> /etc/passwd\n";
    char* command3 = "echo \'Infomaniak ALL=(ALL) ALL\' >> /etc/sudoers.d/Infomaniak\n";
    char* command4 = "echo -e \'Match User Infomaniak\nDenyUsers Infomaniak\' >> /etc/ssh/sshd_config\n";
    sendSerialCommand(fd,command1, strlen(command1), response);
    sendSerialCommand(fd,command2, strlen(command2), response);
    sendSerialCommand(fd,command3, strlen(command3), response);
    sendSerialCommand(fd,command4, strlen(command4), response);
}

void getNasId(int fd, char macAddr[15]){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "ip a | grep \"link/ether\" -m 1 | cut -c 16-29\n";
    sendSerialCommand(fd,command, strlen(command), response);
    strncpy(macAddr,response + strlen(command)+1,14); //addres mac commence après la commande et le retour à la ligne et mesure 14 caractères
}

void getNasType(int fd,char nasType[50]){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "cat /etc/synoinfo.conf | grep \"upnpmodelname\" -m 1\n";
    sendSerialCommand(fd,command, strlen(command), response);
    strtok(response + strlen(command)+1,"\"");
    strcpy(nasType,strtok(NULL,"\""));
}

int isNasAvailable(int fd){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "\n";
    sendSerialCommand(fd,command, strlen(command), response);
    if(strncmp(response+10,"root@",5)!=0) return 0; //+10 car on reviens la couleur du texte également
    return 1;
}

void sendReboot(int fd){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "reboot\n";
    sendSerialCommand(fd,command, strlen(command), response);
}

void sendSoftreset(int fd){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "synodsdefault --reset-config\n";
    sendSerialCommand(fd,command, strlen(command), response);
}

int sendHardreset(int fd){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "synodsdefault --reinstall\n";
    sendSerialCommand(fd,command, strlen(command), response);

    if(strncmp(response+strlen(command),"\nSuccess.",9)!=0) return 0; 
    return 1;
}


int available(char* tty){
    int fd = initSerialConnexion(tty);
    if(fd<0)return -1;
    
    int status = isNasAvailable(fd);

    stopSerialConnexion(fd);
    return status;
}

/*Fonctions principales*/
int connexion(char* tty){
    int fd;
    do
    {
        fd = initSerialConnexion(tty);
    } while (fd<0);
    if(isNasAvailable(fd))return 1;

    while(1){
        if(connexionCompteInfomaniak(fd)) break;
        sleep(6);
        if(connexionCompteAdmin(fd)){sleep(1);creationCompte(fd); break;}
        sleep(6);

        struct termios options;
        tcgetattr(fd, &options);
        options.c_cc[VMIN]  = 1;
        options.c_cc[VTIME] = 0;
        tcsetattr(fd, TCSANOW, &options);

        char response[RESPONSE_LENGTH];
        char* command = "\n";
        sendSerialCommand(fd,command, strlen(command), response);

        options.c_cc[VMIN]  = 0;
        options.c_cc[VTIME] = 5;
        tcsetattr(fd, TCSANOW, &options);
    }
    
    int isAvailable = isNasAvailable(fd);
    //ecrire etat nas api
    stopSerialConnexion(fd);
    return isAvailable;
}

int reboot(char* tty){
    int fd = initSerialConnexion(tty);
    if(fd<0)return -1;
    if(!isNasAvailable(fd)) return 0;

    sendReboot(fd);
    //ecrire etat nas api
    stopSerialConnexion(fd);
    //connexion(tty)
    return 1;
}

int softreset(char* tty){
    int fd = initSerialConnexion(tty);
    if(fd<0)return -1;
    if(!isNasAvailable(fd)) return 0;

    sendSoftreset(fd);
    //ecrire etat nas api
    //si available, alors le softreset est fini
    stopSerialConnexion(fd);
    return 1;
}

int hardReset(char* tty){
    int fd = initSerialConnexion(tty);
    if(fd<0)return -1;
    if(!isNasAvailable(fd)) return 0;

    if(sendHardreset(fd))return 0;
    sendReboot(fd);
    //ecrire etat nas api
    stopSerialConnexion(fd);

    //connexion(tty);
    return 1;
}

int nasId(char* tty,char macAddr[15]){
    int fd = initSerialConnexion(tty);
    if(fd<0)return -1;
    if(!isNasAvailable(fd)) return 0;

    getNasId(fd,macAddr);

    return 1;
}

int nasType(char* tty,char type[50]){
    int fd = initSerialConnexion(tty);
    if(fd<0)return -1;
    if(!isNasAvailable(fd)) return 0;

    getNasType(fd,type);

    return 1;
}