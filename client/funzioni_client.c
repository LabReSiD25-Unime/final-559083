#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

//Funzione  per la creazione della socket client e la connessione
int  connessione_server(){
    int client_socket;
    struct sockaddr_in server_addr;

    //Creazione socket
    if((client_socket = socket(AF_INET,SOCK_STREAM,0)) < 0){
        perror("Errore creazione socket client");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port =  htons(SERVER_PORT);

    //Funzione per verificare se l'indirizzo ip è convertibile in formato binario
    if (inet_pton(AF_INET,SERVER_IP, &server_addr.sin_addr) <= 0){
        perror("Indirizzo server non valido");
        close(client_socket);
        return -1;
    }

    //Connessione client-server
    if(connect(client_socket, (struct sockaddr *)&server_addr,sizeof(server_addr)) < 0){
        perror("Errore connessione");
        close(client_socket);
        return -1;
    }

    printf("Connessione al server FTP riuscita\n");
    ricevi_risposta(client_socket);
    return client_socket;

}

//Funzione per l'inoltro dei comandi al server
void interazione_utente(int client_socket){
    char buffer[DIM_BUFFER];

    while(1){
        printf("Comandi disponibili: CWD,LIST,RETR,STOR,QUIT\n Inserisci comando: ");
        fflush(stdout);

        //Lettura del comando all'interno del buffer
        if(!fgets(buffer,DIM_BUFFER,stdin)) break;

        //Inseriamo carattere di terminazione nel buffer
        buffer[strcspn(buffer,"\n")] = "\0";

        //Inoltriamo il comando da eseguire
        if(send(client_socket, buffer,strlen(buffer),0) <= 0){
            printf("Errore durante l'invio del comando");
            break;
        }

        if(strncmp(buffer,"QUIT",4) == 0){
            printf("Disconnessione");
            break;
        }

        ricevi_risposta(client_socket);
    }
}

//Funzione per ricevere dati dal server
void ricevi_risposta(int client_socket){
    char risposta[DIM_BUFFER];
    
    //Funzione per ricevere i dati all'interno del buffer dal server
    ssize_t ricevuti = recv(client_socket,risposta,DIM_BUFFER - 1, 0);

    if(ricevuti > 0){
        risposta[ricevuti] = "\0";
        printf("%s",risposta);
    }else{
        printf("Errore nella ricezione della risposta\n");
    }
}