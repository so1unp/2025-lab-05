// Convenciones de valores en el mapa:
// -1 → pared
//  0 → celda libre
//  1..MAX_TESOROS → tesoros
// PID de jugador → cuando un jugador está en la celda
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "catacumbas.h"

// son de prueba
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"


// =========================
//    FUNCIONES AUXILIARES
// =========================
void usage(char *argv[]);
void fatal(char msg[]);
void mostrarMapa(char mapa[FILAS][COLUMNAS]);
void handler(int signal);// TODO: implementar un handler para interrumpir la atencion.

// =========================
//      FUNCIONES DE MAPA Y TESOROS
// =========================
void designArena(char mapa[FILAS][COLUMNAS]);
void generarTesoros(struct Tesoro tesoros[], char mapa[FILAS][COLUMNAS]);
long ingresarJugador(struct Jugador jugador, char mapa[FILAS][COLUMNAS]);

// =========================
//      FUNCIONES DE MENSAJERIA
// =========================
void solicitudJugador(int *recibido, int mailbox_solicitudes_id, struct SolicitudConexion *solicitud);
void responder(int mailbox_respuestas_id, struct RespuestaConexion *respuesta);
void recibirAccion();

// =========================
//      VARIABLES GLOBALES
// =========================
struct Tesoro tesoros[MAX_TESOROS];
struct Jugador jugadores[MAX_JUGADORES];
char (*mapa)[COLUMNAS];
struct Estado *estado;
struct Jugador* jugador;

struct RespuestaConexion respuesta;
struct SolicitudConexion solicitud;

