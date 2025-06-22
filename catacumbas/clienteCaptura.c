#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "catacumbas.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define clear() printf("\033[H\033[J")

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"

void consultarMostrar();
void mostrarMapa();

void finalizar();
void jugarComoRaider(long mi_pid, int clave_server, int clave_mailbox_respuestas);
void jugarComoGuardian(long mi_pid, int clave_server, int clave_mailbox_respuestas);

void solicitudRespuesta(long mi_pid, struct SolicitudServidor *solicitud, int clave_server);
void fatal(char msg[]);

int mailbox_respuesta_id, mailbox_solicitudes_id;
char (*mapa)[COLUMNAS];


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s clave-mailbox-servidor\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int clave_server = atoi(argv[1]);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Hijo - nuevo pid
        long mi_pid_hijo = (long)getpid();
        jugarComoGuardian(mi_pid_hijo, clave_server, mi_pid_hijo * MAILBOX_SOLICITUDES_SUFIJO);
        exit(EXIT_SUCCESS);
    } else {
        // Padre - pid original
        long mi_pid_padre = (long)getpid();
        jugarComoRaider(mi_pid_padre, clave_server, mi_pid_padre * MAILBOX_SOLICITUDES_SUFIJO);
        wait(NULL);
    }
    exit(EXIT_SUCCESS);
}


void fatal(char msg[]) {
    perror(msg);
    finalizar();
}
void jugarComoRaider(long mi_pid, int clave_server, int clave_mailbox_respuestas) {
    // Aquí armás la solicitud para RAIDER
    // (igual que en tu cliente)

    // conectar
    struct SolicitudServidor solicitud = {0};
    solicitud.mtype = mi_pid;
    solicitud.codigo = CONEXION;
    solicitud.clave_mailbox_respuestas = clave_mailbox_respuestas;
    solicitud.fila = 0;
    solicitud.columna = 0;
    solicitud.tipo = RAIDER;

    solicitudRespuesta(mi_pid, &solicitud, clave_server); // Función que envía y recibe

    // mover a (1,1)
    solicitud.codigo = MOVIMIENTO;
    solicitud.fila = 1;
    solicitud.columna = 1;

    solicitudRespuesta(mi_pid, &solicitud, clave_server);

    // Aquí el raider puede quedarse o desconectarse después del intento de captura guardian
}

void jugarComoGuardian(long mi_pid, int clave_server, int clave_mailbox_respuestas) {
    sleep(3); // Esperar que el padre se mueva

    struct SolicitudServidor solicitud = {0};
    solicitud.mtype = mi_pid;
    solicitud.codigo = CONEXION;
    solicitud.clave_mailbox_respuestas = clave_mailbox_respuestas;
    solicitud.fila = 0;
    solicitud.columna = 0;
    solicitud.tipo = GUARDIAN;

    solicitudRespuesta(mi_pid, &solicitud, clave_server);

    // El guardian intenta capturar raider moviéndose a (1,1)
    solicitud.codigo = RAIDER_CAPTURADO;
    solicitud.fila = 1;
    solicitud.columna = 1;

    solicitudRespuesta(mi_pid, &solicitud, clave_server);

    // desconectar guardian
    solicitud.codigo = DESCONEXION;
    solicitudRespuesta(mi_pid, &solicitud, clave_server);
    printf("raider llega esta aca\n");
}

// La función solicitudRespuesta debe ser algo que maneje enviar/recibir
// Ejemplo rápido:

void solicitudRespuesta(long mi_pid, struct SolicitudServidor *solicitud, int clave_server) {
    mailbox_solicitudes_id = msgget(clave_server, 0666);
    if (mailbox_solicitudes_id == -1) fatal("Error al conectar mailbox solicitud");

    key_t clave_mailbox_respuestas = mi_pid * MAILBOX_SOLICITUDES_SUFIJO;
    mailbox_respuesta_id = msgget(clave_mailbox_respuestas , 0777 | IPC_CREAT);
    if (mailbox_respuesta_id == -1)
        fatal("Error al crear mailbox de respuesta");

    struct RespuestaServidor respuesta;
    respuesta.mtype = solicitud->mtype;

    if (msgsnd(mailbox_solicitudes_id, solicitud, sizeof(*solicitud) - sizeof(long), 0) == -1)
        fatal("Error al enviar solicitud");
    
    if (msgrcv(mailbox_respuesta_id, &respuesta, sizeof(respuesta) - sizeof(long), mi_pid, 0) == -1)
        fatal("Error al recibir respuesta del servidor");

    printf("Respuesta servidor: código %d - %s\n", respuesta.codigo, respuesta.mensaje);
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