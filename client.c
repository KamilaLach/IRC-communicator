#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/select.h>

#define ERROR {printf("FATAL (line %d): %s\n", __LINE__, strerror(errno)); \
				exit(errno);}   

typedef struct wiadomosc{
    char od[32];  
    char zawartosc[480]; 
} wiadomosc;

int sock, licznik = 0; 
char * wysylajacy, buf[480]; 

void init_terminal(char *snd) { 
    struct termios term; 
    tcgetattr( STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON);
    tcsetattr( STDIN_FILENO, TCSANOW, &term);

    memset(buf, 0, sizeof(buf)); 
    wysylajacy = snd;
    printf("\n\n%s: ", wysylajacy);
}

void* autor(void* arg) {
    wiadomosc message;
    fd_set set; 

    while(true) {
        FD_ZERO(&set); 
        FD_SET(sock, &set); 

        if (select(sock + 1, &set, NULL, NULL, NULL) < 0) ERROR;

        if(FD_ISSET(sock, &set)) {
            memset(&message, 0, sizeof(wiadomosc));
            if(read(sock, &message, sizeof(wiadomosc)) > 0) { 
                fflush(stdout); 

                printf("%s: %s\n", message.od, message.zawartosc); 

                printf("\n%s: ", wysylajacy);
                buf[licznik] = 0;
                printf("%.*s", licznik, buf); 
                fflush(stdout);
            
            }
        }
    }
}

void wyslij_wiadomosc() {
    wiadomosc message;
    memset(&message, 0, sizeof(wiadomosc)); 
    if (strcpy(message.od, wysylajacy) == NULL) ERROR; 
    if (strncpy(message.zawartosc, buf, licznik-1) == NULL) ERROR;
    if (write(sock, &message, sizeof(wiadomosc)) < 0) ERROR;
}

void* czytajacy(void* arg) {
    while(true) {
        buf[licznik++] = getchar();

        if(buf[licznik - 1] == '\x7f') { 
            if(--licznik)
                licznik--;
            printf("%s: ", wysylajacy);
            printf("%.*s", licznik, buf);  
        
        } else if(buf[licznik - 1] == '\n') { 
            if(licznik != 1) {
                wyslij_wiadomosc();
                printf("%s: %.*s\n", wysylajacy, licznik-1, buf);
                memset(buf, 0, sizeof(buf));   
            }
            licznik = 0;
            printf("\n%s: ", wysylajacy); 
        }
    }
}


void tworzenie_socket(char *nick, int port, char* ip_addr) {
    struct sockaddr_in sock_addr;  

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) ERROR; 

    memset(&sock_addr, 0, sizeof(sock_addr)); 
    sock_addr.sin_family = AF_INET; 
    sock_addr.sin_port = htons(port);
    inet_aton(ip_addr, &sock_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) ERROR; 

}

int main(int argc, char *argv[]) {
    pthread_t reader, writer; 
    tworzenie_socket(argv[1], atoi(argv[2]), argv[3]);

    init_terminal(argv[1]); 

    if (pthread_create(&writer, NULL, autor, NULL) < 0) ERROR;
    if (pthread_create(&reader, NULL, czytajacy, NULL) < 0) ERROR;
    if (pthread_join(writer, NULL) < 0) ERROR; 
    if (pthread_join(reader, NULL) < 0) ERROR; 

exit(0);
}
