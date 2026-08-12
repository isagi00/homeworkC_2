/*
funzioni riguardanti il file di log

logger_init(): apre/crea il file di log iniziale
logger_write(id_mittente, dato): acquisisce lock, genera timestamp, scrive la riga in append, rilascia il lock
logger_check_and_rotate(): controlla la dimensione (stat) e se serve rinomina/archivia il file corrente e ne apre uno nuovo 
    (chiamata dal main in risposta al flag di SIGALARM)
logger_close(): chiude il file (usato nel shutdown da SIGNINT)
*/

#include "common.h"
#include "logger.h"

#include <stdio.h>
#include <time.h>

#include <pthread.h>

/*
pthread_mutex_t: tipo di dato, mutex che permette a un solo thread alla volta di accedere in una sezione critica.
PTHREAD_MUTEX_INITIALIZER: macro, inizializza un mutex con valori di default.
*/
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER; //chiave per lock.

static FILE * log_fp = NULL; //puntatore al file di log

static void get_timestamp(char* buffer, size_t buffer_size){
    time_t now = time(NULL);
    strftime(buffer, buffer_size, "%d-%m-%Y %H:%M:%S", localtime(&now));    //formatta secondo %d-%m-%Y %H:%M:%S
}

int logger_init(void){
    pthread_mutex_lock(&log_mutex);
    log_fp = fopen(LOG_FILE, "a");
    pthread_mutex_unlock(&log_mutex);
    return log_fp ? 0 : -1;
}

void logger_write(int id_mittente, double dato){
    //lock
    pthread_mutex_lock(&log_mutex);

    //crea timestamp e scrivi nel log
    // FILE *f = fopen(LOG_FILE, "a")
    if (log_fp){
        char ts_buffer[64];
        get_timestamp(ts_buffer, sizeof(ts_buffer));
        fprintf(log_fp, "[%s, %d, %.2f]\n", ts_buffer, id_mittente, dato);
        fflush(log_fp);
        // fclose(f);
    }
    else{
        fprintf(stderr, "logger: tentativo scrittura con file non aperto \n");
    }
    
    //unlock
    pthread_mutex_unlock(&log_mutex);
}


void logger_write_event(int id_mittente, const char* evento){
    //lock
    pthread_mutex_lock(&log_mutex);

    //crea timestamp e scrivi nel log
    // FILE *f = fopen(LOG_FILE, "a");
    if (log_fp){
        char ts_buffer[64];
        get_timestamp(ts_buffer, sizeof(ts_buffer));
        fprintf(log_fp, "[%s, %d, %s]\n", ts_buffer, id_mittente, evento);
        fflush(log_fp);
        // fclose(f);
    }
    else{
        perror("server fopen log fallito");
    }
    //unlock
    pthread_mutex_unlock(&log_mutex);
}

void logger_close(void){
    pthread_mutex_lock(&log_mutex);
    if (log_fp){
        fclose(log_fp);
        log_fp = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}