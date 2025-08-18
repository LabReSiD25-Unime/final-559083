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
    struct sockaddr_in indirizzo; //struttura che contiene info di rete (ipv4,porta...)

    //Funzione socket per la creazione del socket TCP
    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("Errore creazione socket");
        return -1;
    }

    //configurazione indirizzo
    indirizzo.sin_family = AF_INET;
    indirizzo.sin_addr.s_addr = INADDR_ANY;//accetta connessioni da qualsiasi IP
    indirizzo.sin_port = htons(PORT);

    //Funzione per associare indirizzo e porta alla socket
    if(bind(server_fd,(struct sockaddr *)&indirizzo,sizeof(indirizzo)) < 0){
        perror("Errore bind");
        close(server_fd);
        return -1;
    }

    //Funzione per mettere in ascolto la nostra socket su più connessioni
    if(listen(server_fd,MAX_CONNESSIONI) < 0){
        perror("Errore listen");
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}

//Funzione per garantire l'accesso da parte di un utente
int autentica_utente(int client_fd) {
    char username[64] = {0};
    char password[64] = {0};
//Ricezione di username e password dal client
    recv(client_fd, username, sizeof(username), 0);
    username[strcspn(username, "\r\n")] = '\0';

    recv(client_fd, password, sizeof(password), 0);
    password[strcspn(password, "\r\n")] = '\0';
//Apertura file in lettura contenente le coppie username:password
    FILE *fp = fopen("Autentica.txt", "r");
    if (!fp) {
        perror("Errore apertura file utenti");
        return 0;
    }
//lettura del file riga per riga e confronto di username e password con strcmp
    char riga[128];
    while (fgets(riga, sizeof(riga), fp)) {
        char file_user[64], file_pass[64];
        if (sscanf(riga, "%[^:]:%s", file_user, file_pass) == 2) {
            if (strcmp(file_user, username) == 0 && strcmp(file_pass, password) == 0) {
                fclose(fp);
                send(client_fd, "230 Login riuscito.\n", 21, 0);
                return 1;
            }
        }
    }
//Se username e password non corrispondono login fallito
    fclose(fp);
    send(client_fd, "530 Login fallito.\n", 19, 0);
    return 0;
}



//Funzione che prende in input la socket e permette a più client di connettersi
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
        strncpy(sessione->directory_corrente,FTP_ROOT,PATH_MAX); //Impostiamo la directory corrente a ftp_root

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
    //buffer che conterrà i comandi
    char buffer[DIM_BUFFER];
    ssize_t n;//numero di byte ricevuti

    char *benvenuto = "220 Benvenuto nel nostro server FTP\n";
    send(sessione->client_fd,benvenuto,strlen(benvenuto),0);
    //se l'autenticazione fallisce la sessione termina
    if (!autentica_utente(sessione->client_fd)) {
        printf("Autenticazione fallita. Chiusura della connessione.\n");
        close(sessione->client_fd);
        free(sessione);
        pthread_exit(NULL);
    } else {
        printf("Utente autenticato correttamente.\n");
    }


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
    }
    close(sessione->client_fd);
    free(sessione);
    pthread_exit(NULL);
}