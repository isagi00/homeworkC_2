/*
funzioni riguardanti il file di log

logger_init(): apre/crea il file di log iniziale
logger_write(id_mittente, dato): acquisisce lock, genera timestamp, scrive la riga in append, rilascia il lock
logger_check_and_rotate(): controlla la dimensione (stat) e se serve rinomina/archivia il file corrente e ne apre uno nuovo 
    (chiamata dal main in risposta al flag di SIGALRM)
logger_close(): chiude il file (usato nel shutdown da SIGINT)
*/

#include "common.h"
#include "logger.h"

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#include <pthread.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static FILE * log_fp = NULL;

#define MAX_LOG_SIZE_BYTES 1000

static void get_timestamp(char* buffer, size_t buffer_size){
    time_t now = time(NULL);
    strftime(buffer, buffer_size, "%d-%m-%Y %H:%M:%S", localtime(&now));
}

int logger_init(void){
    pthread_mutex_lock(&log_mutex);
    log_fp = fopen(LOG_FILE, "a");
    pthread_mutex_unlock(&log_mutex);
    return log_fp ? 0 : -1;
}

void logger_write(int id_mittente, double dato){
    pthread_mutex_lock(&log_mutex);

    if (log_fp){
        printf("[logger] scrittura dati in log in corso... \n");
        char ts_buffer[64];
        get_timestamp(ts_buffer, sizeof(ts_buffer));
        fprintf(log_fp, "[%s, %d, %.2f]\n", ts_buffer, id_mittente, dato);
        fflush(log_fp);
    }
    else{
        fprintf(stderr, "logger: tentativo scrittura con file non aperto \n");
    }

    pthread_mutex_unlock(&log_mutex);
}

void logger_write_event(int id_mittente, const char* evento){
    pthread_mutex_lock(&log_mutex);

    if (log_fp){
        printf("[logger] scrittura evento in log in corso... \n");
        char ts_buffer[64];
        get_timestamp(ts_buffer, sizeof(ts_buffer));
        fprintf(log_fp, "[%s, %d, %s]\n", ts_buffer, id_mittente, evento);
        fflush(log_fp);
    }
    else{
        fprintf(stderr, "logger: tentativo scrittura con file non aperto \n");
    }

    pthread_mutex_unlock(&log_mutex);
}

void logger_check_and_rotate(void){
    pthread_mutex_lock(&log_mutex);

    struct stat info_file;

    if (stat(LOG_FILE, &info_file) == 0){
        if (info_file.st_size >= MAX_LOG_SIZE_BYTES){

            printf("[logger] dimensione massima superata, creando nuovo log... \n");
            // chiudi il file attualmente aperto
            if (log_fp){
                fclose(log_fp);
                log_fp = NULL;
            }

            //archivia il file
            char buffer[100];
            time_t now = time(NULL);
            strftime(buffer, sizeof(buffer), "%d-%m-%Y_%H-%M-%S", localtime(&now));
            char nuovo_nome[150];
            snprintf(nuovo_nome, sizeof(nuovo_nome), "./logs/log_%s.txt", buffer);
            rename(LOG_FILE, nuovo_nome);

            // riapri un nuovo log.txt vuoto, per le prossime scritture
            log_fp = fopen(LOG_FILE, "a");
        }
    }
    else{
        fprintf(stderr, "logger: stat fallita\n");
    }

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