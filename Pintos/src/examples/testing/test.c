#include <bits/fcntl-linux.h>
void main() {

char buffer[1024] = {'\0'};
int fd = open("test", O_RDWR) ;
int pid = fork();
int write

}