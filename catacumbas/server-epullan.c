#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#define MAX_JUGADORES 10
#define MAX_TESOROS 10
#define HEIGHT 25
#define WIDTH 80
// por ahora no usar limitantes para guardianes y raiders
#define MAX_GUARDIANES MAX_JUGADORES / 2 
#define MAX_EXPLORADORES MAX_JUGADORES - MAX_GUARDIANES

int px = 1, py = 1;

struct Tesoro {
    int local_id;
    int fila;
    int columna;
};

struct Jugador {
    int local_id;
    int fila;
    int columna;
    char tipo;
    int mov_fila;
    int mov_columna;
};


struct Arena {
    struct Tesoro tesoros[MAX_TESOROS];
    struct Jugador jugadores[MAX_JUGADORES];
    int mapa[WIDTH][HEIGHT]; // con los IDs
};

struct Mensaje {
    struct Jugador remitente;
    struct Jugador receptor;
    char msg[150];
};

void receiveAction(); 
void sendSomething();
struct Arena designArena(void);
void genTreasure(struct Arena *arena);
void updateState(struct Arena *arena);
void eraseArena(struct Arena *arena); 
int _login(struct Arena *arena, char tipo); 

int main(int argc, char* argv[])
{
    printf("Catacumba\n");
    exit(EXIT_SUCCESS);
}

// Generar los tesoros para una Arena dada
void genTreasure(struct Arena *arena) { 
    int tx, ty, i;
    for (i = 0; i < MAX_TESOROS; i++) {
        tx = 1 + rand() % (WIDTH-1); // x <- de 1 a 79 ?
        ty = 1 + rand() % (HEIGHT-1); // y <- de 1 a 24 ?

        arena->tesoros[i].columna = tx;
        arena->tesoros[i].fila = ty;
        arena->tesoros[i].local_id = i;
    }
    // probar que funcione
    // falta un validador de posiciones
}

// cliente envia una accion: movimiento, mensajes a otros jugadores.
void receiveAction() {
    // abrir mensaje
    // si la accion es valida actualizar el estado
    // case de acciones
}

 // respondemos con mensajes, resultados de las acciones.
void sendSomething() {
    // despues de actualizar la memoria
    // responder al cliente
}

 // la creacion de una catacumba, se deberia retornar 
struct Arena designArena() {
    struct Arena newarena;
    genTreasure(&newarena);
    // faltan cosas...
    // avisar al directorio
    return newarena;
}

// en mem.comp. el estado de una catacumba 
void updateState(struct Arena *arena){
    // modificaciones realizadas dentro de la arena
    // movimientos, tesoros tomados, colision.
}

// eliminar una Arena
void eraseArena(struct Arena *arena) {
    // avisar al directorio
    // borrar mapa
}

// ingresa un jugador a una Arena seleccionada
// debe retorno un valor para saber si fue exitoso
int _login(struct Arena *arena, char tipo) {
    // refactoring:
    // >>> max guardianes
    // >>> max exploradores
    // >>> discutir la posicion inicial de un jugador 

    int i; 
    for (i = 0; i < MAX_JUGADORES; i++) {
        if (arena->jugadores[i].tipo == '\0') { // char null = '\0'
            // dado jugadores[i] = libre registrar jugador
            arena->jugadores[i].columna = i+1; // esto solo es para probar
            arena->jugadores[i].fila = i+1;
            arena->jugadores[i].local_id = i;
            arena->jugadores[i].tipo = tipo;
            arena->jugadores[i].mov_columna = 0; // ?
            arena->jugadores[i].mov_fila = 0; // ?
            
            return i; // se registro un jugador
        } 
    }
    // cq valor no positivo es falso
    return -1; // no se registro un jugador
}