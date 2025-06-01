#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#define MAX_JUGADORES 10
#define MAX_GUARDIANES MAX_JUGADORES / 2
#define MAX_EXPLORADORES MAX_JUGADORES - MAX_GUARDIANES
#define MAX_TESOROS 10
#define HEIGHT 25
#define WIDTH 80

int px = 1, py = 1;


void genTreasure(); // generar tesoro.
void receiveAction(); // cliente envia una accion: movimiento, mensajes a otros jugadores.
void sendSomething(); // respondemos con mensajes, resultados de las acciones.
void designMap(); // la creacion de un mapa catacumba
void updateState(); // en mem.comp. el estado de una catacumba 
void eraseMap(); // eliminar mapa
void _login(); // 

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

int main(int argc, char* argv[])
{
    printf("Catacumba\n");
    
    exit(EXIT_SUCCESS);
}

// void genTreasure(struct Tesoro *tesoros) {
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
}

void receiveAction() {
    // abrir mensaje
    // si la accion es valida actualizar el estado
}

void sendSomething() {
    // despues de actualizar la memoria
    // responder al cliente
}

void designMap() {
    // definir una base
    // avisar al directorio
}

void eraseMap() {
    // avisar al directorio
    // borrar mapa
}

void _login() {
    // verificacion: max jugadores max guardianes max exploradores
    // se registro un jugador
}