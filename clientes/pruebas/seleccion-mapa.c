#include <ncurses.h>

#define NUM_MAPS 3

typedef struct {
    char name[20];
    int players_connected;
    int max_players;
} Map;

// Lista de mapas disponibles
Map maps[NUM_MAPS] = {
    {"Castillo Oscuro", 3, 5},
    {"Bosque Encantado", 1, 4},
    {"Ciudad Perdida", 4, 6}
};

int selected_map = 0;

void draw_map_selection() {
    clear();
    
    attron(COLOR_PAIR(3)); // Título en amarillo
    mvprintw(0, 5, "Seleccione un mapa:");
    attroff(COLOR_PAIR(3));

    for (int i = 0; i < NUM_MAPS; i++) {
        if (i == selected_map) {
            attron(A_REVERSE | COLOR_PAIR(1)); // Resaltar mapa seleccionado con color azul
        } else {
            attron(COLOR_PAIR(2)); // Color estándar para los mapas
        }

        mvprintw(2 + (i * 2), 2, "%s - Jugadores: %d/%d", maps[i].name, maps[i].players_connected, maps[i].max_players);
        
        attroff(A_REVERSE);
        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));

        // Línea divisoria
        attron(COLOR_PAIR(3)); // Usamos amarillo para las líneas
        mvprintw(3 + (i * 2), 2, "------------------------------");
        attroff(COLOR_PAIR(3));
    }

    attron(COLOR_PAIR(3)); // Instrucciones en amarillo
    mvprintw(NUM_MAPS * 2 + 3, 2, "Usa las flechas arriba-abajo para cambiar. Presiona Enter para seleccionar.");
    attroff(COLOR_PAIR(3));

    refresh();
}

int main() {
    initscr();
    start_color();

    // Definir colores
    init_pair(1, COLOR_BLUE, COLOR_BLACK);  // Color azul para mapa seleccionado
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // Color blanco para mapas no seleccionados
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Color amarillo para el título e instrucciones
    
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int ch;
    draw_map_selection();

    while ((ch = getch()) != '\n') {
        switch (ch) {
            case KEY_UP:
                if (selected_map > 0) selected_map--;
                break;
            case KEY_DOWN:
                if (selected_map < NUM_MAPS - 1) selected_map++;
                break;
        }
        draw_map_selection();
    }

    clear();
    attron(COLOR_PAIR(1)); // Mensaje final con color azul
    mvprintw(2, 5, "Mapa seleccionado: %s", maps[selected_map].name);
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(3)); // Instrucción de salida en amarillo
    mvprintw(4, 2, "Presiona cualquier tecla para salir...");
    attroff(COLOR_PAIR(3));

    refresh();
    getch();

    endwin();
    return 0;
}
