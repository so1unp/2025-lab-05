// ESTE PROGRAMA ES EJEMPLO DE SERVIDOR
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
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "catacumbas.h"
#include "directorio.h"

// son de prueba
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"

#define PERMISOS 0664

// =========================
//  FUNCIONES AUXILIARES
// =========================
void usage(char *argv[]);
void fatal(char msg[]);
void mostrarMapa(char mapa[FILAS][COLUMNAS]);
void handler(int signal);
void verificarArgumentos(int argc, char* argv[]);

// =========================
// FUNCIONES DE MAPA Y TESOROS
// =========================
void designArena(char mapa[FILAS][COLUMNAS]);
void generarTesoros(struct Tesoro tesoros[], char mapa[FILAS][COLUMNAS]);

// TODO: mejorar logica de posicionamiento del jugador,
// considerar otros jugadores y el tipo de jugador
long ingresarJugador(struct Jugador jugador, char mapa[FILAS][COLUMNAS]);

// TODO: comparar y actualizar posicion de jugador en Jugadores y Mapa
void movimientoJugador(struct Jugador jugador, char mapa[FILAS][COLUMNAS]);

// =========================
//  FUNCIONES DE MENSAJERIA
// =========================
void solicitudJugador(int mailbox_solicitudes_id, struct SolicitudServidor *solicitud);
void responderJugador(int mailbox_respuestas_id, struct RespuestaServidor *respuesta);

// TODO: un hilo que se encargue solo de la atencion de directorio
void *hiloDirectorio(void *arg);// se crea un hilo dedicado exclusivamente a la atencion de solicitudes del directorio.
void *hiloMapa(void *arg);
void solicitudDirectorio(int mailbox_solicitudes_id, struct solicitud *solicitud);
void responderDirectorio(int mailbox_repuestas_id, struct respuesta *respuesta);

// =========================
// FUNCIONES DE CONFIGURACION
// =========================
void conectarMailbox();
void nombresMemoria(char *shm_mapa_nombre,size_t size_mmapa, char *shm_estado_nombre, size_t size_mestado, int catacumba_id);
void abrirMemoria(char *shm_mapa_nombre, char *shm_estado_nombre, int *shm_mapa_fd, int *shm_estado_fd,int crear);
void cerrarMemoria(int *shm_mapa_fd, int *shm_estado_fd);
void borrarMemoria(char *shm_mapa_nombre, char *shm_estado_nombre);

// =========================
//     VARIABLES GLOBALES
// =========================
struct Tesoro tesoros[MAX_TESOROS];
struct Jugador jugadores[MAX_JUGADORES];
char (*mapa)[COLUMNAS];
struct Estado *estado;
struct Jugador* jugador;

pthread_t hilo_directorio;

pthread_t hilo_mapa;

struct SolicitudServidor solicitud;
struct RespuestaServidor respuesta;

struct solicitud d_solicitud;
struct respuesta d_respuesta;

