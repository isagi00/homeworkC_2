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

#include <sys/socket.h>

void* handle_client(void* arg){
    int client_fd = *(int*) arg; //cast di arg in int, poi prendi il contenuto
    free(arg);  //libera memoria allocata in main, non serve

    //ricevi messaggio
    struct msg m;
    ssize_t n = recv(client_fd, &m, sizeof(m), 0);

    if (n == sizeof(m)){
        logger_write(m.id_mittente, m.dato);
    }
    else if (n == 0) {  //client disconnesso
        printf("client fd = %d disconnesso senza inviare dati\n", client_fd);
    }
    else if (n < 0){
        printf("server revc fallito o dati parziali \n");
    }
    close(client_fd);
    return NULL;
}

