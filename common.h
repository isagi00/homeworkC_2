/*
dati comuni condivisi
*/


#ifndef COMMON_H
#define COMMON_H
#define PORT 8888   //porta su cui coordinatore è in ascolto e produttore si connettono.
#define LOG_FILE "./logs/log.txt"


struct msg{
    int id_mittente;
    double dato;
};

#endif