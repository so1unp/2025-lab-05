#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "catacumbas.h"

#define SHM_PREFIX "/mapa_memoria_"


void usage(char *argv[]);
void F(char msg[]);

// mapa donde aleatoria mente elige a los 
void designArena(char mapa[FILAS][COLUMNAS]);

// recibe un puntero en el que trabajar y un mapa en donde almacenar
void generarTesoros(struct Tesoro tesoros[], char mapa[FILAS][COLUMNAS]);

void updateState();

struct Tesoro tesoros[MAX_TESOROS];
struct Jugador jugadores[MAX_JUGADORES];
char (*mapa)[COLUMNAS];
struct Estado *estado;

int mailbox_solicitudes_id;
int mailbox_movimientos_id;


int main(int argc, char* argv[]) {

    if(argc < 2){
        usage(argv);
        exit(EXIT_FAILURE);
    }

    if (argv[1][0] != '-') {
        usage(argv);
        exit(EXIT_FAILURE);
    }

    int catacumba_id = atoi(argv[argc-1]);
    if (catacumba_id < 0 || catacumba_id >= TOTAL_CATACUMBAS) F("Número de catacumba inválido");
    
    int size_mapa = sizeof(char) * FILAS * COLUMNAS;
    int size_estado = sizeof(struct Estado);

    // ===============================
    // INICIALIZAR MEMORIA COMPARTIDA
    // ===============================
    
    char shm_mapa_nombre[128];
    char shm_estado_nombre[128];

    snprintf(shm_mapa_nombre, sizeof(shm_mapa_nombre), 
        SHM_MAPA_PREFIX "%s", catacumbas[catacumba_id]);
    snprintf(shm_estado_nombre, sizeof(shm_estado_nombre), 
        SHM_ESTADO_PREFIX "%s", catacumbas[catacumba_id]);

    int shm_mapa_fd, shm_estado_fd;

    char option = argv[1][1];
    switch (option)
    {
    case 'c':
        if (argc < 3) {
            usage(argv);
            exit(EXIT_FAILURE);
        }

        printf ("Va a crear la memoria para %s\n",catacumbas[atoi(argv[2])]);

        shm_mapa_fd = 
        shm_open(shm_mapa_nombre,
             O_CREAT | O_RDWR | O_EXCL, 0664);
        if (shm_mapa_fd == -1) F("Error creando shm mapa");
        if (ftruncate(shm_mapa_fd, size_mapa) == -1) F("Error truncando shm mapa");

        mapa = mmap(NULL,
             size_mapa, PROT_READ | PROT_WRITE, MAP_SHARED, shm_mapa_fd, 0);
        if (mapa == MAP_FAILED) F("Error mapeando shm mapa");

        shm_estado_fd = 
            shm_open(shm_estado_nombre,
                 O_CREAT | O_RDWR | O_EXCL, 0664);
        if (shm_estado_fd == -1) F("Error creando shm estado");
        if (ftruncate(shm_estado_fd, size_estado) == -1) F("Error truncando shm estado");

        estado =  mmap(NULL,
            size_estado, PROT_READ | PROT_WRITE, MAP_SHARED, shm_estado_fd, 0);
        if (estado == MAP_FAILED) F("Error mapeando shm estado");

        // ===============================
        // INICIALIZAR ESTRUCTURAS
        // ===============================
        designArena(mapa);
        generarTesoros(tesoros, mapa);
        memset(estado, 0, sizeof(struct Estado));
        estado->max_jugadores = MAX_JUGADORES;

        printf("Servidor listo para catacumba '%s'. Presiona Enter para salir...\n", catacumbas[catacumba_id]);
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

        printf("Borra la memoria de %s\n", catacumbas[atoi(argv[2])]);

        if (shm_unlink(shm_mapa_nombre) < 0) F("Error al borrar memoria mapa");
        if (shm_unlink(shm_estado_nombre) < 0) F("Error al borrar memoria estado");

        printf("Se ha eliminado la memoria de %s\n", catacumbas[atoi(argv[2])]);
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

void usage(char *argv[])
{
    fprintf(stderr, "Uso: %s [comando] [numero catacumba]\n", argv[0]);
    fprintf(stderr, "Comandos:\n");
    fprintf(stderr, "\t-c catacumba: iniciar catacumba\n");
    fprintf(stderr, "\t-b catacumba: eliminar catacumba\n");
    fprintf(stderr, "\t-h imprime este mensaje \n");
    fprintf(stderr, "Catacumbas disponibles:\n");
    for (int i = 0; i < TOTAL_CATACUMBAS; i++) {
        fprintf(stderr, "\t%d: %s\n", i, catacumbas[i]);
    }
}

// Convenciones de valores en el mapa:
// -1 → pared
//  0 → celda libre
//  1..MAX_TESOROS → tesoros
// PID de jugador → cuando un jugador está en la celda
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
        mapa[fila][columna] = i + 1;
        tesoros[i].id = i + 1;
        tesoros[i].posicion = (struct Posicion){fila, columna};
    }
}

void F(char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
}


// AGREGAR
// semaforos para gestionar el acceso a la memoria compartida.
// mediante mensajes la gestion acciones: login, movimientos, 
// mediante mensajes comunicacion con directorio