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

#include <stdio.h>  //util
#include <stdlib.h> //util
#include <unistd.h> //util

#include <sys/socket.h> //creazione socket, bind, listen ... 
#include <netinet/in.h> //preparazione indirizzo
#include <time.h> //per timestamp

#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
/*
pthread_mutex_t: tipo di dato, mutex che permette a un solo thread alla volta di accedere in una sezione critica.
PTHREAD_MUTEX_INITIALIZER: macro, inizializza un mutex con valori di default.
*/
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER; //chiave per lock.

#define max_Byte 1000

void singalarm(int signo){
    pthread_mutex_lock(&log_mutex);

    struct stat info_file;

    if(stat(LOG_FILE, &info_file) == 0){
        if(info_file.st_size >= max_Byte){
            printf("%ld su %d\n", info_file.st_size, max_Byte);
            char buffer[100];
            time_t now = time(NULL);
            strftime(buffer, sizeof(buffer), "%d-%m-%Y_%H-%M-%S", localtime(&now));
            char nuovo_nome[150];
            snprintf(nuovo_nome, sizeof(nuovo_nome), "./logs/log_%s.txt", buffer);
            rename(LOG_FILE, nuovo_nome);
        }
        else{
            printf("%ld su %d , crea nuova file\n", info_file.st_size, max_Byte);
        }
    }
    else{
        printf("errore lettura byte file\n");
    }

    pthread_mutex_unlock(&log_mutex);
    alarm(5);
}

void write_log_dis(int id_mittente){
    //lock
    pthread_mutex_lock(&log_mutex);

    //crea timestamp e scrivi nel log
    FILE *f = fopen(LOG_FILE, "a");
    if (f){
        time_t now = time(NULL); //timestamp unix corrente
        char ts_buffer[64];
        strftime(ts_buffer, sizeof(ts_buffer), "%d-%m-%Y %H:%M:%S", localtime(&now)); //formatta secondo "%d-%m-%Y %H:%M:%S"
        fprintf(f, "[%s, %d, %s]\n", ts_buffer, id_mittente, "DISCONNECT");
        fclose(f);
    }
    else{
        perror("server fopen log fallito");
    }
    //unlock
    pthread_mutex_unlock(&log_mutex);
}

void write_log(int id_mittente, double dato){
    //lock
    pthread_mutex_lock(&log_mutex);

    //crea timestamp e scrivi nel log
        FILE *f = fopen(LOG_FILE, "a");
        if (f){
            time_t now = time(NULL); //timestamp unix corrente
            char ts_buffer[64];
            strftime(ts_buffer, sizeof(ts_buffer), "%d-%m-%Y %H:%M:%S", localtime(&now)); //formatta secondo "%d-%m-%Y %H:%M:%S"
            fprintf(f, "[%s, %d, %.2f]\n", ts_buffer, id_mittente, dato);
            fclose(f);
        }
        else{
            perror("server fopen log fallito");
        }
    //unlock
    pthread_mutex_unlock(&log_mutex);
}

void* handle_client(void* arg){
    int client_fd = *(int*) arg; //cast di arg in int, poi prendi il contenuto
    free(arg);  //libera memoria allocata in main, non serve

    //ricevi messaggio
    struct msg m;
    ssize_t n = recv(client_fd, &m, sizeof(m), 0);
    if (n == sizeof(m)){
        write_log(m.id_mittente, m.dato);
        char *ok="ok";
        ssize_t s = send(client_fd,ok,strlen(ok)+1,0);
        if(s>0){
            printf("messaggio mandato\n");
        }else{
            printf("sigpipe\n");
            write_log_dis(m.id_mittente);
        }

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




int main(void){
    //creazione socket lato server, sempre in ascolto
    signal(SIGPIPE, SIG_IGN);

    signal(SIGALRM, singalarm);
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET: usa ipv4, SOCK_STREAM: usa tcp, 0: protocollo scelto automaticamente
    if (listen_fd < 0) {
        perror("socket() fallita");
        exit(1);
    }
    printf("server socket creato correttamente. fd = %d\n", listen_fd);
    
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt SO_REUSEADDR fallita");
        exit(1);
    }
    //preparazione indirizzo socket
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET; //stesso protocollo (ipv4) usato per la creazione del socket
    addr.sin_addr.s_addr = INADDR_ANY;  // indirizzo ip su cui il server ascolterà. IDADDR_ANY = 0.0.0.0
    addr.sin_port = htons(PORT); //porta su cui server sta in ascolto. htons() = host to network short, converte la porta al formato della rete.

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

    alarm(5);

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
        
        //libera risorse thread dopo aver terminato
        pthread_detach(tid); 


    }

    
    close(listen_fd);
    return 0;
}
