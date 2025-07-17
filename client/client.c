#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(){
    //Creazione socket richiamando la funzione connessione_server()
    int client_socket = connessione_server();
    if(client_socket < 0){
        fprintf(stderr,"Impossibile connettersi al server.\n");
        return 1;
    }
    
    char buffer[DIM_BUFFER];
    char username[64],password[64];

    printf("Username: ");
    scanf("%63s", username);
    snprintf(buffer, sizeof(buffer), "%s\n", username);
    send(client_socket, buffer, strlen(buffer), 0);

    printf("Password: ");
    scanf("%63s", password);
    snprintf(buffer, sizeof(buffer), "%s\n", password);
    send(client_socket, buffer, strlen(buffer), 0);

// Ricevi esito
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("Server: %s", buffer);
    if (strstr(buffer, "Login riuscito") != NULL) {
        interazione_utente(client_socket);  
    } else {
        printf("Autenticazione fallita. Uscita.\n");
        close(client_socket);
        return 1;
    }

}