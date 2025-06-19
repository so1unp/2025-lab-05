#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ipc.h>
#define _GNU_SOURCE

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

struct Posicion {
    int fila;
    int columna;
};

struct Tesoro {
    int local_id;
    struct Posicion posicion;
};

struct Jugador {
    int pid;
    char nombre[MAX_LONGITUD_NOMBRE_JUGADOR];
    char tipo;
    struct Posicion posicion;
};

struct Estado {
    int max_jugadores;
    int cant_jugadores;
    int cant_raiders;
    int cant_guardianes;
    int cant_tesoros;
};

struct Tesoro tesoros[MAX_TESOROS];
struct Estado *estado;
char (*mapa)[COLUMNAS];
int mailbox_solicitudes_id;
struct Jugador jugadores[MAX_JUGADORES];

// Para comunicación con clientes
// Puede cambiar segun lo que Cliente haga o tenga
struct SolicitudServidor {
    long mtype;
    int codigo; // para identificar eventos que ocurrieron del lado del cliente
    int clave_mailbox_respuestas; //mailbox del cliente
    struct Jugador jugador;
};

struct RespuestaServidor {
    int codigo;
    char mensaje[MAX_LONGITUD_MENSAJES];
};

// Para comunicación con el directorio (Robado de directorio.h)
#define MAX_TEXT 100
struct solicitud
{
    long mtype;           /**< PID del cliente (requerido por las funciones msgrcv/msgsnd) */
    int tipo;             /**< Código de operación (OP_LISTAR, OP_AGREGAR, etc.) */
    char texto[MAX_TEXT]; /**< Datos adicionales según la operación (nombre, dirección, etc.) */
};







// COMUNICACIÓN

/**
 * @brief Crear todos los espacios compartidos para IPC
 *  
 */
void setup(){
    mailbox_solicitudes_id = msgget(MAILBOX_SOLICITUD_KEY, 0666);
    if (mailbox_solicitudes_id == -1) {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char* argv[])
{
    printf("Catacumba\n");

    setup();
    
    int codigo = 0;
    while(1){
        struct SolicitudServidor solicitud;
        solicitud.mtype = 1;
        solicitud.codigo = codigo;
        printf("Tamaño del mensaje: %ld\n", sizeof(solicitud) - sizeof(long));
        if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }
        printf("Mensaje enviado: %d\n", solicitud.codigo);

        codigo = codigo + 1;
        sleep(2);
    }

    exit(EXIT_SUCCESS);
}

