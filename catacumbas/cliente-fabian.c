#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ipc.h>
#define _GNU_SOURCE
#include "catacumbas.h"


// Nos tenemos que poner de acuerdo con las claves de mailbox y nombres de memoria
// con el resto de equipos
#define NOMBRE_BASE "/cat0"
#define NOMBRE_MEMORIA_MAPA NOMBRE_BASE + "-mapa"
#define NOMBRE_MEMORIA_ESTADO NOMBRE_BASE + "-estado"
#define MAILBOX_SOLICITUD_KEY 12345
#define MAILBOX_MOVIMIENTO_KEY 12346

// Tambien con las longitudes de mensajes y nombres
#define MAX_LONGITUD_NOMBRE_RUTAS 30
#define MAX_LONGITUD_NOMBRE_JUGADOR 10
#define MAX_LONGITUD_MENSAJES 50

// Esto esta medianamente fijo (lo manejamos nosotros)
#define MAX_RAIDERS 8
#define MAX_GUARDIANES 8
#define MAX_JUGADORES MAX_GUARDIANES + MAX_RAIDERS
#define MAX_TESOROS 10
#define FILAS 25
#define COLUMNAS 80

struct Tesoro tesoros[MAX_TESOROS];
struct Estado *estado;
char (*mapa)[COLUMNAS];
int mailbox_solicitudes_id;
struct Jugador jugadores[MAX_JUGADORES];








// COMUNICACIÓN

/**
 * @brief Crear todos los espacios compartidos para IPC
 *  
 */
void setup(int key){
    mailbox_solicitudes_id = msgget(key, 0666);
    if (mailbox_solicitudes_id == -1) {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char* argv[])
{
    printf("Catacumba\n");

    setup(atoi(argv[1]));
    
    while(1){
        struct SolicitudServidor {
            long mtype; // el PID del cliente
            int codigo; // codigos especiales para tipos de mensajes
            int clave_mailbox_respuestas; // Mailbox del cliente
            int fila; // Posición
            int columna; 
            char tipo; // Raider o guardian
        };
        struct SolicitudServidor solicitud;
        solicitud.mtype = 1;
        solicitud.codigo = CONEXION;
        solicitud.tipo = GUARDIAN;
        solicitud.fila = 0;
        solicitud.columna = 0;
        if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
        printf("Mensaje enviado: %d\n", solicitud.codigo);

        sleep(2);
    }

    exit(EXIT_SUCCESS);
}



/*
Abrir y cerrar mailbox para enviar mensajes
(para recibir despues veremos)




*/