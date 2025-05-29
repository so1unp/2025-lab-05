#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <string.h>
#include <signal.h>

//salida
#define KEY_ESC 27

#define FILAS 25
#define COLUMNAS 80
#define JUGADOR 'O'
#define PARED '#'
#define TESORO '¶'
#define ENEMIGO 'N'
#define VACIO ' '

//CONSTANTES ARENA
#define MAX_JUGADORES 10
#define MAX_TESOROS 10
#define HEIGHT 25
#define WIDTH 80
#define JUGADOR_TEST 1
//ESTRUCTURAS 
struct Posicion {
    int fila;
    int columna;
};

struct Jugador {
    int local_id;
    struct Posicion pos;
    char symbol;
    char tipo;
};

struct Tesoro {
    int local_id;
    struct Posicion pos;
    char contenido;
};


struct Arena {
    struct Tesoro tesoros[MAX_TESOROS];
    struct Jugador jugadores[MAX_JUGADORES];
    int mapa[FILAS][COLUMNAS]; // con los IDs
};

//ESTRUCTURA GENERAL DE UNA ARENA (CATACUMBA)
struct Arena arena;

// Agregar variable global
volatile sig_atomic_t running = 1;

// Agregar función manejadora de señales
void signal_handler(int signum) {
    running = 0;
}

void init_map() {
    // crea un mapa vacio
    for(int i = 0; i < FILAS; i++) {
        for(int j = 0; j < COLUMNAS; j++) {
            if(i == 0 || i == FILAS - 1 || j == 0 || j == COLUMNAS - 1)
                arena.mapa[i][j] = PARED;
            else
                arena.mapa[i][j] = VACIO;
        }
    }
}

void init_jugador(int i) {
    arena.jugadores[i].pos.fila = FILAS/2;
    arena.jugadores[i].pos.columna = COLUMNAS/2;
    arena.jugadores[i].symbol = JUGADOR;
    arena.mapa
        [arena.jugadores[i].pos.fila]
        [arena.jugadores[i].pos.columna]
        = arena.jugadores[i].symbol;
}

void draw_map() {
    clear();
    for(int i = 0; i < FILAS; i++) {
        for(int j = 0; j < COLUMNAS; j++) {
            mvaddch(i, j, arena.mapa[i][j]);
        }
    }
    refresh();
}

void move_player(int jugador,int dy, int dx) {
    int nueva_fila = arena.jugadores[jugador].pos.fila + dy;
    int nueva_columna = arena.jugadores[jugador].pos.columna + dx;
    
    // Check if movement is valid
    if(arena.mapa[nueva_fila][nueva_columna] == VACIO) {
        arena.mapa
            [arena.jugadores[jugador].pos.fila]
            [arena.jugadores[jugador].pos.columna]
            = VACIO;
        
        arena.jugadores[jugador].pos.fila = nueva_fila;
        arena.jugadores[jugador].pos.columna = nueva_columna;
        arena.mapa
            [arena.jugadores[jugador].pos.fila]
            [arena.jugadores[jugador].pos.columna]
            = arena.jugadores[jugador].symbol;
    }
}

void handle_input_of(int jugador, int ch) {
    switch(ch) {
        case KEY_UP:
            move_player(jugador, -1, 0);
            break;
        case KEY_DOWN:
            move_player(jugador, 1, 0);
            break;
        case KEY_LEFT:
            move_player(jugador, 0, -1);
            break;
        case KEY_RIGHT:
            move_player(jugador, 0, 1);
            break;
    }
}

int main() {
    // Configurar manejador de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize ncurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    // Initialize game
    init_map();
    init_jugador(JUGADOR_TEST);

    // Game loopº   
    while(1) {
        draw_map();
        int ch = getch();
        if(ch == KEY_ESC) break;  // Quit on 'q'
        handle_input_of(JUGADOR_TEST ,ch);
    }

    // Cleanup
    endwin();
    return 0;
}