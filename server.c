#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/un.h>

#define ERROR {printf("FATAL (line %d): %s\n", __LINE__, strerror(errno)); \
				exit(errno);} 

int sockets[10]; 
int socket_net, licznik = 0; 

typedef struct wiadomosc{
    char od[32];  
    char zawartosc[480]; 
} wiadomosc;

void tworzenie_socket(int port) {
    struct sockaddr_in sock_addr;

    if ((socket_net = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) ERROR;  

    memset(&sock_addr, 0, sizeof(sock_addr)); 
    sock_addr.sin_family = AF_INET; 
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY); 

    if (bind(socket_net, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0) ERROR;
    if (listen(socket_net, 10) < 0) ERROR; 
}

void przekieruj_sygnal(int sig, void (*handler)(int)) { 
    struct sigaction act_usr;
    if (sigemptyset(&act_usr.sa_mask) < 0) ERROR; 
    if (sigaddset(&act_usr.sa_mask, sig) < 0) ERROR;
    act_usr.sa_handler = handler; 
    act_usr.sa_flags = 0;
    if (sigaction(sig, &act_usr, NULL) < 0) ERROR;
}

void exit_handler() { 
    for(int i = 0; i < licznik; i++) {
        shutdown(sockets[i], SHUT_RDWR);
        if (close(sockets[i]) < 0) ERROR; 
    }
    if (close(socket_net) < 0) ERROR; 

    printf("Zamknieto poprawnie\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    fd_set set;
    int i, max;
    wiadomosc message;
    
    przekieruj_sygnal(SIGINT, &exit_handler); 
    tworzenie_socket(atoi(argv[1])); 

    max = socket_net + 1;

    while(true) {
        FD_ZERO(&set); 

        FD_SET(socket_net, &set); 
        for(i = 0; i < licznik; i++)
            FD_SET(sockets[i], &set); 

        if (select(max, &set, NULL, NULL, NULL) < 0) ERROR; 

        if(FD_ISSET(socket_net, &set)) { 
            sockets[licznik] = accept(socket_net, NULL, NULL);
            if(0 > sockets[licznik]) { 
              perror("error accept failed");
              close(sockets[licznik]);
              exit(EXIT_FAILURE); 
            }
            if(max < sockets[licznik] + 1) 
                max = sockets[licznik] + 1; 
            licznik++;  
        }

        for(i = 0; i < licznik; i++)
            if(FD_ISSET(sockets[i], &set)) {
                memset(&message, 0, sizeof(wiadomosc));
                if(read(sockets[i], &message, sizeof(wiadomosc)) == 0) 
                    sockets[i] = sockets[--licznik]; 
                else
                    for(int j = 0; j< licznik; j++)
                        if(j != i)
                            if (write(sockets[j], &message, sizeof(wiadomosc)), "Write error.");
            }
    }
    return 0;
}

