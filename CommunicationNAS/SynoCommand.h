#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define RESPONSE_LENGTH 500

int connexion(char*);

int available(char*);

int nasId(char*, char[15]);
int nasType(char*, char[50]);

int reboot(char*);
int softreset(char*);
int hardReset(char*);