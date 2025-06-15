#include "comandi.h"

// Inclusioni delle librerie standard e di sistema necessarie per le operazioni
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>    
#include <ctype.h>     
#include <dirent.h>     // Funzioni per la lettura delle directory (opendir, readdir, closedir)
#include <sys/stat.h>   // Funzioni per ottenere informazioni sui file (stat) e controllare se è una directory
#include <fcntl.h>      

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
        send(sessione->client_fd, msg,strln(msg),0);
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
}