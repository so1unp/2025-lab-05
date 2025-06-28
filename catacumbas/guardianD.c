// GUARDIAN DUMMY
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

void solicitudRespuesta(long mi_pid);
void imprimirSolicitud(struct SolicitudServidor *solicitud);
void imprimirRespuesta(struct RespuestaServidor *respuesta);
void finalizar();
void capturaManual(long mi_pid);

int mailbox_respuesta_id, mailbox_solicitudes_id;
struct SolicitudServidor solicitud;
struct RespuestaServidor respuesta;

void fatal(char msg[]) {
    perror(msg);
    finalizar();
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <mailbox_server> <fila> <columna> <captura_bandera>\n", argv[0]);
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
    solicitud.tipo = GUARDIAN;

    solicitudRespuesta(mi_pid); // solicita conectar
    if (respuesta.codigo < 0) fatal("No fue posible conectarse. Finalizando...\n");   
    else printf("se conecto con exito\n");

    solicitud.codigo = MOVIMIENTO;
    solicitud.fila = atoi(argv[2]);
    solicitud.columna = atoi(argv[3]);
    solicitudRespuesta(mi_pid);
    if (respuesta.codigo < 0) fatal("No fue posible moverse. Finalizando...\n");
    else printf("Se movió con éxito a (%d, %d).\n", solicitud.fila, solicitud.columna);
    
    int capturarFlag = 0;
    if (argc > 4) capturarFlag = atoi(argv[4]);

    if (capturarFlag) capturaManual(mi_pid);

    printf("Esperando fin del juego...\n");
    if (msgrcv(mailbox_respuesta_id, &respuesta, sizeof(respuesta) - sizeof(long), mi_pid, 0) == -1)
        fatal("Error al recibir respuesta del servidor");
    imprimirRespuesta(&respuesta);

    finalizar();
}


void capturaManual(long mi_pid) {
    char buffer[100];
    int fila, columna;

    printf("\nIngreso manual de capturas:\n");
    printf("Escribí coordenadas 'fila columna' para intentar captura.\n");
    printf("Escribí 'fin' para terminar y esperar el fin del juego.\n");

    while (1) {
        printf("Ingresá coordenadas (fila columna) o 'fin': ");
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("Error leyendo entrada.\n");
            continue;
        }

        if (strncmp(buffer, "fin", 3) == 0) {
            printf("Saliste del modo captura manual.\n");
            break;
        }

        if (sscanf(buffer, "%d %d", &fila, &columna) != 2) {
            printf("Entrada inválida. Formato correcto: fila columna\n");
            continue;
        }

        solicitud.codigo = RAIDER_CAPTURADO;
        solicitud.fila = fila;
        solicitud.columna = columna;

        solicitudRespuesta(mi_pid);

        if (respuesta.codigo < 0) {
            printf("Captura en (%d, %d) fallida.\n", fila, columna);
        } else {
            printf("Captura en (%d, %d) exitosa o procesada.\n", fila, columna);
        }
    }
}

void solicitudRespuesta(long mi_pid){
    imprimirSolicitud(&solicitud);
    if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1)
    fatal("Error al enviar solicitud de conexión");
    
    if (msgrcv(mailbox_respuesta_id, &respuesta, sizeof(respuesta) - sizeof(long), mi_pid, 0) == -1)
    fatal("Error al recibir respuesta del servidor"); 
    imprimirRespuesta(&respuesta);
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
    printf("Programa terminado\n\n");
    exit(EXIT_SUCCESS);
}