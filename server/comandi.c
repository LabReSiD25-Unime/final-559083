#include "comandi.h"
#include <string.h>
// Inclusioni delle librerie standard e di sistema necessarie per le operazioni
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>    
#include <ctype.h>     
#include <dirent.h>     // Funzioni per la lettura delle directory (opendir, readdir, closedir)
#include <sys/stat.h>   // Funzioni per ottenere informazioni sui file (stat) e controllare se è una directory
#include <fcntl.h>   
#include <sys/types.h>
#include <sys/socket.h>


#define DIM_BUFFER 1024

void gestisci_comando(Sessione *sessione, const char *comando){
    char copia[DIM_BUFFER];
    //copiamo il comando ricevuto nel buffer
    strncpy(copia,comando, DIM_BUFFER -1);
    copia[DIM_BUFFER - 1] = '\0';

    /*suddividiamo la stringa (cmd) in token in cui il primo token rappresenterà il comando vero e proprio 
      mentre il secondo (arg) conterrà l'argomento se inserito es:directory..
    */

    char *cmd = strtok(copia, " ");
    char *arg = strtok(NULL,"");

    //Se il comando inoltrato non è riconosciuto o non esiste allora verranno stabiliti i relativi log
    if(!cmd){
        fprintf(stderr, "Errore: nessun comando ricevuto, Input: '%s'\n", comando);
        char msg[] = "500 Comando non riconosciuto.\n";
        send(sessione->client_fd, msg,strlen(msg),0);
        return;
    }
    //Se il comando ricevuto è QUIT allora il client si disconnette
    if(strcmp(cmd, "QUIT") == 0){
        char msg[] = "221 Arrivederci. \n";
        send(sessione->client_fd,msg,strlen(msg),0);
    }
    //Altrimenti, se il comando è CWD:
    else if(strcmp(cmd,"CWD") == 0){
        //se non vi è argomento allora si inoltra al client la spiegazione d'uso
        if(arg == NULL){
            char msg[] = "501 Utilizzo comando: CWD <directory> \n";
            send(sessione->client_fd, msg, strlen(msg),0);
        }
        //altrimenti se si vuole risalire alla directory superiore utilizziamo il comando ".."
        else if (strcmp(arg, "..") == 0){
            //se la directory in cui ci troviamo è FTP_root non possiamo effettuare la risalita
            if(strcmp(sessione->directory_corrente, FTP_ROOT) == 0){
                char msg[] = "550 Non puoi uscire dalla directory principale FTP_root";
                send(sessione->client_fd, msg, strlen(msg),0);
                /*altrimenti risaliamo di una directory  eliminando l'ultimo '/' e tutto ciò che segue,
                 lasciando solo il percorso della directory superiore.*/
            }else{
                char *p = strrchr(sessione->directory_corrente, '/'); //cerca l'ultima occorrenza di /
                if(p != NULL)
                    *p = '\0';
                if(strcmp(sessione->directory_corrente, "") == 0) //se la directory è vuota viene impostata a FTP_ROOT
                    strncpy(sessione->directory_corrente, FTP_ROOT,PATH_MAX);
                char msg[DIM_BUFFER];
                snprintf(msg,DIM_BUFFER,"250 Directory cambiata a %s\n", sessione->directory_corrente);
                send(sessione->client_fd,msg,strlen(msg),0);
            }

        }
        //Se l'argomento passato è un percorso assoluto passiamo al carattere successivo allo /, altrimenti rimarrà invariato
        else{
            char pulisci_argomento[PATH_MAX];
            if(arg[0] == '/'){
                snprintf(pulisci_argomento, PATH_MAX, "%s", arg +1);
            } else{
                snprintf(pulisci_argomento,PATH_MAX,"%s",arg);
            }
            char nuovoPercorso[PATH_MAX];
            snprintf(nuovoPercorso,PATH_MAX,"%s/%s",sessione->directory_corrente,pulisci_argomento);
            //se stat restituisce 0 allora il percorso esiste, la MACRO S_ISDIR controlla se il percorso è una directory
            struct stat info;
            if(stat(nuovoPercorso,&info) == 0 && S_ISDIR(info.st_mode)){
                strncpy(sessione->directory_corrente,nuovoPercorso,PATH_MAX);
                char msg[DIM_BUFFER];
                snprintf(msg,DIM_BUFFER,"250 Directory cambiata a : %s\n",sessione->directory_corrente);
                send(sessione->client_fd,msg,strlen(msg),0);
            }
            //altrimenti se il percorso non esiste
            else{
               fprintf(stderr, "Errore  directory non trovata, Input: %s",comando);
               char msg[] = "550 Directory non trovata\n";
               send(sessione->client_fd,msg,strlen(msg),0);
            }
        }
    }
    //Se il comando passato è LIST vengono ritornate le directory ed i relativi file
    else if(strcmp(cmd,"LIST") == 0){
        DIR *dir = opendir(sessione->directory_corrente); //Apre la directory grazie alla funzione opendir() dalla libreria dirent.h
        if(dir == NULL){//se non è stata trovata alcuna directory
            fprintf(stderr, "Errore: impossibile leggere la directory corrente. Input: '%s'\n", comando);
            char msg[] = "550 Impossibile leggere la directory. \n";
            send(sessione->client_fd,msg,strlen(msg),0);
            return;
        }
        char msg_inizio[] = "150 Inizio elenco-- \n";
        send(sessione->client_fd,msg_inizio,strlen(msg_inizio),0);
        struct dirent *elemento;
        char lista[DIM_BUFFER];
        while((elemento = readdir(dir)) != NULL){//finchè vengono lette directory
            if(strcmp(elemento->d_name, ".") == 0 ||strcmp(elemento->d_name,"..") == 0 )//Se la directory è figlia (.) oppure padre (..) continua a leggere
                continue;
            snprintf(lista, DIM_BUFFER, "%s\n", elemento->d_name);
            send(sessione->client_fd, lista, strlen(lista), 0);    
        }
        closedir(dir);//chiudiamo la directory
        char msg_fine[] = "226 Fine elenco.\r\n";
        send(sessione->client_fd, msg_fine, strlen(msg_fine), 0);
    }
    //Se il comando ricevuto è RETR viene scaricato un file inoltrato dal client
    else if(strcmp(cmd,"RETR") == 0){
        if(arg == NULL){//se non viene fornito alcun argomento si spiega l'uso del comando
            char msg[] = "501 Uso: RETR <nome_file>\n";
            send(sessione->client_fd, msg, strlen(msg), 0);
        }else{
                char percorso[PATH_MAX];//salvo il path
                snprintf(percorso, PATH_MAX, "%s/%s", sessione->directory_corrente, arg); //inserisce il path formato dalla dir corrente e l'argomento passato
                FILE *file = fopen(percorso,"rb");//apriamo il file in modalità 'rb'
                if(file ==NULL){
                    char msg[] = "550 File non trovato.\n";
                    send(sessione->client_fd, msg, strlen(msg), 0);
                    return;
                }
                char msg_inizio[] = "150 Inizio download...\n";
                send(sessione->client_fd, msg_inizio, strlen(msg_inizio), 0);
                char dati[DIM_BUFFER];
                size_t file_letti;
                while ((file_letti = fread(dati, 1, DIM_BUFFER, file)) > 0) {//con fread leggiamo i file e salviamo il contenuto in dati
                    send(sessione->client_fd, dati, file_letti, 0);
                }
                fclose(file);
                char msg_fine[] = "\n226 Download completato.\n";
                send(sessione->client_fd, msg_fine, strlen(msg_fine), 0);
            }

    }
    //Se il comando è STOR viene caricato un file inoltrato dal client
    else if(strcmp(cmd,"STOR") == 0){
        if (arg == NULL) {//se non vi è alcun file viene specificato il caso d'uso
            char msg[] = "501 Uso: STOR <nome_file>\n";
            send(sessione->client_fd, msg, strlen(msg), 0);
        } else {
                char percorso[PATH_MAX];//si identifica il percorso
                snprintf(percorso, PATH_MAX, "%s/%s", sessione->directory_corrente, arg);//inserisce il path formato dalla dir e arg
                FILE *file = fopen(percorso,"wb");//apriamo il file in modalità wb
                if(file == NULL){
                        char msg[] = "550 Impossibile creare il file.\n";
                        send(sessione->client_fd, msg, strlen(msg), 0);
                        return;
                }
                char msg_inizio[] = "150 Inizio caricamento, invia dati e termina con <EOF> su una linea.\n";
                send(sessione->client_fd, msg_inizio, strlen(msg_inizio), 0);
                char buffer_dati[DIM_BUFFER];//buffer per ricevere i dati dal client
                int lunghezza;
                while ((lunghezza = recv(sessione->client_fd, buffer_dati, DIM_BUFFER - 1, 0)) > 0) { //finchè ci sono dati da leggere
                buffer_dati[lunghezza] = '\0';
                if (strncmp(buffer_dati, "<EOF>", 5) == 0) //se il client ha inviato <EOF> interrompiamo il programma
                    break;
                fwrite(buffer_dati, 1, lunghezza, file);//in seguito scriviamo i dati ricevuti
            }
            fclose(file);
            char msg_fine[] = "226 caricamento completato.\n";
            send(sessione->client_fd, msg_fine, strlen(msg_fine), 0);

        }
    }else{
            char msg[] = "502 Comando non implementato.\n";
            send(sessione->client_fd, msg, strlen(msg), 0);
    }
}