// server:
// - manejo de memoria compartida
// - generación de mapa y tesoros
// - interfaz CLI para crear/borrar catacumba
int main(int argc, char* argv[]) {

    if(argc < 2){
        usage(argv);
        exit(EXIT_FAILURE);
    }

    if (argv[1][0] != '-') {
        usage(argv);
        exit(EXIT_FAILURE);
    }

    
    int mailbox_solicitudes_id, mailbox_movimientos_id;
    int recibido;

    mailbox_solicitudes_id = msgget(MAILBOX_SOLICITUD_KEY, 0666 | IPC_CREAT);
    if (mailbox_solicitudes_id == -1) {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
    mailbox_movimientos_id = msgget(MAILBOX_MOVIMIENTO_KEY, 0666 | IPC_CREAT);
    if (mailbox_movimientos_id == -1) {
        perror("Error al crear el mailbox de respuestas");
        exit(EXIT_FAILURE);
    }

    int catacumba_id = atoi(argv[argc-1]);
    if (catacumba_id < 0 || catacumba_id >= TOTAL_CATACUMBAS) fatal("Número de catacumba inválido");
    
    int size_mapa = sizeof(char) * FILAS * COLUMNAS;
    int size_estado = sizeof(struct Estado);
    
    // =============================
    //    INICIALIZAR SHM_NAME
    // =============================

    char shm_mapa_nombre[128];
    char shm_estado_nombre[128];

    snprintf(shm_mapa_nombre, sizeof(shm_mapa_nombre), 
        SHM_MAPA_PREFIX "%s", catacumbas[catacumba_id]);
    snprintf(shm_estado_nombre, sizeof(shm_estado_nombre), 
        SHM_ESTADO_PREFIX "%s", catacumbas[catacumba_id]);

    int shm_mapa_fd, shm_estado_fd;

    char option = argv[1][1];
    switch (option) {
    case 'c':
        if (argc < 3) {
            usage(argv);
            exit(EXIT_FAILURE);
        }
        srand(catacumba_id);
        printf ("Va a crear la catacumba '%s'\n",catacumbas[catacumba_id]);

        shm_mapa_fd = 
        shm_open(shm_mapa_nombre,
             O_CREAT | O_RDWR | O_EXCL, 0664);
        if (shm_mapa_fd == -1) fatal("Error creando shm mapa");
        if (ftruncate(shm_mapa_fd, size_mapa) == -1) fatal("Error truncando shm mapa");

        mapa = mmap(NULL,
             size_mapa, PROT_READ | PROT_WRITE, MAP_SHARED, shm_mapa_fd, 0);
        if (mapa == MAP_FAILED) fatal("Error mapeando shm mapa");

        shm_estado_fd = 
            shm_open(shm_estado_nombre,
                 O_CREAT | O_RDWR | O_EXCL, 0664);
        if (shm_estado_fd == -1) fatal("Error creando shm estado");
        if (ftruncate(shm_estado_fd, size_estado) == -1) fatal("Error truncando shm estado");

        estado =  mmap(NULL,
            size_estado, PROT_READ | PROT_WRITE, MAP_SHARED, shm_estado_fd, 0);
        if (estado == MAP_FAILED) fatal("Error mapeando shm estado");

        designArena(mapa);
        generarTesoros(tesoros, mapa);
        memset(estado, 0, sizeof(struct Estado));
        estado->max_jugadores = MAX_JUGADORES;

        printf("Servidor listo para catacumba '%s'. Presiona Enter para salir...\n",
             catacumbas[catacumba_id]);
        getchar();

        munmap(mapa, size_mapa);
        munmap(estado, size_estado);
        close(shm_mapa_fd);
        close(shm_estado_fd);

        break;
    case 'b':
        if (argc < 3) {
            usage(argv);
            exit(EXIT_FAILURE);
        }

        printf("Va a borrar la catacumba '%s'\n", catacumbas[catacumba_id]);

        if (shm_unlink(shm_mapa_nombre) < 0) fatal("Error al borrar memoria mapa");
        if (shm_unlink(shm_estado_nombre) < 0) fatal("Error al borrar memoria estado");

        printf("Se ha eliminado la catacumba '%s'. Presiona Enter para salir...\n", catacumbas[catacumba_id]);
        getchar();

        break;
    //
    // PRIMER CASO EL JUGADOR SE CONECTA
    case 'a':
    
        shm_mapa_fd = shm_open(shm_mapa_nombre, O_RDWR, 0);
        if (shm_mapa_fd == -1) fatal("No se pudo abrir shm mapa");
        
        mapa = mmap(NULL, size_mapa, PROT_READ|PROT_WRITE,  MAP_SHARED, shm_mapa_fd, 0);
        if (mapa == MAP_FAILED) fatal("No se pudo mapear shm mapa");
        
        shm_estado_fd = shm_open(shm_estado_nombre, O_RDWR, 0);
        if (shm_estado_fd == -1) fatal("No se pudo abrir shm estado");
        
        estado = mmap(NULL, size_estado, PROT_READ|PROT_WRITE, MAP_SHARED, shm_estado_fd, 0);
        if (estado == MAP_FAILED) fatal("No se pudo mapear shm estado");
    
        printf("Servidor de catacumba '%s' esperando solicitudes \n", catacumbas[catacumba_id]);
        while (1) { 
            // TODO: espera solicitud conexion.
            solicitudJugador(&recibido,mailbox_solicitudes_id, &solicitud);
  
            if (!ingresarJugador(solicitud.jugador, mapa)){
                snprintf(respuesta.mensaje, MAX_LONGITUD_MENSAJES, "Intento fallido, no se conecto jugador");
                respuesta.clave_mailbox_movimientos = 0;
            }  else {
                snprintf(respuesta.mensaje, MAX_LONGITUD_MENSAJES, "Jugador conectado con éxito");                respuesta.clave_mailbox_movimientos = MAILBOX_MOVIMIENTO_KEY;
                strncpy(respuesta.nombre_memoria_mapa, shm_mapa_nombre, 
                    sizeof(respuesta.nombre_memoria_mapa));
                // asegurarse que termine bien el nombre.
                respuesta.nombre_memoria_mapa[sizeof(respuesta.nombre_memoria_mapa) - 1] = '\0';
            }
            respuesta.mtype = solicitud.jugador.pid;
            responder(solicitud.clave_mailbox_respuestas, &respuesta);
        }
        break;
    case 'm':
        if (argc < 3) {
            usage(argv);
            exit(EXIT_FAILURE);
        }
        printf("Va a mostrar la catacumba '%s'\n", catacumbas[catacumba_id]);
        shm_mapa_fd = shm_open(shm_mapa_nombre, O_RDONLY, 0);
        if (shm_mapa_fd == -1) fatal("No se pudo abrir shm mapa");

        mapa = mmap(NULL, size_mapa, PROT_READ, MAP_SHARED, shm_mapa_fd, 0);
        if (mapa == MAP_FAILED) fatal("No se pudo mapear shm mapa");

        mostrarMapa(mapa);

        printf("Se ha visualizado la catacumba '%s'. Presiona Enter para salir...\n",
             catacumbas[catacumba_id]);
        getchar();

        munmap(mapa, size_mapa);
        close(shm_mapa_fd);
        break;
    case 'h':
        usage(argv);
        break;
    default:
        fprintf(stderr, "Comando desconocido: %s\n", argv[1]);
        usage(argv);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void usage(char *argv[]) {
    fprintf(stderr, "Uso: %s [comando] [numero catacumba]\n", argv[0]);
    fprintf(stderr, "Comandos:\n");
    fprintf(stderr, "\t-c catacumba: iniciar catacumba\n");
    fprintf(stderr, "\t-b catacumba: eliminar catacumba\n");
    fprintf(stderr, "\t-m catacumba: mostrar catacumba\n");
    fprintf(stderr, "\t-a catacumba: atiende catacumba servidor \n");
    fprintf(stderr, "\t-h imprime este mensaje \n");
    fprintf(stderr, "Catacumbas disponibles:\n");
    for (int i = 0; i < TOTAL_CATACUMBAS; i++) {
        fprintf(stderr, "\t%d: %s\n", i, catacumbas[i]);
    }
}

void fatal(char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void designArena(char mapa[FILAS][COLUMNAS]) {
    int i, j;
    for (i = 0; i < FILAS; ++i) {
        for (j = 0; j < COLUMNAS; ++j) {
            if (i == 0 || i == FILAS - 1 || j == 0 || j == COLUMNAS - 1) {
                mapa[i][j] = -1;
            } else { 
                mapa[i][j] = (rand() % 10 < 2) ? -1 : 0;
            }
        }
    }
}

void generarTesoros(struct Tesoro tesoros[], char mapa[FILAS][COLUMNAS]) { 
    int fila, columna, i, libre;
    for (i = 0; i < MAX_TESOROS; i++) {
        do {
            libre = 1;
            fila = 1 + rand() % (FILAS-2);
            columna = 1 + rand() % (COLUMNAS-2);
            if (mapa[fila][columna] != 0) libre = 0;
        } while (!libre);
        mapa[fila][columna] = i + TESORO_OFFSET;
        tesoros[i].id = i + TESORO_OFFSET;
        tesoros[i].posicion = (struct Posicion){fila, columna};
    }
}


// saber que se dibujo
void mostrarMapa(char mapa[FILAS][COLUMNAS]) {
    printf("\n=== MAPA DE LA CATACUMBA ===\n");
    int i,j;
    for (i = 0; i < FILAS; i++) {
        for (j = 0; j < COLUMNAS; j++) {
            char c;
            const char *color = ANSI_RESET;

            if (mapa[i][j] == PARED) {
                c = '#';  
                color = ANSI_BLUE;
            } else if (mapa[i][j] == VACIO) {
                c = ' ';  
            } else if (mapa[i][j] >= TESORO_OFFSET && mapa[i][j] <= MAX_TESOROS) {
                c = '$';  
                color = ANSI_YELLOW;
            } else {
                c = 'J';
                color = ANSI_RED;
            }
            // putchar(c);
            printf("%s%c%s", color, c, ANSI_RESET);

        }
        putchar('\n');
    }
    printf("===========================\n");
}


long ingresarJugador(struct Jugador jugador, char mapa[FILAS][COLUMNAS]) {
    if (estado->cant_jugadores >= estado->max_jugadores) {
        printf("No hay más espacio para jugadores.\n");
        return -1;
    }
    int i, j; 
    for (i = 1; i < FILAS - 1; ++i) {
        for (j = 1; j < COLUMNAS - 1; ++j) {
            if (mapa[i][j] == 0) {
                int idx = estado->cant_jugadores;
                jugadores[idx] = jugador;
                jugadores[idx].posicion.fila = i;
                jugadores[idx].posicion.columna = j;
                mapa[i][j] = jugador.tipo;

                estado->cant_jugadores++;

                printf("Jugador conectado: %s (PID: %ld, tipo: %c) en (%d,%d)\n",
                       jugador.nombre, jugador.pid, jugador.tipo, i, j);
                return jugador.pid;
            }
        }
    }
    printf("No hay lugar en el mapa para el jugador.\n");
    return -1;
}


void solicitudJugador(int *recibido, int mailbox_solicitudes_id, struct SolicitudConexion *solicitud) {
    printf("Esperando nuevas solicitudes de conexion...\n");
    *recibido = msgrcv(mailbox_solicitudes_id, solicitud,
         sizeof(struct SolicitudConexion) - sizeof(long), 0, 0);
    if (*recibido == -1) {
        perror("Error al recibir solicitud");
        return;
    }
}

void responder(int mailbox_respuestas_id, struct RespuestaConexion *respuesta){
    // algo
}


// AGREGAR
// semaforos para gestionar el acceso a la memoria compartida. <- ? 
// modificar e incrementar la comunicacion con los componentes de cliente y directorio
// >> mensajes
// >> memoria compartida