int mailbox_solicitudes_id;
int solicitud_directorio_id, respuesta_directorio_id;
int size_mapa, size_estado;

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

    // int size_mapa, size_estado;

    signal(SIGINT,handler);
    conectarMailbox();

    int catacumba_id = atoi(argv[argc-1]);
    if (catacumba_id < 0 || catacumba_id >= TOTAL_CATACUMBAS)
        fatal("Número de catacumba inválido");
    
    size_mapa = sizeof(char) * FILAS * COLUMNAS;
    size_estado = sizeof(struct Estado);
    
    // =============================
    //    INICIALIZAR SHM_NAME
    // =============================
    char shm_mapa_nombre[128];
    char shm_estado_nombre[128];
    nombresMemoria(shm_mapa_nombre, sizeof(shm_mapa_nombre),
        shm_estado_nombre, sizeof(shm_estado_nombre),
        catacumba_id);

    int shm_mapa_fd, shm_estado_fd;

    char option = argv[1][1];
    switch (option) {
    case 'c':
        verificarArgumentos(argc, argv);
        srand(catacumba_id*1000);
        printf ("Va a crear la catacumba '%s'\n",
             catacumbas[catacumba_id]);

        abrirMemoria(shm_mapa_nombre, shm_estado_nombre,
             &shm_mapa_fd, &shm_estado_fd, 1); // 1 porque crea

        designArena(mapa);
        generarTesoros(tesoros, mapa);
        memset(estado, 0, sizeof(struct Estado));
        estado->max_jugadores = MAX_JUGADORES;

        printf("Servidor listo para catacumba '%s'."
            " Presiona Enter para salir...\n", catacumbas[catacumba_id]);
        getchar();

        cerrarMemoria(&shm_mapa_fd,&shm_estado_fd);
        break;
    case 'b':
        verificarArgumentos(argc, argv);

        printf("Va a borrar la catacumba '%s'\n",
             catacumbas[catacumba_id]);
        borrarMemoria(shm_mapa_nombre,shm_estado_nombre);
        printf("Se ha eliminado la catacumba '%s'."
            " Presiona Enter para salir...\n", catacumbas[catacumba_id]);
        getchar();

        break;
    case 'a': // atender a los clientes
        verificarArgumentos(argc, argv);
        int clave_mailbox_respuestas;

        pthread_create(&hilo_directorio,NULL,hiloDirectorio, NULL); // Un hilo para atender al directorio
        pthread_create(&hilo_mapa,NULL,hiloMapa, NULL); // Un hilo para atender al directorio
        
        abrirMemoria(shm_mapa_nombre, shm_estado_nombre,
             &shm_mapa_fd, &shm_estado_fd, 0); // solo abre

        printf("Servidor de catacumba '%s' esperando solicitudes \n",
             catacumbas[catacumba_id]);
        while (1) { 
            solicitudJugador(mailbox_solicitudes_id,
                 &solicitud);

            clave_mailbox_respuestas = solicitud.clave_mailbox_respuestas;
            respuesta.mtype = solicitud.jugador.pid;

            if (solicitud.codigo == CONECTAR &&
                 !ingresarJugador(solicitud.jugador, mapa)) {
                snprintf(respuesta.mensaje, MAX_LONGITUD_MENSAJES,
                     "Intento fallido, no se conecto jugador");
                respuesta.codigo = -1;
            }  else {
                snprintf(respuesta.mensaje, MAX_LONGITUD_MENSAJES,
                     "Jugador conectado con éxito");
                respuesta.codigo = 1;
            }
            responderJugador(clave_mailbox_respuestas, &respuesta);
        }
        break;
    case 'm':
        verificarArgumentos(argc, argv);
        printf("Va a mostrar la catacumba '%s'\n",
             catacumbas[catacumba_id]);

        abrirMemoria(shm_mapa_nombre, shm_estado_nombre,
             &shm_mapa_fd, &shm_estado_fd, 0);

        mostrarMapa(mapa);

        printf("Se visualizo la catacumba '%s'."
            " Presiona Enter para salir...\n", catacumbas[catacumba_id]);
        getchar();

        cerrarMemoria(&shm_mapa_fd,&shm_estado_fd);
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
            } else if (mapa[i][j] >= TESORO_OFFSET 
                && mapa[i][j] <= MAX_TESOROS) {
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

void handler(int signal) {
    printf("Finalizando servidor...\n");
    exit(EXIT_SUCCESS);
}

void verificarArgumentos(int argc, char* argv[]){
    if (argc < 3) {
        usage(argv);
        exit(EXIT_FAILURE);
    }
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

                printf("Jugador registrado: "
                    "%s (PID: %ld, tipo: %c) en (%d,%d)\n",
                    jugador.nombre, jugador.pid, jugador.tipo, i, j);
                return jugador.pid;
            }
        }
    }
    printf("No hay lugar en el mapa para el jugador.\n");
    return -1;
}




void solicitudJugador(int mailbox_solicitudes_id,
     struct SolicitudServidor *solicitud) {

    printf("Esperando nuevas solicitudes de conexion...\n");
    if (msgrcv(mailbox_solicitudes_id, solicitud,
        sizeof(struct SolicitudServidor) - sizeof(long), 0, 0) == -1) {
        perror("Error al recibir solicitud de jugador");
        return;
    }
}

// responder al cliente
void responderJugador(int mailbox_respuestas_id,
     struct RespuestaServidor *respuesta) {
 
    if (msgsnd(mailbox_respuestas_id, respuesta,
         sizeof(struct RespuestaServidor) - sizeof(long), 0) == -1) {
        perror("Error al enviar respuesta al jugador");
    } else {
        printf("Respuesta enviada al cliente PID %ld\n",
            respuesta->mtype);
    }
}

// conectar o crear mailbox
void conectarMailbox() {
    mailbox_solicitudes_id = msgget(   
        MAILBOX_SOLICITUD_KEY, 0666 | IPC_CREAT);
    if (mailbox_solicitudes_id == -1) {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }

    // solicitud_directorio_id = msgget(  
    //     MAILBOX_KEY, 666);
    // if (mailbox_solicitudes_id == -1) {
    //     perror("Error al crear el mailbox de solicitudes");
    //     exit(EXIT_FAILURE);
    // }
    // respuesta_directorio_id = msgget(   
    //         MAILBOX_RESPUESTA_KEY, 0666 | IPC_CREAT);
    // if (mailbox_solicitudes_id == -1) {
    //     perror("Error al crear el mailbox de solicitudes");
    //     exit(EXIT_FAILURE);
    // }
}

