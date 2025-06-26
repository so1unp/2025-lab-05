#include <ncurses.h>

typedef struct {
    char name[64];
    char shm_path[128];   // Dirección de la memoria compartida
    int mailbox;          // Mailbox del mapa (si lo necesitas)
    int players_connected;
    int max_players;
} Map;

static int current_selection = 0;

// Ahora la función recibe el array de mapas y su cantidad
void draw_map_selection(Map *maps, int num_maps) {
    clear();
    
    attron(COLOR_PAIR(3)); // Título en amarillo
    mvprintw(0, 5, "Seleccione un mapa:");
    attroff(COLOR_PAIR(3));

    for (int i = 0; i < num_maps; i++) {
        if (i == current_selection) {
            attron(A_REVERSE | COLOR_PAIR(1)); // Resaltar mapa seleccionado con color azul
        } else {
            attron(COLOR_PAIR(2)); // Color estándar para los mapas
        }
       //mvprintw(2 + (i * 2), 2, "%s - Jugadores: %d/%d", maps[i].name, maps[i].players_connected, maps[i].max_players);

        mvprintw(2 + (i * 2), 2, "%s - Jugadores: %d/%d - SHM: %s", maps[i].name, maps[i].players_connected, maps[i].max_players, maps[i].shm_path);
        
        attroff(A_REVERSE);
        attroff(COLOR_PAIR(1));
        attroff(COLOR_PAIR(2));

        // Línea divisoria
        attron(COLOR_PAIR(3));
        mvprintw(3 + (i * 2), 2, "------------------------------");
        attroff(COLOR_PAIR(3));
    }

    attron(COLOR_PAIR(3)); // Instrucciones en amarillo
    mvprintw(num_maps * 2 + 3, 2, "Usa las flechas arriba-abajo para cambiar. Presiona Enter para seleccionar.");
    attroff(COLOR_PAIR(3));

    refresh();
}

// También recibe el array y su cantidad
int mostrar_seleccion_mapa(Map *maps, int num_maps) {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);  // Color azul para mapa seleccionado
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // Color blanco para mapas no seleccionados
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Color amarillo para el título e instrucciones
    
    int ch;
    current_selection = 0;
    draw_map_selection(maps, num_maps);

    while ((ch = getch()) != '\n') {
        switch (ch) {
            case KEY_UP:
                if (current_selection > 0) current_selection--;
                break;
            case KEY_DOWN:
                if (current_selection < num_maps - 1) current_selection++;
                break;
        }
        draw_map_selection(maps, num_maps);
    }

    clear();
    attron(COLOR_PAIR(1)); // Mensaje final con color azul
    mvprintw(2, 5, "Mapa seleccionado: %s", maps[current_selection].name);
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(3)); // Instrucción de salida en amarillo
    mvprintw(4, 2, "Presiona cualquier tecla para salir...");
    attroff(COLOR_PAIR(3));

    refresh();
    getch();

    return current_selection;
}
