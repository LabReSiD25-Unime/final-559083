#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <pthread.h>

#define PORT 21 //Porta di ascolto server FTP

int inizializzazione_server(); //funzione principale server creazione socket ecc

void attesa_connessioni(int server_fd); //funzione che accetta le varie connessioni in entrata

void *gestione_client(void *arg); //funzione eseguita da un thread 

int autentica_utente(int client_fd); //funzione di login

#endif