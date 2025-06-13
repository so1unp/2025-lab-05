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

// son de prueba
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"

// Borra la pantalla.
#define clear() printf("\033[H\033[J")


void mostrar_menu();

struct RespuestaConexion respuesta;
struct SolicitudConexion solicitud;

// la idea es probar
int main(int argc, char *argv[]) {
    char buffer[150];
    int opcion;
    pid_t mi_pid = getpid();
    int mailbox_solicitudes_id, mailbox_movimientos_id;
    
    mailbox_solicitudes_id = msgget(MAILBOX_SOLICITUD_KEY,0666);
    if (mailbox_solicitudes_id == -1){
        perror("Error al conectar mailbox solicitud");
    }

    mailbox_movimientos_id = msgget(MAILBOX_MOVIMIENTO_KEY,0666);
    if (mailbox_movimientos_id == -1){
        perror("Error al conectar mailbox respuesta");
    }
    
    printf("este es el cliente test %s", argv[0]);
    while (1) {
        clear();
        mostrar_menu();

        printf("\nSeleccione una opción: ");
        if (scanf("%d", &opcion) != 1) {
            while (getchar() != '\n'); // limpiar buffer
            printf(ANSI_RED "Entrada inválida. Intente de nuevo.\n" ANSI_RESET);
            sleep(1);
            continue;
        }
        while (getchar() != '\n'); // limpiar buffer

        switch (opcion) {
        case CONECTAR:
            solicitud.jugador.pid = mi_pid;
            printf("Nombre: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0; // quitar el salto de linea
            strncpy(solicitud.jugador.nombre, buffer, MAX_LONGITUD_NOMBRE_JUGADOR);
            solicitud.jugador.nombre[MAX_LONGITUD_NOMBRE_JUGADOR - 1] = '\0';

            printf("Tipo (R = Raider, G = Guardián): ");
            solicitud.jugador.tipo = getchar();
            while (getchar() != '\n'); // limpiar buffer
            
            solicitud.mtype = 1; //para que funcione

            solicitud.jugador.posicion.fila = -1;
            solicitud.jugador.posicion.columna = -1;
            // Simular claves de mailbox para respuestas y notificaciones
            solicitud.clave_mailbox_respuestas = 12678; // valor de prueba
            solicitud.clave_mailbox_notificaciones = 23678; // valor de prueba

            solicitud.clave_mailbox_respuestas = msgget(12678, 0666 | IPC_CREAT);
            if (solicitud.clave_mailbox_respuestas == -1) {
                perror("Error al crear el mailbox de respuesta");
                exit(EXIT_FAILURE);
            }

            // mostrar contenido 
            printf("\n--- FORMULARIO DE CONEXIÓN ---\n");
            printf("PID: %ld\n", solicitud.jugador.pid);
            printf("Nombre: %s\n", solicitud.jugador.nombre);
            printf("Tipo: %c\n", solicitud.jugador.tipo);
            printf("Posición: (%d, %d)\n",
                   solicitud.jugador.posicion.fila,
                   solicitud.jugador.posicion.columna);
            printf("Mailbox respuesta: %d\n", solicitud.clave_mailbox_respuestas);
            printf("Mailbox notificación: %d\n", solicitud.clave_mailbox_notificaciones);
            printf("--------------------------------\n\n");
            if (msgsnd(mailbox_solicitudes_id,
                    &solicitud,
                    sizeof(solicitud) - sizeof(long),
                    0)== -1) {
                perror("Error al enviar solicitu de conexion");
                break;
            }
            printf("Solicitud enviada...\n");
            if (msgrcv(solicitud.clave_mailbox_respuestas,
                    &respuesta,
                    sizeof(respuesta) - sizeof(long),
                    mi_pid, 0) == -1) {
                perror("Error al recibir respuesta");
            }
            else {
                printf("RESPUESTA DEL SERVIDOR\n");
                printf("%s\n",respuesta.mensaje);
                printf("%s\n",respuesta.nombre_memoria_mapa);
            }
           
            break;
        case MOVERSE:
            struct Movimiento movimiento;
            movimiento.pid_cliente = getpid();
                
            // Simulamos posición actual (ejemplo: centro del mapa)
            struct Posicion actual = {5, 5}; // Esto sería real en producción
            movimiento.posicion = actual;
                
            printf("Movimiento de jugador:\n");
            printf("\tW(^)\tS(v)\tA(<)\tD(>)\n");
            printf("Dirección: ");
            char dir = getchar();
            while (getchar() != '\n'); // Limpiar buffer
                
            // Actualizar posición según dirección
            switch (dir) {
                case 'W': case 'w':
                    movimiento.posicion.fila--;
                    break;
                case 'S': case 's':
                    movimiento.posicion.fila++;
                    break;
                case 'A': case 'a':
                    movimiento.posicion.columna--;
                    break;
                case 'D': case 'd':
                    movimiento.posicion.columna++;
                    break;
                default:
                    printf(ANSI_RED "Dirección inválida. Usa W/A/S/D.\n" ANSI_RESET);
                    break;
            }
        
            // Mostrar movimiento simulado
            printf("\n--- MENSAJE DE MOVIMIENTO ---\n");
            printf("PID: %ld\n", movimiento.pid_cliente);
            printf("Posición destino: fila=%d, columna=%d\n",
                   movimiento.posicion.fila,
                   movimiento.posicion.columna);
            printf("----------------------------------\n");
            // TODO: enviar movimiento
            break;
        case DESCONECTAR:
            printf("Desconectando...\n");
            // TODO: enviar señal de desconexión
            exit(EXIT_SUCCESS);
        case NOTIFICACION:
            printf("Mostrando notificación...\n");
            // TODO: leer o recibir notificación
            break;
        default:
            printf(ANSI_YELLOW "Opción no válida. Intente de nuevo.\n" ANSI_RESET);
            sleep(2);
            continue;
        }
       
        printf("Presione Enter para continuar...");
        getchar();
    }
}

void mostrar_menu() {
    printf(ANSI_BLUE "\n========= CLIENTE DE PRUEBA =========\n" ANSI_RESET);
    printf("\t1. Conectar\n");
    printf("\t2. Moverse\n");
    printf("\t3. Desconectar\n");
    printf("\t4. Notificación\n");
}