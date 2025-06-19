#include <ncurses.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include "juego_constantes.h"

#define WIDTH 20
#define HEIGHT 10
#define SHM_NAME "/mapa_memoria"

char (*map)[WIDTH + 1];

int px = 1, py = 1;

void draw_map()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        mvprintw(y, 0, map[y]);
    }
    refresh();
}

int main()
{
    int tipo_jugador = JUGADOR_EXPLORADOR;

    // Abrir memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0664);
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

        if (es_caminable(tipo_jugador, map[new_py][new_px]))
        {
            px = new_px;
            py = new_py;
        }
        draw_map();
        mvaddch(py, px, CELDA_EXPLORADOR);

        if (captura_tesoro(tipo_jugador, map[py][px]))
        {
            mvprintw(HEIGHT + 1, 0, "¡Ganaste! Toca 'q' para salir del juego");
        }
        refresh();
    }
    endwin();
    munmap(map, HEIGHT * (WIDTH + 1));
    close(shm_fd);
    return 0;
}