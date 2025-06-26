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

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"



void consultarMostrar();
void mostrarMapa();
void solicitudRespuesta(long mi_pid);
void fatal(char msg[]);
void imprimirSolicitud(struct SolicitudServidor *solicitud);
void imprimirRespuesta(struct RespuestaServidor *respuesta);
void finalizar();

char (*mapa)[COLUMNAS];
struct SolicitudServidor solicitud;
struct RespuestaServidor respuesta;
int mailbox_respuesta_id, mailbox_solicitudes_id;


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
    

    printf("mensaje a la mailbox: %d\n\n",clave_server);

    solicitudRespuesta(mi_pid); // solicita conectar
    if (respuesta.codigo < 0) fatal("No fue posible conectarse. Finalizando...\n");   
    else printf("se conecto con exito\n");

    char nombre_memoria[128];
    int base_memoria = atoi(argv[1]) / 10;
    snprintf(nombre_memoria, sizeof(nombre_memoria), "/servidor-mapa-%d", base_memoria);

    int fd = shm_open(nombre_memoria, O_RDONLY, 0666);
    if (fd == -1) fatal("Error al abrir la memoria compartida");
    
    mapa = mmap(NULL, FILAS * COLUMNAS, PROT_READ, MAP_SHARED, fd, 0);
    if (mapa == MAP_FAILED) fatal("Error al mapear la memoria compartida");

    consultarMostrar();

    solicitud.codigo = MOVIMIENTO;
    solicitud.fila = 5;
    solicitud.columna = 5;
    solicitudRespuesta(mi_pid); // solicita moverse
    if (respuesta.codigo < 0) fatal("No fue posible moverse. Finalizando...\n"); 
    else printf("se movio con exito\n");
    
    consultarMostrar();

    solicitud.codigo = DESCONEXION;
    solicitudRespuesta(mi_pid); // solicita desconectar
    if (respuesta.codigo < 0) fatal("No fue posible desconectar. Finalizando...\n");   
    else printf("se desconecto con exito\n");
    
    consultarMostrar();
    
    solicitud.codigo = MOVIMIENTO;
    solicitudRespuesta(mi_pid); // solicita moverse debe fallar
    if (respuesta.codigo < 0) fatal("No fue posible moverse. Finalizando...\n");
    else printf("se movio con exito\n");

    finalizar();
}

void solicitudRespuesta(long mi_pid){
    imprimirSolicitud(&solicitud);
    if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1)
    fatal("Error al enviar solicitud de conexión");
    
    if (msgrcv(mailbox_respuesta_id, &respuesta, sizeof(respuesta) - sizeof(long), mi_pid, 0) == -1)
    fatal("Error al recibir respuesta del servidor"); 
    imprimirRespuesta(&respuesta);
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
    printf("Programa terminado\n\n");
    exit(EXIT_SUCCESS);
}

void consultarMostrar(){
    char respuesta;

    printf("¿Querés ver el mapa? (y/n): ");
    scanf(" %c", &respuesta);

    if (respuesta == 'y' || respuesta == 'Y') mostrarMapa();
}

void mostrarMapa() {
    printf("\n=== MAPA DE LA CATACUMBA ===\n");
    int i, j;
    for (i = 0; i < FILAS; i++) {
        for (j = 0; j < COLUMNAS; j++) {
            const char *color = ANSI_RESET;

            if (mapa[i][j] == PARED) {
                color = ANSI_BLUE;
            } else if (mapa[i][j] == TESORO) {
                color = ANSI_YELLOW;
            } else {
                color = ANSI_RESET;
            }
            printf("%s%c%s", color, mapa[i][j], ANSI_RESET);
        }
        putchar('\n');
    }
    printf("===========================\n");
}