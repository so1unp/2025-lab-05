#include "juego_constantes.h"

void jugar_partida(int tipo) {

    char caracter_jugador;
    char caracter_atrapable;
    switch (tipo) {
        case JUGADOR_EXPLORADOR:
            caracter_jugador = JUGADOR_EXPLORADOR;
            caracter_atrapable = CELDA_TESORO;
            break;
        case JUGADOR_GUARDIAN:
            caracter_jugador = JUGADOR_GUARDIAN;
            caracter_atrapable = CELDA_EXPLORADOR;
            break;
        default:
            printf("Tipo de jugador no válido.\n");
            return;
    }
    
    posicion jugador_posicion = {0, 0};

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

        if (es_caminable(map[new_py][new_px]))
        {
            jugador_posicion->posX = new_px;
            jugador_posicion->posY = new_py;
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

// Función para saber si una celda es caminable 
int es_caminable(char celda) {
    return celda == CELDA_VACIA || celda == celda_atrapable;
}

// Función para saber si un caracter es atrapable
int es_atrapable(char celda) {
    return celda == caracter_atrapable;
}