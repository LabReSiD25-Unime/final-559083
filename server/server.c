#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "server.h"

int main(){

    int server_fd = inizializzazione_server();
    printf("server FTP in ascolto sulla porta : %d\n", PORT);

    attesa_connessioni(server_fd);

    return 0;
}