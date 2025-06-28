#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 20 
#define HEIGHT 10

char map[HEIGHT][WIDTH + 1] = {
    "####################",
    "#     #          $ #",
    "#  ### ######## ####",
    "#      #           #",
    "# ###### ######## ##",
    "#        #         #",
    "# ######## ###### ##",
    "#      G           #",
    "# ###############  #",
    "####################"};

int px = 1, py = 1;

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
            if (map[y][x] == '#') {
                //Paredes
                attron(COLOR_PAIR(1));
                mvaddch(y + 4, x + 2, map[y][x]); // +4 para el header, +2 para margen
                attroff(COLOR_PAIR(1));
            } else if (map[y][x] == ' ') {
                //Espacio caminable
                attron(COLOR_PAIR(3));
                mvaddch(y + 4, x + 2, map[y][x]);
                attroff(COLOR_PAIR(3));
            } else if (map[y][x] == '$') {
                //Tesoro
                attron(COLOR_PAIR(2));
                mvaddch(y + 4, x + 2, map[y][x]);
                attroff(COLOR_PAIR(2));
            } else {
                mvaddch(y + 4, x + 2, map[y][x]);
            }
        }
    }
    
    // Instrucciones en la parte inferior
    attron(COLOR_PAIR(5));
    mvprintw(HEIGHT + 6, 2, "Controles: flechas = Mover, 'q' = Salir");
    attroff(COLOR_PAIR(5));
    
    refresh();
}

int is_walkable(int y, int x)
{
    char cell = map[y][x];
    return cell == ' ' || cell == '$';
}

int mostrar_base(char playerChar) {
    initscr();
    //Habilito colores
    start_color(); 
    //Defino los pares de colores
    //Paredes
    init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
    //Tesoro
    init_pair(2, COLOR_RED, COLOR_YELLOW);
    //Espacio caminable
    init_pair(3, COLOR_BLACK, COLOR_GREEN);
    //Header fijo
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
    //Instrucciones
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int ch;
    draw_map();

    while ((ch = getch()) != 'q')
    {
        // Borrar posición anterior del jugador (ajustada por el desplazamiento)
        mvaddch(py + 4, px + 2, ' ');

        int new_px = px;
        int new_py = py;

        switch (ch)
        {
        case KEY_UP:
            new_py--;
            break;
        case KEY_DOWN: 
            new_py++;
            break;
        case KEY_LEFT:
            new_px--;
            break;
        case KEY_RIGHT:
            new_px++;
            break;
        }

        if (is_walkable(new_py, new_px))
        {
            px = new_px;
            py = new_py;
        }
        
        draw_map();
        
        // Dibujar jugador en nueva posición (ajustada por el desplazamiento)
        attron(COLOR_PAIR(2));
        mvaddch(py + 4, px + 2, playerChar);
        attroff(COLOR_PAIR(2));

        if (map[py][px] == '$')
        {
            attron(COLOR_PAIR(2));
            mvprintw(HEIGHT + 8, 2, "¡Conseguiste el tesoro! ¡Felicitaciones! :)");
            attroff(COLOR_PAIR(2));
        }
        refresh();
    }
    endwin();
    return 0;
}