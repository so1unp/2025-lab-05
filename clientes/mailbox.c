#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "../directorio/directorio.h"

int main()
{
    int mailbox_respuestas_id;
    struct respuesta resp;
    pid_t mi_pid = getpid();

    // se conecta al mailbox de respuestas
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0666);
    if (mailbox_respuestas_id == -1)
    {
        perror("Error al conectar con el mailbox de respuestas");
        return 1;
    }

    if (msgrcv(mailbox_respuestas_id, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1)
    {
        {
            perror("Error al recibir resp");
            return 1;
        }

        printf("Codigo de respuesta: %d\n", resp.codigo);
        printf("Mensaje: %s\n", resp.datos);

        return EXIT_SUCCESS;
    }
}
void ejemplo()
{
    int pid_del_jugador;
    int RESP_GAME_OVER;
    int mailbox_respuestas_id;
    
    // lo que tendria q hacer ma o meno el servidor para enviarnos el mensaje
    struct respuesta resp;
    resp.mtype = pid_del_jugador;                   // PID del cliente (jugador) al que va dirigido el mensaje
    resp.codigo = RESP_GAME_OVER;                   // Eestado: GAME_OVER
    strcpy(resp.datos, "Te atraparon! Game Over."); // mensaje q se enviaria al cliente

    // se envia el mensaje a la cola de respuestas
    msgsnd(mailbox_respuestas_id, &resp, sizeof(resp) - sizeof(long), 0);
}