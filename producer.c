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
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char* argv[]){
    if (argc < 3){
        fprintf(stderr, "uso: %s <id_mittente> <dato> [--no-ack]\n ", argv[0]);
        exit(1);
    }
    char* id = argv[1];
    int skip_ack = (argc >= 4 && strcmp(argv[3], "--no-ack") == 0);

    //creazione socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0){
        perror("client socket() fallito");
        exit(1);
    }
    printf("[client id = %s] socket creato correttamente. fd = %d \n", id, sock_fd);

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
    printf("[client id = %s] fd [%d] connesso su porta %d con ip %s \n",id, sock_fd, addr.sin_port, localhost_ip);

    //costruisci messaggio
    struct msg m;
    m.id_mittente = atoi(argv[1]);
    m.dato = atof(argv[2]);

    //manda
    send(sock_fd, &m, sizeof(m), 0);
    printf("[client id = %s] fd [%d] mandato un messaggio \n",id,sock_fd);


    if (skip_ack){
        //disconnessione veloce tramite --no-ack, per test SIGPIPE
        close(sock_fd);
        printf("[client id = %d] --no-ack, chiuso socket. \n", m.id_mittente);
        return 0;
    }

    //ricevi ack 
    char buf[16] = {0};
    ssize_t n = recv(sock_fd, buf, sizeof(buf) - 1, 0);
    if (n > 0){
        printf("[client id = %d] ricevuto ack dal coordinator: %s \n", m.id_mittente, buf);
    }
    else{
        printf("[client id = %d] nessun ack ricevuto \n", m.id_mittente);
    }

    // // ricevi la conferma (ACK) dal coordinatore
    // char risposta[16];
    // ssize_t n = recv(sock_fd, risposta, sizeof(risposta) - 1, 0);

    // if (n > 0){
    //     risposta[n] = '\0';
    //     printf("client fd [%d] ricevuto ACK: %s\n", sock_fd, risposta);
    // }
    // else if (n == 0){
    //     printf("client fd [%d] coordinatore ha chiuso la connessione senza rispondere\n", sock_fd);
    // }
    // else{
    //     perror("client recv() fallita");
    // }

    close(sock_fd);
    return 0;
}