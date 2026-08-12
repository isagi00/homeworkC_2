CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

all: coordinator producer

coordinator: coordinator.c worker.c logger.c worker.h logger.h common.h
	$(CC) $(CFLAGS) -o coordinator coordinator.c worker.c logger.c


producer: producer.c common.h
	$(CC) $(CFLAGS) -o producer producer.c 


clean: 
	rm -f coordinator producer *.o