#include <ncurses.h>
#include <string.h>

typedef struct {
    char name[64];
    char shm_path[128];
    int mailbox;
    int players_connected;
    int max_players;
} Map;

static int current_selection = 0;

void draw_static_header_mapa() {
    attron(COLOR_PAIR(4));
    mvprintw(1, 2, "=== SELECCIÓN DE MAPA ===");
    attroff(COLOR_PAIR(4));
}

void draw_map_selection(Map *maps, int num_maps) {
    clear();

    // Header fijo
    draw_static_header_mapa();

    int start_y = 4; // Dejar espacio para el header
    int width = 60;

    for (int i = 0; i < num_maps; i++) {
        int y = start_y + i * 3;

        // Marco izquierdo
        attron(COLOR_PAIR(1));
        mvprintw(y, 1, "|");
        attroff(COLOR_PAIR(1));

        // Selección resaltada
        if (i == current_selection) {
            attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
            mvprintw(y, 3, " %d. %-20s ", i + 1, maps[i].name);
            attroff(A_REVERSE | A_BOLD | COLOR_PAIR(2));
        } else {
            attron(COLOR_PAIR(3));
            mvprintw(y, 3, " %d. %-20s ", i + 1, maps[i].name);
            attroff(COLOR_PAIR(3));
        }

        // Info de jugadores y SHM
        mvprintw(y, 26, "| Jugadores: %d/%d | SHM: %-20s", maps[i].players_connected, maps[i].max_players, maps[i].shm_path);

        // Marco derecho
        attron(COLOR_PAIR(1));
        mvprintw(y, width, "|");
        attroff(COLOR_PAIR(1));

        // Línea divisoria
        attron(COLOR_PAIR(1));
        mvhline(y + 1, 1, '-', width);
        attroff(COLOR_PAIR(1));
    }

    // Instrucciones
    attron(COLOR_PAIR(5));
    mvprintw(start_y + num_maps * 3 + 1, 3, "Flechas: mover  |  Enter: seleccionar  |  q: salir");
    attroff(COLOR_PAIR(5));

    refresh();
}

int mostrar_seleccion_mapa(Map *maps, int num_maps) {
    // Colores similares a base.c
    start_color();
    init_pair(1, COLOR_MAGENTA, COLOR_BLACK); // Marco y divisores
    init_pair(2, COLOR_RED, COLOR_YELLOW);    // Selección resaltada
    init_pair(3, COLOR_BLACK, COLOR_GREEN);   // No seleccionado
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK); // Header
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);  // Instrucciones

    keypad(stdscr, TRUE);
    curs_set(0);

    int ch;
    current_selection = 0;
    draw_map_selection(maps, num_maps);

    while ((ch = getch()) != '\n' && ch != '\r') {
        switch (ch) {
            case KEY_UP:
                if (current_selection > 0) current_selection--;
                break;
            case KEY_DOWN:
                if (current_selection < num_maps - 1) current_selection++;
                break;
            case 'q':
            case 'Q':
                return -1;
        }
        draw_map_selection(maps, num_maps);
    }

    clear();
    attron(COLOR_PAIR(2));
    mvprintw(2, 5, "Mapa seleccionado: %s", maps[current_selection].name);
    attroff(COLOR_PAIR(2));

    attron(COLOR_PAIR(5));
    mvprintw(4, 2, "Presiona cualquier tecla para continuar...");
    attroff(COLOR_PAIR(5));

    refresh();
    getch();

    return current_selection;
}
