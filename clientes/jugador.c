#include "juego_constantes.h"
#include <ncurses.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>

char caracter_atrapable;
char caracter_jugador;
char (*map)[WIDTH + 1];
extern char selected_role[50]; // Valor por defecto
extern char  selected_map[50]; // Valor por defecto


void draw_static_header() {
    // Dibujar barra superior fija con información del juego
    attron(COLOR_PAIR(4));
    
    // Línea superior del marco
    
    // Información del juego
    mvprintw(1, 0, "|");
    mvprintw(1, 2, "ROL: %s                          ", selected_role);
    mvprintw(1, 25, "|");
    mvprintw(1, 27, "MAPA: %s                        ", selected_map);
    
    // Línea inferior del marco
    
    attroff(COLOR_PAIR(4));
}

void draw_map()
{
    // SIEMPRE dibujar el header fijo primero
    draw_static_header();
    
    // Dibujar el mapa desplazado 4 líneas hacia abajo (después del header)
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            if (map[y][x] == CELDA_PARED) {
                //Paredes
                attron(COLOR_PAIR(1));
                mvaddch(y + 4, x + 2, map[y][x]); // +4 para el header, +2 para margen
                attroff(COLOR_PAIR(1));
            } else if (map[y][x] == CELDA_VACIA) {
                //Espacio caminable
                attron(COLOR_PAIR(3));
                mvaddch(y + 4, x + 2, map[y][x]);
                attroff(COLOR_PAIR(3));
            } else if (map[y][x] == CELDA_TESORO) {
                //Tesoro
                attron(COLOR_PAIR(2));
                mvaddch(y + 4, x + 2, map[y][x]);
                attroff(COLOR_PAIR(2));
            } else {
                mvaddch(y + 4, x + 2, map[y][x]);
            }
        }
    }
}

// Función para saber si una celda es caminable
bool es_caminable(char celda) {
    return celda == CELDA_VACIA || celda == caracter_atrapable;
}

// Función para saber si un caracter es atrapable
bool es_atrapable(char celda) {
    return celda == caracter_atrapable;
}

int jugar_partida(int tipo, char *shm_name) {

    int px = 1;
    int py = 1;
    Posicion jugador_posicion = {0, 0};

    switch (tipo) {
        case JUGADOR_EXPLORADOR:
            caracter_jugador = CELDA_EXPLORADOR;
            caracter_atrapable = CELDA_TESORO;
            break;
        case JUGADOR_GUARDIAN:
            caracter_jugador = CELDA_GUARDIAN;
            caracter_atrapable = CELDA_EXPLORADOR;
            break;
        default:
            printf("Tipo de jugador no válido.\n");
            return 1;
    }
    

    // Abrir memoria compartida
    int shm_fd = shm_open(shm_name, O_RDONLY, 0664);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    map = mmap(NULL, HEIGHT * (WIDTH + 1), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int ch;
    draw_map();

    while ((ch = getch()) != 'q')
    {
        mvaddch(py, px, ' ');

        int new_px = px;
        int new_py = py;

        switch (ch)
        {
        case KEY_UP:    new_py--; break;
        case KEY_DOWN:  new_py++; break;
        case KEY_LEFT:  new_px--; break;
        case KEY_RIGHT: new_px++; break;
        }

        if (es_caminable(map[new_py][new_px]))
        {
            jugador_posicion.posX = new_px;
            jugador_posicion.posY = new_py;
            if (es_atrapable(map[new_px][new_py]))
            {
                // Implementar mensajes al servidor
                mvprintw(HEIGHT + 1, 0, "¡Ganaste! Toca 'q' para salir del juego");
            }else{
                // Implementar mensajes al servidor
            }
        }

        draw_map();
        mvaddch(py, px, caracter_jugador);
        refresh();
    }
    endwin();
    munmap(map, HEIGHT * (WIDTH + 1));
    close(shm_fd);
    return 0;
}