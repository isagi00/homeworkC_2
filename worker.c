/*
gestione di una singola connessione già accettata.
riceve file descriptor del client pronto.
*/

#include "worker.h"
#include "common.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>

void* handle_client(void* arg){
    int client_fd = *(int*) arg;
    free(arg);

    struct msg m;
    ssize_t n = recv(client_fd, &m, sizeof(m), 0);
    if (n == sizeof(m)){
        logger_write(m.id_mittente, m.dato);
        char *ok = "ok";
        ssize_t s = send(client_fd, ok, strlen(ok)+1, 0);
        if(s > 0){
            printf("messaggio mandato\n");
        }
        else{
            printf("sigpipe\n");
            logger_write_event(m.id_mittente, "DISCONNECT");
        }
    }
    else if (n == 0) {
        printf("client fd = %d disconnesso senza inviare dati\n", client_fd);
    }
    else if (n < 0){
        printf("server recv fallito o dati parziali \n");
    }
    close(client_fd);
    return NULL;
}