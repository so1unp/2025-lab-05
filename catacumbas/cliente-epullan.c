#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "catacumbas.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>

#define clear() printf("\033[H\033[J")

void fatal(char msg[]);
void imprimirSolicitud(struct SolicitudServidor *solicitud);
void imprimirRespuesta(struct RespuestaServidor *respuesta);
void finalizar();
void acciones();
void mostrar_menu();

struct SolicitudServidor solicitud;
struct RespuestaServidor respuesta;
int mailbox_respuesta_id, mailbox_solicitudes_id;
char (*mapa)[COLUMNAS];

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s clave-mailbox-servidor\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, finalizar);
    long mi_pid = (long) getpid();
     
    key_t clave_server = atoi(argv[1]);
    mailbox_solicitudes_id = msgget(clave_server, 0666);
    if (mailbox_solicitudes_id == -1) fatal("Error al conectar mailbox solicitud");

    key_t clave_mailbox_respuestas = mi_pid * MAILBOX_SOLICITUDES_SUFIJO;
    mailbox_respuesta_id = msgget(clave_mailbox_respuestas , 0777 | IPC_CREAT);
    if (mailbox_respuesta_id == -1)
        fatal("Error al crear mailbox de respuesta");
        
    // accion primera la conexion del jugador
    solicitud.mtype = mi_pid;
    solicitud.codigo = CONEXION;
    solicitud.clave_mailbox_respuestas = clave_mailbox_respuestas;
    solicitud.fila = 0;
    solicitud.columna = 0;
    solicitud.tipo = RAIDER;

    if (solicitud.clave_mailbox_respuestas == -1)
        fatal("Error al crear el mailbox de respuesta");
    

    printf("mensaje a la mailbox: %d\n",clave_server);
    imprimirSolicitud(&solicitud);

    if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1)
        fatal("Error al enviar solicitud de conexión");
        
    if (msgrcv(mailbox_respuesta_id, &respuesta, sizeof(respuesta) - sizeof(long), mi_pid, 0) == -1)
        fatal("Error al recibir respuesta del servidor"); 
        
    imprimirRespuesta(&respuesta);
    if (!respuesta.codigo) fatal("No fue posible conectarse. Finalizando...\n");   
}

void fatal(char msg[]) {
    perror(msg);
    finalizar();
}

void imprimirSolicitud(struct SolicitudServidor *solicitud) {
    printf("Solicitud del cliente:\n");
    printf("- Mtype: %li\n", solicitud->mtype);
    printf("- Codigo: %i\n", solicitud->codigo);
    printf("- Mailbox: %i\n", solicitud->clave_mailbox_respuestas);
    printf("- Fila: %i\n", solicitud->fila);
    printf("- Columna: %i\n", solicitud->columna);
    printf("- Tipo: %c\n\n", solicitud->tipo);
}
void imprimirRespuesta(struct RespuestaServidor *respuesta) {
    printf("Respuesta del servidor:\n");
    printf("- Mtype: %li\n", respuesta->mtype);
    printf("- Código: %d\n", respuesta->codigo);
    printf("- Mensaje: %s\n\n", respuesta->mensaje);
}

void finalizar(){
    if (msgctl(mailbox_respuesta_id, IPC_RMID, NULL) == -1) {
        printf("%d \n",mailbox_respuesta_id);
        perror("🚫 Error al eliminar el buzón de respuesta, por favor eliminelo manualmente con ipcs -q y luego ipcrm -q <id>");
        exit(EXIT_FAILURE);
    }
    printf("Programa terminado\n");
}

