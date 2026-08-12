

#ifndef LOGGER_H
#define LOGGER_H

int logger_init(void);
void logger_write(int id_mittente, double dato);
void logger_write_event(int id_mittente, const char * evento);
void logger_close(void);


#endif