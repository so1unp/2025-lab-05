#ifndef CATACUMBAS_H
#define CATACUMBAS_H

#define MAX_LONGITUD_NOMBRE_RUTAS 30
#define MAX_LONGITUD_NOMBRE_JUGADOR 10
#define MAX_LONGITUD_MENSAJES 50

#define MAX_RAIDERS 8
#define MAX_GUARDIANES 8
#define MAX_JUGADORES MAX_GUARDIANES + MAX_RAIDERS
#define MAX_TESOROS 10
#define FILAS 25
#define COLUMNAS 80

#define SHM_MAPA_PREFIX "/mapa_memoria_"
#define SHM_ESTADO_PREFIX "/estado_memoria_"
#define TOTAL_CATACUMBAS 10

static const char* catacumbas[TOTAL_CATACUMBAS] = {
    "stack_overflow_abyss",
    "segfault_sanctum",
    "kernel_panic_crypts",
    "syscall_shrine",
    "mmu_maze",
    "dev_null_vaults",
    "trapframe_tomb",
    "bus_error_bastion",
    "deadlock_dungeons",
    "segmented_shadows"
};

struct Posicion {
    int fila;
    int columna;
};

struct Jugador {
    int pid;
    struct Posicion posicion;
    char nombre[MAX_LONGITUD_NOMBRE_JUGADOR];
    char tipo;
};

struct Tesoro {
    int id;
    struct Posicion posicion;
};

struct Arena {
    struct Tesoro tesoros[MAX_TESOROS];
    struct Jugador jugadores[MAX_JUGADORES];
    int mapa[FILAS][COLUMNAS]; // con los IDs
};

struct Estado {
    int max_jugadores;
    int cant_jugadores;
    int cant_raiders;
    int cant_guardianes;
    int cant_tesoros;
};

#endif