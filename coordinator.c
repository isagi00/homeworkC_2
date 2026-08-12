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
#include "logger.h"
#include "worker.h"

#include <stdio.h>  //util
#include <stdlib.h> //util
#include <unistd.h> //util
#include <string.h>

#include <sys/socket.h> //creazione socket, bind, listen ... 
#include <netinet/in.h> //preparazione indirizzo

#include <pthread.h>

#include <signal.h>
#include <errno.h>

#define MAX_THREADS 1024

//flag settato da handler di SIGINT
volatile sig_atomic_t shutdown_requested = 0;

//array che tiene traccia di tutti i thread attivi
pthread_t thread_ids[MAX_THREADS];
int thread_count = 0;
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER; //lock per accesso all'array dei thread

int listen_fd_global = -1;

void sigint_handler(int signo){
    (void) signo;
    shutdown_requested = 1;
    if (listen_fd_global != -1){ //(listen fd era aperto)
        close(listen_fd_global);
    }
}

int main(void){
    //registrazione handler SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); //pulisci memoria
    sa.sa_handler = sigint_handler; //collega handler
    sigemptyset(&sa.sa_mask); //imposta sa_mask come vuoto, quindi accetta tutti i segnali durante eseuzione di sigint_handler
    sa.sa_flags = 0; //nessun flag speciale
    sigaction(SIGINT, &sa, NULL);   //usa sa per la gestione di SIGINT

    //creazione socket lato server, sempre in ascolto
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET: usa ipv4, SOCK_STREAM: usa tcp, 0: protocollo scelto automaticamente
    if (listen_fd < 0) {
        perror("socket() fallita");
        exit(1);
    }
    printf("server socket creato correttamente. fd = %d\n", listen_fd);
    listen_fd_global = listen_fd;

    //preparazione indirizzo socket
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET; //stesso protocollo (ipv4) usato per la creazione del socket
    addr.sin_addr.s_addr = INADDR_ANY;  // indirizzo ip su cui il server ascolterà. IDADDR_ANY = 0.0.0.0
    addr.sin_port = htons(PORT); //porta su cui server sta in ascolto. htons() = host to network short, converte la porta al formato della rete.

    //SO_REUSEADDR, riuso rapido porta
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

    //apertura file di log
    if (logger_init() != 0){   
        fprintf(stderr, "apertura file di log fallita\n");
        exit(4);
    }   

    //loop principale, continua ad accettare connessioni affinché non si ha SIGINT
    while (!shutdown_requested){ 
        //accept(), accetta connessioni arrivate al socket.
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0){
            if (shutdown_requested){
                break;  //se accept interrotto da SIGINT esci direttamente
            }
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

        //aggiungi in lista il thread creato
        pthread_mutex_lock(&thread_list_mutex);
        if (thread_count < MAX_THREADS){
            thread_ids[thread_count++] = tid;
        }
        pthread_mutex_unlock(&thread_list_mutex);

        // //libera risorse usate dal thread dopo aver terminato
        // pthread_detach(tid); 
    }

    printf("ricevuto SIGINT, chiusura in corso...\n");

    //attendi join di tutti i thread registrati / creati
    for (int i = 0; i < thread_count; i++){
        pthread_join(thread_ids[i], NULL);
    }
    printf("terminati tutti i thread, finalizzata chiusura \n");

    logger_close();
    printf("chiuso file di log. \n");

    // close(listen_fd);
    return 0;
}
