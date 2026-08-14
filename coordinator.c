/*
direttore di orchestra.
*/

#include "common.h"
#include "logger.h"
#include "worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define MAX_THREADS 1024

//flag settato da handler di SIGINT
volatile sig_atomic_t shutdown_requested = 0;

//flag settato da handler di SIGALRM (controllato nel main, NON si chiama logger da dentro il signal handler)
volatile sig_atomic_t rotate_requested = 0;

pthread_t thread_ids[MAX_THREADS];
int thread_count = 0;
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;

int listen_fd_global = -1;

void sigint_handler(int signo){
    (void) signo;
    shutdown_requested = 1;
    if (listen_fd_global != -1){
        close(listen_fd_global);
    }
}

void sigalrm_handler(int signo){
    (void) signo;
    rotate_requested = 1;
    alarm(5);
}

void sigpipe_handler(int signo){
    (void)signo;
    // printf("[handler] thread %p sta gestendo sigpipe, current_client_id = %d\n", (void* ) pthread_self(), current_client_id);
    logger_write_event(current_client_id, "DISCONNECT");
}

int main(void){
    //registrazione handler SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    
    //registrazione handler SIGALRM
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof(sa_alarm));
    sa_alarm.sa_handler = sigalrm_handler;
    sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = 0;
    sigaction(SIGALRM, &sa_alarm, NULL);
    

    //registrazione handler SIGPIPE
    struct sigaction sa_pipe;
    memset(&sa_pipe, 0, sizeof(sa_pipe));
    sa_pipe.sa_handler = sigpipe_handler;
    sigemptyset(&sa_pipe.sa_mask);
    sa_pipe.sa_flags = 0;
    sigaction(SIGPIPE, &sa_pipe, NULL);

    //creazione socket lato server, sempre in ascolto
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET: usa ipv4, SOCK_STREAM: usa tcp, 0: protocollo scelto automaticamente
    if (listen_fd < 0) {
        perror("[coordinator] socket() fallita");
        exit(1);
    }
    printf("[coordinator] server socket creato correttamente. fd = %d\n", listen_fd);
    listen_fd_global = listen_fd;

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt SO_REUSEADDR fallita");
        exit(1);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if ( bind(listen_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0){
        perror("[coordinator] server bind() fallito");
        exit(2);
    }
    printf("[coordinator] server port: %d \n", addr.sin_port);
    printf("[coordinator] server ip: %d \n", addr.sin_addr.s_addr);

    int max_connessioni_in_attesa = 5;
    if ( listen(listen_fd, max_connessioni_in_attesa) < 0){
        perror("[coordinator] server listen() fallito");
        exit(3);
    }
    printf("[coordinator] in ascolto sulla porta %d... \n", PORT);

    //apertura file di log
    if (logger_init() != 0){   
        fprintf(stderr, "[coordinator] apertura file di log fallita\n");
        exit(4);
    }

    alarm(5);

    while (!shutdown_requested){
        int client_fd = accept(listen_fd, NULL, NULL);

        if (rotate_requested){
            rotate_requested = 0;
            // printf("[coordinator] creando nuovo file di log... \n");
            logger_check_and_rotate();
        }

        if (client_fd < 0){
            if (shutdown_requested){
                break;
            }
            if (errno == EINTR){
                continue; //interrotto da un segnale, ad esempio da SIGPIPE. continua ad accettare connessioni senza stampare nulla
            }
            perror("[coordinator] server accept() fallito");
            continue;   //se accept fallisce prova con la prossima connessione
        }
        printf("[coordinator] accettato connessione client fd = %d\n", client_fd);

        int* arg = malloc(sizeof(int));
        if (!arg){
            perror("[coordinator] malloc() fallita");
            close(client_fd);
            continue;
        }
        *arg = client_fd;

        //creazione thread
        pthread_t tid; //id del thread
        if (pthread_create(&tid, NULL, handle_client, arg) != 0){ //handle_client: funzione che il thread creato eseguirà, arg: argomento da passare alla funzione
            perror("[coordinator] pthread create fallita");
            free(arg);
            close(client_fd);
            continue;
        }
        printf("[coordinator] creato thread %p per il client fd %d\n", (void *)tid, client_fd);

        pthread_mutex_lock(&thread_list_mutex);
        if (thread_count < MAX_THREADS){
            thread_ids[thread_count++] = tid;
        }
        pthread_mutex_unlock(&thread_list_mutex);
    }

    printf("[coordinator] ricevuto SIGINT, chiusura in corso...\n");

    for (int i = 0; i < thread_count; i++){
        pthread_join(thread_ids[i], NULL);
    }
    printf("[coordinator] terminati i thread\n");

    logger_close();
    printf("[coordinator] chiuso file di log \n");

    return 0;
}