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
#include "catacumbas.h"

// son de prueba
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"

// Borra la pantalla.
#define clear() printf("\033[H\033[J")


void mostrar_menu();
void banderas(int c, int m, int d, int n); // para no pisar 


int main(int argc, char *argv[]) {
    


    char buffer[150];
    int opcion;
    // int mailbox_solicitudes_id, mailbox_movimientos_id;

    printf("este es el cliente test %s", argv[0]);
    while (1) {
        clear();
        mostrar_menu();

        printf("\nSeleccione una opción: ");
        if (scanf("%d", &opcion) != 1) {
            // Limpiar el buffer de entrada si hay error
            while (getchar() != '\n');
            printf(ANSI_RED "Entrada inválida. Intente de nuevo.\n" ANSI_RESET);
            sleep(1);
            continue;
        }
        // Limpiar el buffer de entrada
        while (getchar() != '\n');


        switch (opcion) {
        case CONECTAR:
            struct SolicitudConexion solicitud;
            solicitud.jugador.pid = getpid();
            printf("Nombre: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0; // Quitar \n
            strncpy(solicitud.jugador.nombre, buffer, MAX_LONGITUD_NOMBRE_JUGADOR);
            
            printf("Tipo (R = Raider, G = Guardián): ");
            solicitud.jugador.tipo = getchar();
            while (getchar() != '\n'); // Limpiar buffer
            
            // TODO: formulario jugador
            solicitud.jugador.posicion.fila = -1;
            solicitud.jugador.posicion.columna = -1;
        
            // Simular claves de mailbox para respuestas y notificaciones
            solicitud.clave_mailbox_respuestas = 1234;     // Valor de prueba
            solicitud.clave_mailbox_notificaciones = 5678; // Valor de prueba

            // Mostrar contenido estructurado
            printf("\n--- FORMULARIO DE CONEXIÓN ---\n");
            printf("PID: %ld\n", solicitud.jugador.pid);
            printf("Nombre: %s\n", solicitud.jugador.nombre);
            printf("Tipo: %c\n", solicitud.jugador.tipo);
            printf("Posición: (%d, %d)\n",
                   solicitud.jugador.posicion.fila,
                   solicitud.jugador.posicion.columna);
            printf("Mailbox respuesta: %d\n", solicitud.clave_mailbox_respuestas);
            printf("Mailbox notificación: %d\n", solicitud.clave_mailbox_notificaciones);
            printf("--------------------------------\n");
            break;
        case MOVERSE:
            printf("Movimiento de jugador\n");
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
        // Pequeña pausa para poder ver los resultados
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