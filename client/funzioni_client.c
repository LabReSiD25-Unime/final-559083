#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <ctype.h>

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
    int in_upload = 0; // flag per modalità upload

    while(1){
        if (!in_upload) {
            printf("Comandi disponibili: CWD,LIST,RETR,STOR,QUIT\n Inserisci comando: ");
            fflush(stdout);

            if(!fgets(buffer,DIM_BUFFER,stdin)) break;
            buffer[strcspn(buffer,"\n")] = '\0';

            if(send(client_socket, buffer, strlen(buffer), 0) <= 0){
                printf("Errore durante l'invio del comando\n");
                break;
            }

            // Leggo risposta server
            ricevi_risposta(client_socket);

            if(strncmp(buffer,"QUIT",4) == 0){
                char msg[100];
                snprintf(msg, sizeof(msg), "Disconnessione client:%d", client_socket);
                send(client_socket, msg, strlen(msg), 0);
                break;
            }

            // Se il comando era STOR <file>, passo in modalità upload dati
            if(strncmp(buffer, "STOR ", 5) == 0) {
                in_upload = 1;
                printf("Inserisci i dati da caricare. Termina con <EOF> su una riga.\n");
            }

        } else {
            // Modalità upload dati: invio dati finché non ricevo <EOF>
            if(!fgets(buffer,DIM_BUFFER,stdin)) break;

            // se linea è <EOF>, invio e esco dalla modalità upload
            if(strcmp(buffer, "<EOF>\n") == 0 || strcmp(buffer, "<EOF>") == 0) {
                send(client_socket, "<EOF>", 5, 0);
                in_upload = 0;
                // Leggo la risposta finale dal server
                ricevi_risposta(client_socket);
            } else {
                send(client_socket, buffer, strlen(buffer), 0);
            }
        }
    }
}


//Funzione per ricevere dati dal server
void ricevi_risposta(int client_socket) {
    char buffer[DIM_BUFFER + 1];
    int n;
    int fine_risposta = 0;
    fd_set read_fds;
    struct timeval tv;

    while (!fine_risposta) {
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds);

        // Timeout di 2 secondi per evitare blocco infinito
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int rv = select(client_socket + 1, &read_fds, NULL, NULL, &tv);
        if (rv == -1) {
            perror("select");
            break;
        } else if (rv == 0) {
            // Timeout: nessun dato arrivato, esco comunque
            break;
        }

        if (FD_ISSET(client_socket, &read_fds)) {
            n = recv(client_socket, buffer, DIM_BUFFER, 0);
            if (n <= 0) {
                printf("Connessione chiusa o errore nella ricezione\n");
                break;
            }

            buffer[n] = '\0';
            printf("%s", buffer);

            // Verifica se inizia una linea con codice di risposta di fine (es: 226, 250, 221, 550)
            // Controlla solo le prime 4 caratteri: 3 numeri + spazio o trattino
            if (n >= 4 && ((buffer[0] == '2' || buffer[0] == '5' || buffer[0] == '4' || buffer[0] == '1') &&
                 isdigit(buffer[1]) && isdigit(buffer[2]) &&
                 (buffer[3] == ' ' || buffer[3] == '\r' || buffer[3] == '\n'))) {
                // Se codice è uno di quelli finali:
                if (strncmp(buffer, "226", 3) == 0 ||
                    strncmp(buffer, "250", 3) == 0 ||
                    strncmp(buffer, "221", 3) == 0 ||
                    strncmp(buffer, "550", 3) == 0) {
                    fine_risposta = 1;
                }
            }
        }
    }
}

