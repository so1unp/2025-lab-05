// server_status.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include "status.h"

int main() {
    // Crear o abrir la cola de mensajes
    int id_cola_status = msgget(MAILBOX_STATUS_KEY, IPC_CREAT | 0666);
    if (id_cola_status == -1) {
        perror("msgget");
        exit(1);
    }

    printf("Servidor de estado listo (id_cola_status=%d)\n", id_cola_status);

    struct status_msg msg;
    int indice_mensaje = 0;

    while (1) {
        if (indice_mensaje == 0) {
            // Enviar estado de conexión OK
            msg.mtype = TYPE_SERVER_STATUS;
            msg.code  = ST_SERVER_OK;
            strncpy(msg.text, "Servidor ACTIVO", MAX_MSG-1);
            msg.text[MAX_MSG-1] = '\0';
        }
        else if (indice_mensaje == 1) {
            // Enviar evento de juego: explorador conectado
            msg.mtype = TYPE_GAME_EVENT;
            msg.code  = ST_CONNECTED;
            strncpy(msg.text, "Explorador conectado", MAX_MSG-1);
            msg.text[MAX_MSG-1] = '\0';
        }
        else if (indice_mensaje == 2) {
            // Enviar evento de juego: tesoro encontrado
            msg.mtype = TYPE_GAME_EVENT;
            msg.code  = ST_TREASURE_FOUND;
            strncpy(msg.text, "¡Tesoro encontrado!", MAX_MSG-1);
            msg.text[MAX_MSG-1] = '\0';
        }
        else if (indice_mensaje == 3) {
            // Enviar estado de conexión ERROR
            msg.mtype = TYPE_SERVER_STATUS;
            msg.code  = ST_SERVER_ERROR;
            strncpy(msg.text, "Servidor CAÍDO", MAX_MSG-1);
            msg.text[MAX_MSG-1] = '\0';
        }
        else if (indice_mensaje == 4) {
            // Enviar evento de juego: fin de la partida
            msg.mtype = TYPE_GAME_EVENT;
            msg.code  = ST_GAME_OVER;
            strncpy(msg.text, "La partida ha terminado", MAX_MSG-1);
            msg.text[MAX_MSG-1] = '\0';
        }

        // Avanzar al siguiente paso (ciclo de 0 a 4)
        indice_mensaje = (indice_mensaje + 1) % 5;

        // Enviar el mensaje
        if (msgsnd(id_cola_status, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
            perror("msgsnd");
        }

        sleep(3);
    }

    return 0;
}
