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

int main(void){
    //registrazione handler SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, sigalrm_handler);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket() fallita");
        exit(1);
    }
    printf("server socket creato correttamente. fd = %d\n", listen_fd);
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
        perror("server bind() fallito");
        exit(2);
    }
    printf("server port: %d \n", addr.sin_port);
    printf("server ip: %d \n", addr.sin_addr.s_addr);

    int max_connessioni_in_attesa = 5;
    if ( listen(listen_fd, max_connessioni_in_attesa) < 0){
        perror("server listen() fallito");
        exit(3);
    }
    printf("coordinator in ascolto sulla porta %d... \n", PORT);

    if (logger_init() != 0){
        fprintf(stderr, "apertura file di log fallita\n");
        exit(4);
    }

    alarm(5);

    while (!shutdown_requested){
        int client_fd = accept(listen_fd, NULL, NULL);

        if (rotate_requested){
            rotate_requested = 0;
            logger_check_and_rotate();
        }

        if (client_fd < 0){
            if (shutdown_requested){
                break;
            }
            perror("server accept() fallito");
            continue;
        }
        printf("accettato connessione client fd = %d\n", client_fd);

        int* arg = malloc(sizeof(int));
        if (!arg){
            perror("malloc() fallita");
            close(client_fd);
            continue;
        }
        *arg = client_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, arg) != 0){
            perror("pthread create fallita");
            free(arg);
            close(client_fd);
            continue;
        }
        printf("creato thread %p per il client fd %d\n", (void *)tid, client_fd);

        pthread_mutex_lock(&thread_list_mutex);
        if (thread_count < MAX_THREADS){
            thread_ids[thread_count++] = tid;
        }
        pthread_mutex_unlock(&thread_list_mutex);
    }

    printf("ricevuto SIGINT, chiusura in corso...\n");

    for (int i = 0; i < thread_count; i++){
        pthread_join(thread_ids[i], NULL);
    }
    printf("terminati tutti i thread, finalizzata chiusura \n");

    logger_close();
    printf("chiuso file di log. \n");

    return 0;
}