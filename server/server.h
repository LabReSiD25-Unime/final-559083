#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <pthread.h>

#define PORT 2121 //Porta di ascolto server FTP

int inizializzazione_server();//funzione principale server creazione socket ecc

void attesa_connessioni(int server_fd);

void *gestione_client(void *arg); //funzione eseguita da un thread per gestire la comunicazione con un client

#endif SERVER_H