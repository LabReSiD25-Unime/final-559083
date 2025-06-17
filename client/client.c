#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
    //Creazione socket richiamando la funzione connessione_server()
    int client_socket = connessione_server();
    if(client_socket < 0){
        fprintf(stderr,"Impossibile connettersi al server.\n");
        return 1;
    }

    interazione_utente(client_socket);

    close(client_socket);
    return 0;
}