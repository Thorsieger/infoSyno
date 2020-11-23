#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define RESPONSE_LENGTH 500

int initSerialConnexion(char* tty){
    struct termios options;
    
    int fd = open(tty, O_RDWR | O_NOCTTY | O_NDELAY);
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

void sendCommand(int fd, char* command, char buffer[RESPONSE_LENGTH]){
    int commandlength = sizeof(command)/sizeof(command[0])-3;//retirer \0 et passer \n de 2 à 1 caractère
    write(fd,"ls /\n",commandlength);
    usleep ((commandlength + RESPONSE_LENGTH) * 100); //attente de 100µs pour chaque caractère envoyé + recu
    read(fd,buffer,RESPONSE_LENGTH);
}

void printResponse(char* response){
    for (int i = 0; i < RESPONSE_LENGTH; i++)
    {
        printf("%c",response[i]);
        response[i] = 0;
    }
    printf("\n");
}

int main(void){
    char response[RESPONSE_LENGTH] = {0};

    int fd = initSerialConnexion("/dev/ttyUSB0");
    
    sendCommand(fd,"ls /\n", response);
    printResponse(response);
    
    stopSerialConnexion(fd);
}