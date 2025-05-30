#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


const int MAX_JUGADORES = 10;
const int MAX_GUARDIANES = MAX_JUGADORES / 2;
const int MAX_EXPLORADORES = MAX_JUGADORES - MAX_GUARDIANES;
const int MAX_TESOROS = 10;
const int HEIGHT = 25;
const int WIDTH = 80;


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

void genTreasure() {
    // x aleatoria
    // y aleatoria 
    // pos[x,y]
    // tesoro <- pos
    // arena <- tesoro
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
}

void eraseMap() {
    // avisar al directorio
    // borrar mapa
}

void _login() {
    // verificacion: max jugadores max guardianes max exploradores
    // se registro un jugador
}