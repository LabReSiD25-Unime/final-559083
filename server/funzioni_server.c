#include "server.h"        
#include "comandi.h"       
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define DIM_BUFFER 1024
#define MAX_CONNESSIONI 2

int inizializzazione_server(){
    int server_fd;
    struct sockaddr_in indirizzo;

    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("Errore creazione socket");
        return -1;
    }

    indirizzo.sin_family = AF_INET;
    indirizzo.sin_addr.s_addr = INADDR_ANY;
    indirizzo.sin_port = htons(PORT);

    if(bind(server_fd,(struct sockaddr *)&indirizzo,sizeof(indirizzo) < 0)){
        perror("Errore bind");
        close(server_fd);
        return -1;
    }

    if(listen(server_fd,MAX_CONNESSIONI) < 0){
        perror("Errore listen");
        close(server_fd);
        return -1;
    }

    printf("server in ascolto sulla porta : %d\n",PORT);
    return server_fd;
}

void attesa_connessioni(int server_fd){
    while(1){
        struct sockaddr_in indirizzo_client;
        socklen_t lunghezza = sizeof(indirizzo_client);
        int client_fd = accept(server_fd,(struct sockaddr *)&indirizzo_client,&lunghezza);

        if(client_fd < 0){
            perror("errore accept");
            continue;
        }

        printf("Nuova connessione da %s : %d\n",inet_ntoa(indirizzo_client.sin_addr),ntohs(indirizzo_client.sin_port));

        Sessione *sessione = malloc(sizeof(Sessione));
        if(!sessione){
            fprintf(stderr, "Errore malloc\n");
            close(client_fd);
            continue;
        }
        sessione->client_fd = client_fd;
        strncpy(sessione->directory_corrente,FTP_ROOT,PATH_MAX);

        pthread_t thread_id;
        if(pthread_create(&thread_id,NULL,gestione_client,(void *)sessione) != 0){
            perror("Errore creazione thread");
            close(client_fd);
            free(sessione);
            continue;
        }

        pthread_detach(thread_id); //libera le risorse inerenti al thread dopo la sua esecuzione.

    }
}