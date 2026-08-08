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

#include <sys/socket.h> //creazione socket, bind, listen ... 
#include <stdio.h>  //util
#include <stdlib.h> //util
#include <unistd.h> //util
#include <netinet/in.h> //preparazione indirizzo
#include <time.h> //per timestamp

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


        //recv(), riceve il messaggio msg dal client_fd
        struct msg m;
        ssize_t n = recv(client_fd, &m, sizeof(m), 0);
        if (n == sizeof(m)){    //tutto ok
            printf("ricevuto messaggio da client fd = %d \n", client_fd);
            //crea timestamp e scrivi nel log
            FILE *f = fopen(LOG_FILE, "a");
            if (f){
                time_t now = time(NULL); //timestamp unix corrente
                char ts_buffer[64];
                strftime(ts_buffer, sizeof(ts_buffer), "%d-%m-%Y %H:%M:%S", localtime(&now)); //formatta secondo "%d-%m-%Y %H:%M:%S"
                fprintf(f, "[%s, %d, %.2f]\n", ts_buffer, m.id_mittente, m.dato);
                fclose(f);
            }
            else{
                perror("server fopen log fallito");
            }
        }
        else if (n == 0) {  //client disconnesso
            printf("client fd = %d disconnesso senza inviare dati\n", client_fd);
        }
        else if (n < 0){
            printf("server revc fallito o dati parziali \n");
        }

        close(client_fd);   //chiusura connessione di client fd
    }


    close(listen_fd);
    return 0;
}


