/*
direttore di orchestra.

- main(): parsing argomenti (porta, path del log, soglia dimensione)
    - setup socket: socket(), setsockopt(SO_REUSEADDR), bind(), listen()
    - installazione signal handler
    - loop principale:
        accept() -> per ogni connessione lancia un worker
    - tiene traccia di worker attivi
    - handler dei segnali stessi
*/

#include "common.h"
#include "worker.h"

#include <stdio.h>  //util
#include <stdlib.h> //util
#include <unistd.h> //util


#include <sys/socket.h> //creazione socket, bind, listen ... 
#include <netinet/in.h> //preparazione indirizzo

#include <pthread.h>

int main(void){
    //creazione socket lato server, sempre in ascolto
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET: usa ipv4, SOCK_STREAM: usa tcp, 0: protocollo scelto automaticamente
    if (listen_fd < 0) {
        perror("socket() fallita");
        exit(1);
    }
    printf("server socket creato correttamente. fd = %d\n", listen_fd);

    //preparazione indirizzo socket
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET; //stesso protocollo (ipv4) usato per la creazione del socket
    addr.sin_addr.s_addr = INADDR_ANY;  // indirizzo ip su cui il server ascolterà. IDADDR_ANY = 0.0.0.0
    addr.sin_port = htons(PORT); //porta su cui server sta in ascolto. htons() = host to network short, converte la porta al formato della rete.

    // //SO_REUSEADDR, riutilizzo porta e indirizzo
    int opt = 1; //abilita SO_REUSEADDR
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    //bind(), assegna porta + indirizzo al socket.
    if ( bind(listen_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0){
        perror("server bind() fallito");
        exit(2);
    }
    printf("server port: %d \n", addr.sin_port);
    printf("server ip: %d \n", addr.sin_addr.s_addr);

    //listen(), mette in ascolto il socket.
    int max_connessioni_in_attesa = 5;
    if ( listen(listen_fd, max_connessioni_in_attesa) < 0){
        perror("server listen() fallito");
        exit(3);
    }
    printf("coordinator in ascolto sulla porta %d... \n", PORT);



    while (1){ //loop per continuare ad accettare connessioni
        //accept(), accetta connessioni arrivate al socket.
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0){
            perror("server accept() fallito");
            continue;   //se accept fallisce prova con la prossima connessione
        }
        printf("accettato connessione client fd = %d\n", client_fd);

        //allocazione in heap del client fd
        int* arg = malloc(sizeof(int)); //alloca spazio su heap per memorizzare i client fd
        if (!arg){
            perror("malloc() fallita");
            close(client_fd);
            continue;
        }
        *arg = client_fd; //copia il client fd (nello stack) sullo spazio allocato in heap

        //creazione thread
        pthread_t tid; //id del thread
        if (pthread_create(&tid, NULL, handle_client, arg) != 0){ //handle_client: funzione che il thread creato eseguirà, arg: argomento da passare alla funzione
            perror("pthread create fallita");
            free(arg);
            close(client_fd);
            continue;
        }
        printf("creato thread %p per il client fd %d\n", (void *)tid, client_fd);
        
        //libera risorse usate dal thread dopo aver terminato
        pthread_detach(tid); 
    }


    close(listen_fd);
    return 0;
}
