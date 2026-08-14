# homework C 2

sistema in cui dati provenienti da più sorgenti sono inviati a un aggregatore, che scrive i dati su un file di log.
l'aggregatore/coordinatore svolge il ruolo di un server tcp in ascolto su una specifica porta, mentre i produttori svolgono il ruolo di client tcp.
per la gestione di client concorrenti sono stati usati thread: server crea un nuovo thread per ogni nuovo client che si connette.

è stata effettuata l'implementazione di:
    -socket
    -thread
    -lock (mutex)
    -SIGALARM (usato principalmente per archiviazione file di log)
    -SIGPIPE
    -SIGINT
    -riuso rapido di IP:PORT (SO_REUSEADDR)

---

compilazione: 
    make 
    make coordinator
    make producer

avviare il server:
    ./coordinator 

avviare client:
    ./producer id_mittente dato [--no-ack]

file di log si trovano in ./logs

