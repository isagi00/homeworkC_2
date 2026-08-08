/*
producer di dati

- parsing argomenti (ip, porta, id_produttore, forse anche frequenza di invii)
- socket(), connect()
- loop di invio : costruisce secondo struct di common.h, send()
- close() a fine invio


*/
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //util

#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char* argv[]){
    if (argc < 3){
        fprintf(stderr, "uso: %s <id_mittente> <dato> \n ", argv[0]);
        exit(1);
    }

    //creazione socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0){
        perror("client socket() fallito");
        exit(1);
    }
    printf("client socket creato correttamente. fd = %d \n", sock_fd);

    //costruzione indirizzo
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    char localhost_ip[] = "127.0.0.1";
    inet_pton(AF_INET, localhost_ip, &addr.sin_addr); //conversione ip numerico in binario
     

    //connessione
    if ( connect(sock_fd, (struct sockaddr*)  &addr, sizeof(addr)) < 0){
        perror("client connect() fallito");
        exit(2);
    }
    printf("client fd [%d] connesso su porta %d con ip %s \n", sock_fd, addr.sin_port, localhost_ip);
    //costruisci messaggio
    struct msg m;
    m.id_mittente = atoi(argv[1]);
    m.dato = atof(argv[2]);

    //manda
    send(sock_fd, &m, sizeof(m), 0);
    printf("client fd [%d] mandato un messaggio \n", sock_fd);

    close(sock_fd);
    return 0;
}