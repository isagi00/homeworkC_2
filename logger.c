/*
funzioni riguardanti il file di log

logger_init(): apre/crea il file di log iniziale
logger_write(id_mittente, dato): acquisisce lock, genera timestamp, scrive la riga in append, rilascia il lock
logger_check_and_rotate(): controlla la dimensione (stat) e se serve rinomina/archivia il file corrente e ne apre uno nuovo 
    (chiamata dal main in risposta al flag di SIGALARM)
logger_close(): chiude il file (usato nel shutdown da SIGNINT)
*/