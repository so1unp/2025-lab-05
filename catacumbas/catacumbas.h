#ifndef CATACUMBAS_H
#define CATACUMBAS_H

#define MAX_JUGADORES 10
#define MAX_TESOROS 10
#define FILAS 25
#define COLUMNAS 80

// por ahora no usar limitantes para guardianes y raiders
#define MAX_GUARDIANES MAX_JUGADORES / 2 
#define MAX_EXPLORADORES MAX_JUGADORES - MAX_GUARDIANES

struct Posicion {
    int fila;
    int columna;
};

struct Jugador {
    int pid;
    struct Posicion pos;
    char symbol; // nombre
    char tipo; // G o E
};

struct Tesoro {
    int id;
    struct Posicion pos;
};

struct Arena {
    struct Tesoro tesoros[MAX_TESOROS];
    struct Jugador jugadores[MAX_JUGADORES];
    int mapa[FILAS][COLUMNAS]; // con los IDs
};

#endif