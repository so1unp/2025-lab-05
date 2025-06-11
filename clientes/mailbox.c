#include <ncurses.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include "../directorio/directorio.h"

#define ESTADO 1

int main()
{
    int mailbox_respuestas_id;
    struct solicitud mensaje;
    int status[ESTADO];

    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0666);
    if (mailbox_respuestas_id == -1)
    {
        perror("Error al conectar con el mailbox de respuestas");
        return 1;
    }

    if (msgrcv(&status, mailbox_respuestas_id, &mensaje) == -1) //va a fallar, faltaria implementar el codigo e el servidor
    {
        perror("Error al recibir mensaje");
        return 1;
    }


 //mostrar estado del servidor, ?mostrar notificaciones? porq el estado?
    return 0;
}