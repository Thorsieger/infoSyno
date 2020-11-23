#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include<stdlib.h>
#include <string.h>

#define RESPONSE_LENGTH 500

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
    write(fd,command,commandlength);
    usleep ((commandlength + RESPONSE_LENGTH) * 100); //attente de 100µs pour chaque caractère envoyé + recu
    read(fd,buffer,RESPONSE_LENGTH);
}

/*void printSerialResponse(char* response){
    for (int i = 0; i < RESPONSE_LENGTH; i++)
    {
        printf("%c",response[i]);
    }
    printf("\n");
}*/

void getDate(int fd, char* date){
    for (int i = 0; i < 3; i++)//la troisière erreur permet d'avoir la date
    {
        char response1[RESPONSE_LENGTH] = {0};
        char response2[RESPONSE_LENGTH] = {0};

        char* command = "root\n";
        char* command2 = "\n";

        sendSerialCommand(fd,command,strlen(command),response1);
        write(fd,command2,strlen(command2));
        sleep(5);
        read(fd,response2,RESPONSE_LENGTH);

        char* buf = strtok(response2,"\n");
        strcpy(date,strtok(NULL,"\n"));
        strcpy(date,strtok(NULL,"\n"));
    }
}

int gcd(int a, int b){return (b?gcd(b,a%b):a);}

void dateToPwd(char date[30], char pwd[9]){
    char month[4],day[3];
    int m;

    char* buf = strtok(date," ");
    strcpy(month,strtok(NULL," "));
    strcpy(day,strtok(NULL," "));

    if(strcmp(month,"Jan")==0)m=1;
    else if(strcmp(month,"Feb")==0)m=2;
    else if(strcmp(month,"Mar")==0)m=3;
    else if(strcmp(month,"Apr")==0)m=4;
    else if(strcmp(month,"May")==0)m=5;
    else if(strcmp(month,"Jun")==0)m=6;
    else if(strcmp(month,"Jul")==0)m=7;
    else if(strcmp(month,"Aug")==0)m=8;
    else if(strcmp(month,"Sep")==0)m=9;
    else if(strcmp(month,"Oct")==0)m=10;
    else if(strcmp(month,"Nov")==0)m=11;
    else if(strcmp(month,"Dec")==0)m=12;

    sprintf(pwd,"%x%02d-%02x%02d\n",m,m,atoi(day),gcd(m,atoi(day)));
}

int connexion(int fd){
    char date[30] = {0};
    char pwd[9] = {0};

    getDate(fd,date);
    dateToPwd(date,pwd);

    char response[RESPONSE_LENGTH] = {0};
    char* command = "root\n";
    sendSerialCommand(fd,command, strlen(command), response);
    if(strncmp(response+strlen(command),"\nPassword: ",11)!=0) return 0;
    sendSerialCommand(fd,pwd, strlen(pwd), response);
    return 1;
}

void getNasId(int fd, char macAddr[15]){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "ip a | grep \"link/ether\" -m 1 | cut -c 16-29\n";
    sendSerialCommand(fd,command, strlen(command), response);
    strncpy(macAddr,response + strlen(command)+1,14); //addres mac commence après la commande et le retour à la ligne et mesure 14 caractères
}

void getNasType(int fd,char nasType[50]){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "cat /etc/synoinfo.conf | grep \"upnpmodel\" -m 1\n";
    sendSerialCommand(fd,command, strlen(command), response);
    char* buf = strtok(response + strlen(command)+1,"\"");
    strcpy(nasType,strtok(NULL,"\""));
}

int nasAvailable(int fd){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "\n";
    sendSerialCommand(fd,command, strlen(command), response);
    if(strncmp(response,"\r\nSynologyNAS> ",15)==0) return 1;
    else return 0;
}

int reboot(int fd){
    char response[RESPONSE_LENGTH] = {0};
    char* command = "reboot\n";
    sendSerialCommand(fd,command, strlen(command), response);
    printf("%s",response);
    if(strncmp(response+strlen(command),"\r\nThe system is going down NOW!",31)==0) return 1;
    else return 0;
    
}

int main(void){
    char response[RESPONSE_LENGTH] = {0};

    char macAddr[15] = {0};
    char nasType[50] = {0};
    int available = 0;
    int isrebooting = 0;

    int fd = initSerialConnexion("/dev/ttyUSB0");
    if(fd<0)return printf("error\n");
    
    connexion(fd);

    available = nasAvailable(fd);
    printf("Dispo pour commande ? %d\n",available);

    if(available){
        getNasId(fd,macAddr);
        getNasType(fd,nasType);
        //isrebooting = reboot(fd);
    }


    printf("Affichage des résultats :\n");
    printf("Type de NAS : %s\n",nasType);
    printf("Addresse mac/identifiant unique : %s\n",macAddr);
    printf("reboot ? %d\n",isrebooting);

    stopSerialConnexion(fd);
}