#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>

extern int current_client_id;  

void* handle_client(void* arg);

#endif