// nombramiento de los espacios de memoria:
// concatena un prefijo con el nombre de la catacumba
void nombresMemoria(char *shm_mapa_nombre, size_t size_mmapa, char *shm_estado_nombre,
     size_t size_mestado, int catacumba_id) {
    snprintf(shm_mapa_nombre, size_mmapa, 
        SHM_MAPA_PREFIX "%s", catacumbas[catacumba_id]);
    snprintf(shm_estado_nombre, size_mestado, 
        SHM_ESTADO_PREFIX "%s", catacumbas[catacumba_id]);
}

// abre o crea el espacio de memoria compartida, segun el valor de crear
void abrirMemoria(char *shm_mapa_nombre, char *shm_estado_nombre,
            int *shm_mapa_fd, int *shm_estado_fd,int crear) {

    int flags = O_RDWR; // solo lectura
    if (crear) flags |= O_CREAT | O_EXCL; // crear si no existe
    int size_mapa = sizeof(char) * FILAS * COLUMNAS;
    int size_estado = sizeof(struct Estado);

    //  abre o crea memoria compartida de mapa
    *shm_mapa_fd = shm_open(shm_mapa_nombre, flags, PERMISOS);
    if (*shm_mapa_fd == -1) fatal(crear? 
        "Error creando shm mapa":"No se pudo abrir shm mapa");
    // si esta creando, intenta truncar
    if (crear && ftruncate(*shm_mapa_fd, size_mapa) == -1)
        fatal("Error truncando shm mapa");
    
    mapa = mmap(NULL, size_mapa, PROT_READ|PROT_WRITE,  MAP_SHARED, 
        *shm_mapa_fd, 0);
    if (mapa == MAP_FAILED) fatal("No se pudo mapear shm mapa");
    
    //  abre o crea memoria compartida de estado 
    *shm_estado_fd = shm_open(shm_estado_nombre, flags, PERMISOS);
    if (*shm_estado_fd == -1) fatal(crear? 
        "Error creando shm estado":"No se pudo abrir shm estado");
    // si esta creando, intenta truncar
    if (crear && ftruncate(*shm_estado_fd, size_estado) == -1)
        fatal("Error truncando shm estado");
    
    estado = mmap(NULL, size_estado, PROT_READ|PROT_WRITE, MAP_SHARED, 
        *shm_estado_fd, 0);
    if (estado == MAP_FAILED) fatal("No se pudo mapear shm estado");
}

// cerrar la memoria compartida
void cerrarMemoria(int *shm_mapa_fd, int *shm_estado_fd) {
    munmap(mapa, size_mapa);
    munmap(estado, size_estado);
    close(*shm_mapa_fd);
    close(*shm_estado_fd);
}

// borrar los espacios de memoria compartida
void borrarMemoria(char *shm_mapa_nombre, char *shm_estado_nombre) {
    if (shm_unlink(shm_mapa_nombre) < 0) 
        fatal("Error al borrar memoria mapa");
    if (shm_unlink(shm_estado_nombre) < 0) 
        fatal("Error al borrar memoria estado");
}

// hilo encargado de atender el directorio
void *hiloDirectorio(void *arg) { 
    printf("espera por mensajes de directorio \n");

    // solicitudDirectorio(solicitud_directorio_id,&d_solicitud);
    // //
    // d_respuesta.mtype = getpid();
    // respuesta.codigo = 1;
    // snprintf(respuesta.mensaje, MAX_LONGITUD_MENSAJES,
    //     "...");
    // responderDirectorio(respuesta_directorio_id, &d_respuesta);
    printf("hasta luego...\n");
    pthread_exit(NULL);
}

// hilo encargado de mostrar el mapa
void *hiloMapa(void *arg) { 
    while (1) {
        mostrarMapa(mapa);
        sleep(5);
    }
}

void solicitudDirectorio(int mailbox_solicitudes_id,
     struct solicitud *solicitud) {

   printf("Esperando nuevas solicitudes de conexion...\n");
   if (msgrcv(mailbox_solicitudes_id, solicitud,
       sizeof(struct solicitud) - sizeof(long), 0, 0) == -1) {
       perror("Error al recibir solicitud del directorio");
       return;
   }
}

void responderDirectorio(int mailbox_repuestas_id,
     struct respuesta *respuesta) {
        if (msgsnd(mailbox_repuestas_id, respuesta,
            sizeof(struct respuesta) - sizeof(long), 0) == -1) {
           perror("Error al enviar respuesta al directorio");
       } else {
           printf("Respuesta enviada al cliente PID %ld\n",
               respuesta->mtype);
       }
}
