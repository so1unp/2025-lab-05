#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "../catacumbas/catacumbas.h"

#define MENU_ITEMS 3
#define MENU_WIDTH 40
#define MENU_HEIGHT 10

typedef struct
{
    char *texto;
    char *descripcion;
    int valor;
} OpcionMenu;

int mostrar_menu_rol()
{
    OpcionMenu opciones[MENU_ITEMS] = {
        {"Explorador", "Busca tesoros y evita ser capturado", 'E'},
        {"Guardian", "Protege las catacumbas y captura exploradores", 'G'},
        {"Salir", "Volver al menu principal", -1}};

    int seleccion = 0;
    int ch;
    int max_y, max_x;

    // Habilitar colores si está disponible
    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);  // Seleccionado
        init_pair(2, COLOR_WHITE, COLOR_BLACK);  // Normal
        init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Título
        init_pair(4, COLOR_CYAN, COLOR_BLACK);   // Descripción
    }

    getmaxyx(stdscr, max_y, max_x);

    while (1)
    {
        clear();

        // Título
        attron(COLOR_PAIR(3));
        mvprintw(max_y / 2 - 6, (max_x - strlen("=== SELECCIÓN DE ROL ===")) / 2,
                 "=== SELECCIÓN DE ROL ===");
        attroff(COLOR_PAIR(3));

        // Mostrar opciones del menú
        for (int i = 0; i < MENU_ITEMS; i++)
        {
            int y_pos = max_y / 2 - 2 + i * 2;
            int x_pos = max_x / 2 - MENU_WIDTH / 2;

            if (i == seleccion)
            {
                attron(COLOR_PAIR(1));
                mvprintw(y_pos, x_pos, " > %-20s < ", opciones[i].texto);
                attroff(COLOR_PAIR(1));

                // Mostrar descripción del item seleccionado
                if (i < MENU_ITEMS - 1)
                { // No mostrar descripción para "Salir"
                    attron(COLOR_PAIR(4));
                    mvprintw(y_pos + 1, x_pos + 3, "%-35s", opciones[i].descripcion);
                    attroff(COLOR_PAIR(4));
                }
            }
            else
            {
                attron(COLOR_PAIR(2));
                mvprintw(y_pos, x_pos, "   %-20s   ", opciones[i].texto);
                attroff(COLOR_PAIR(2));
            }
        }

        // Instrucciones
        mvprintw(max_y - 3, (max_x - strlen("Usa las flechas para navegar, ENTER para seleccionar")) / 2,
                 "Usa las flechas para navegar, ENTER para seleccionar");

        // Mostrar caracteres del juego
        mvprintw(max_y / 2 + 4, max_x / 2 - 15, "Explorador: %c", RAIDER);
        mvprintw(max_y / 2 + 5, max_x / 2 - 15, "Guardian:   %c", GUARDIAN);

        refresh();

        ch = getch();

        switch (ch)
        {
        case KEY_UP:
            seleccion = (seleccion - 1 + MENU_ITEMS) % MENU_ITEMS;
            break;
        case KEY_DOWN:
            seleccion = (seleccion + 1) % MENU_ITEMS;
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            return opciones[seleccion].valor;
        case 'q':
        case 'Q':
        case 27: // ESC
            return -1;
        }
    }
}

// Función principal para probar el menú
int mostrar_menu_seleccion_rol()
{
    int rol_seleccionado = mostrar_menu_rol();

    switch (rol_seleccionado)
    {
    case RAIDER:
        printf("Has seleccionado: EXPLORADOR\n");
        printf("Tu misión: Buscar tesoros y evitar ser capturado\n");
        break;
    case GUARDIAN:
        printf("Has seleccionado: GUARDIAN\n");
        printf("Tu misión: Proteger las catacumbas y capturar exploradores\n");
        break;
    case -1:
        printf("Saliendo del juego...\n");
        break;
    default:
        printf("Selección no válida\n");
    }

    return 0;
}