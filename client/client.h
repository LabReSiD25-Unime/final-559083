#ifndef CLIENT_H
#define CLIENT_H

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2121
#define DIM_BUFFER 4096

int connessione_server();//Crea la socket del client e si connette al server
void interazione_utente(int client_socket);//Inoltrazione comandi al server
void ricevi_risposta(int client_socket);//Ricezione risposte dal server

#endif 