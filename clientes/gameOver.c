#include <ncurses.h>
#include <string.h>

void mostrar_game_over() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    const char *titulo = "GAME OVER";
    const char *mensaje1 = "¡Has sido capturado por un guardián!";
    const char *mensaje2 = "Presiona 'q' para volver al menú principal...";

    if (has_colors()) {
        attron(COLOR_PAIR(1) | A_BOLD);
    }
    mvprintw(max_y / 2 - 2, (max_x - strlen(titulo)) / 2, "%s", titulo);
    if (has_colors()) {
        attroff(COLOR_PAIR(1) | A_BOLD);
        attron(COLOR_PAIR(5));
    }
    mvprintw(max_y / 2, (max_x - strlen(mensaje1)) / 2, "%s", mensaje1);
    mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%s", mensaje2);
    if (has_colors()) {
        attroff(COLOR_PAIR(5));
    }
    refresh();

    int ch;
    timeout(-1);
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'Q');
}
void mostrar_pantalla_victoria_guardian() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    const char *titulo = "¡VICTORIA!";
    const char *mensaje1 = "¡Has ganado! Los guardianes han derrotado a todos los raiders.";
    const char *mensaje2 = "Presiona 'q' para volver al menú principal...";

    if (has_colors()) {
        attron(COLOR_PAIR(2) | A_BOLD);
    }
    mvprintw(max_y / 2 - 2, (max_x - strlen(titulo)) / 2, "%s", titulo);
    if (has_colors()) {
        attroff(COLOR_PAIR(2) | A_BOLD);
        attron(COLOR_PAIR(5));
    }
    mvprintw(max_y / 2, (max_x - strlen(mensaje1)) / 2, "%s", mensaje1);
    mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%s", mensaje2);
    if (has_colors()) {
        attroff(COLOR_PAIR(5));
    }
    refresh();

    int ch;
    timeout(-1);
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'Q');
}