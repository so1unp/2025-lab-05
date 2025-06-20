// client_status.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include "status.h"

int main(void) {
    // Abrir la cola donde vienen tanto estados como eventos
    int id_cola_status = msgget(MAILBOX_STATUS_KEY, 0666);
    if (id_cola_status == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("Cliente de estado: escuchando en id_cola_status=%d\n\n", id_cola_status);

    struct status_msg status;
    while (1) {
        // Recibir el siguiente mensaje, sea del tipo que sea, evento o estado
        // Usamos 0 como tipo para recibir cualquier mensaje, si queremos recibir un estado o evento se pondria el 1 o 2
        if (msgrcv(id_cola_status, &status, sizeof(status) - sizeof(long), 0, 0) != -1) {
            if (status.mtype == TYPE_SERVER_STATUS) {
                // Mensaje de estado de conexión/juego
                printf("[ESTADO] código=%d, mensaje=\"%s\"\n",
                       status.code, status.text);
            }
            else if (status.mtype == TYPE_GAME_EVENT) {
                // Mensaje de evento 
                printf("[EVENTO] código=%d, mensaje=\"%s\"\n",
                       status.code, status.text);
            }
            else {
                // Mensaje inesperado
                printf("[OTRO] mtype=%ld, código=%d, mensaje=\"%s\"\n",
                       status.mtype, status.code, status.text);
            }
        } else {
            // Si falla la recepción, esperamos un poco antes de reintentar
            perror("msgrcv");
            sleep(3);
        }
    }

    return 0;
}
