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
#define MAX_CONNESSIONI 3

int inizializzazione_server(){
    int server_fd;
    struct sockaddr_in indirizzo;//struttura che contiene info di rete (ipv4,porta...)

    //Funzione socket per la creazione del socket TCP
    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("Errore creazione socket");
        return -1;
    }

    //configurazione indirizzo
    indirizzo.sin_family = AF_INET;
    indirizzo.sin_addr.s_addr = INADDR_ANY;//accetta connessioni da qualsiasi IP
    indirizzo.sin_port = htons(PORT);

    //Funzine per associare indirizzo e porta alla socket
    if(bind(server_fd,(struct sockaddr *)&indirizzo,sizeof(indirizzo)) < 0){
        perror("Errore bind");
        close(server_fd);
        return -1;
    }

    //Funzione per mettere in ascolto la nostra socket su piu connessioni
    if(listen(server_fd,MAX_CONNESSIONI) < 0){
        perror("Errore listen");
        close(server_fd);
        return -1;
    }

    printf("server in ascolto sulla porta : %d\n",PORT);
    return server_fd;
}

//Funzione che prende in input la socket e permette a piu client di connettersi
void attesa_connessioni(int server_fd){
    //loop infinito
    while(1){
        struct sockaddr_in indirizzo_client;
        socklen_t lunghezza = sizeof(indirizzo_client);
        //funzione accept che ritorna la socket client per garantire la comunicazione
        int client_fd = accept(server_fd,(struct sockaddr *)&indirizzo_client,&lunghezza);

        if(client_fd < 0){
            perror("errore accept");
            continue;
        }

        printf("Nuova connessione da %s : %d\n",inet_ntoa(indirizzo_client.sin_addr),ntohs(indirizzo_client.sin_port));

        //Allocazione dinamica di memoria per la sessione tra client-server
        Sessione *sessione = malloc(sizeof(Sessione));
        if(!sessione){
            fprintf(stderr, "Errore malloc\n");
            close(client_fd);
            continue;
        }
        //impostazione campi sessione...
        sessione->client_fd = client_fd;
        strncpy(sessione->directory_corrente,FTP_ROOT,PATH_MAX);//Impostiamo la directory corrente a ftp_root

        //Creazione di un nuovo thread per gestire il client
        pthread_t thread_id;
        //Ogni thread eseguirà la funzione gestione_client per eseguire i vari comandi
        if(pthread_create(&thread_id,NULL,gestione_client,(void *)sessione) != 0){
            perror("Errore creazione thread");
            close(client_fd);
            free(sessione);
            continue;
        }

        pthread_detach(thread_id); //libera le risorse inerenti al thread dopo la sua esecuzione.

    }
}


void *gestione_client(void *arg){
    //Cast dell'argomento in tipo Sessione
    Sessione *sessione = (Sessione *)arg;
    char buffer[DIM_BUFFER];//buffer che conterrà i comandi
    ssize_t n;//numero di byte ricevuti

    char *benvenuto = "220 Benvenuto nel server FTP\n";
    send(sessione->client_fd,benvenuto,strlen(benvenuto),0);

    //loop infinito per gestire i comandi del client finchè non si disconnette
    while(1){
        memset(buffer,0,DIM_BUFFER);
        //Popolazione buffer mediante recv()
        n = recv(sessione->client_fd,buffer,DIM_BUFFER -1,0);

        //se la connessione è stata chiusa (=0) o ci sono stati errori (<0) il client si disconnette
        if(n <= 0){
            printf("Disconnessione client : %d", sessione->client_fd);
            break;
        }

        //Pulizia di eventuali terminatori di riga
        buffer[strcspn(buffer, "\n")] = '\0';
        printf("Comando ricevuto  %d: %s\n", sessione->client_fd, buffer);
       
        //Richiama la funzione per la gestione dei comandi
        gestisci_comando(sessione,buffer);

        if(strncmp(buffer,"QUIT",4) == 0){
            break;
        }
    }
    close(sessione->client_fd);
    free(sessione);
    pthread_exit(NULL);
}