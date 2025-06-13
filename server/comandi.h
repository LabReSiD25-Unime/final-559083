#ifndef COMANDI_H
#define COMANDI_H
#include <limits.h> // per PATH_MAX (inerente al massimo percorso di cartelle)
#define FTP_ROOT "./ftp_root"

typedef struct {
    int client_fd;
    char directory_corrente[PATH_MAX];
}Sessione;


void gestisci_comando(Sessione *sessione, const char *comando);

#endif COMANDI_H
