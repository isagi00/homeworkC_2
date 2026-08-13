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
#include <errno.h>


//globale
int current_client_id = -1;
/*
su linux thread che genera sigpipe teoricamente gestisce lui stesso l'handler del sigpipe -> un current_client_id locale per ogni thread sarebbe stato ok (__thread, locale per ogni thread)
siccome sto su macos il thread che genera sigpipe è diverso da quello che esegue handler del sigpipe -> 2 thread diversi, current_client_id del worker veniva impostato correttamente, ma il current_client_id del handler non veniva mai impostato
quindi serve mutex 
    dato che ci sta solo 1 current_client_id GLOBALE, solo un 1 worker può modificarla
    quindi quando si fa send() sleep() send() -> sigpipe -> handler, il current_client_id viene bloccato
*/
static pthread_mutex_t ack_mutex = PTHREAD_MUTEX_INITIALIZER;


void* handle_client(void* arg){
    int client_fd = *(int*) arg;
    free(arg);

    struct msg m;
    ssize_t n = recv(client_fd, &m, sizeof(m), 0);
    if (n == sizeof(m)){
        printf("[worker, client id = %d] ricevuto messaggio correttamente\n", m.id_mittente);
        logger_write(m.id_mittente, m.dato);

        // char *ok = "ok";
        // ssize_t s = send(client_fd, ok, strlen(ok)+1, 0);
        // if(s > 0){
        //     printf("messaggio mandato\n");
        // }
        // else{
        //     printf("sigpipe\n");
        //     logger_write_event(m.id_mittente, "DISCONNECT");
        // }

        //sezione critica, solo 1 worker alla volta può fare send() sleep() send(), per non far modificare current_client_id da altri thread
        pthread_mutex_lock(&ack_mutex);
        current_client_id = m.id_mittente;
        // printf("[worker] thread %p ha impostato current_client_id = %d\n", (void* )pthread_self(), current_client_id);

        //prova mandare ack al client
        const char* ack = "ACK";
        //primo tentativo, potrebbe avere successo a vuoto se client si è appena disconnesso e kernel non ha ancora processato la disconnessione
        ssize_t sent = send(client_fd, ack, strlen(ack), 0);
        // if (sent < 0){
        //     if (errno == EPIPE){
        //         printf("client id = %d disconnesso prima di ack (EPIPE)\n", m.id_mittente);
        //     }
        //     else{
        //         perror("worker: errore ack");
        //     }
        // }
        // else{
        //     printf("worker per %d: ack inviato con successo\n", m.id_mittente);
        // }

        //dare tempo al kernel per notare la disconnessione
        usleep(50000); 
        ssize_t sent2 = send(client_fd, ack, strlen(ack), 0);
        printf("[worker, client id = %d] mandato ack\n", m.id_mittente);

        if (sent2 < 0){
            if (errno == EPIPE){
                printf("[worker, client id = %d] client disconnesso prima di aver ricevuto ack (EPIPE su seconda send)\n", m.id_mittente);
            }
            else{
                perror("worker: seconda send ack fallita");
            }
        }
        else{
            printf("[worker, client id = %d] ack inviato con successo (2)\n", m.id_mittente);
        }

        pthread_mutex_unlock(&ack_mutex);
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