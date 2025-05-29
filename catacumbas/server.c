#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


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
    int max_participantes;
    int mapa[1];
    struct Tesoro tesoros[1];
    struct Jugador jugadores[1];
};


int main(int argc, char* argv[])
{
    printf("Catacumba\n");

    exit(EXIT_SUCCESS);
}
