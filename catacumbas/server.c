#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


const int MAX_JUGADORES = 10;
const int MAX_TESOROS = 10;
const int HEIGHT = 25;
const int WIDTH = 80;

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


int main(int argc, char* argv[])
{
    printf("Catacumba\n");

    exit(EXIT_SUCCESS);
}
