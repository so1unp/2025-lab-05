#include <ncurses.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <unistd.h>

volatile int mostrar_mensaje = 1;
volatile int salir_mensaje = 0;

/**
 * @brief Hilo que muestra un mensaje parpadeante en la pantalla de Game Over.
 *
 * Este hilo imprime el mensaje "Presiona 'q' para volver al menú principal..."
 * en la parte inferior de la pantalla, haciéndolo aparecer y desaparecer cada segundo,
 * hasta que la variable global salir_mensaje sea distinta de cero.
 *
 * @param arg Un arreglo de dos enteros: [max_y, max_x] con el tamaño de la pantalla.
 * @return NULL
 */
void *hilo_mensaje_parpadeante(void *arg) {
    int max_y = ((int *)arg)[0];
    int max_x = ((int *)arg)[1];

    const char *mensaje2 = "Presiona 'q' para volver al menú principal...";

    while (!salir_mensaje) {
        // Mostrar mensaje en celeste
        if (has_colors()) attron(COLOR_PAIR(11));
        mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%s", mensaje2);
        if (has_colors()) attroff(COLOR_PAIR(11));
        refresh();
        sleep(1); // 1 segundo visible

        // Borrar mensaje
        mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%*s", (int)strlen(mensaje2), " ");
        refresh();
        sleep(1); // 1 segundo oculto
    }

    return NULL;
}

void mostrar_game_over() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    const char *titulo = "GAME OVER";
    const char *mensaje1 = "¡Has sido capturado por un guardián!";
    const char *mensaje2 = "Presiona 'q' para volver al menú principal...";

    // Título y mensaje 1
    if (has_colors()) attron(COLOR_PAIR(8) | A_BOLD);
    mvprintw(max_y / 2 - 2, (max_x - strlen(titulo)) / 2, "%s", titulo);
    if (has_colors()) attroff(COLOR_PAIR(8) | A_BOLD);

    if (has_colors()) attron(COLOR_PAIR(10));
    mvprintw(max_y / 2, (max_x - strlen(mensaje1)) / 2, "%s", mensaje1);
    if (has_colors()) attroff(COLOR_PAIR(10));

    refresh();

    // Lanzar hilo para mensaje parpadeante
    pthread_t hilo;
    int args[2] = {max_y, max_x};
    pthread_create(&hilo, NULL, hilo_mensaje_parpadeante, args);

    int ch;
    timeout(-1);
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'Q');

    // Terminar hilo y limpiar mensaje
    salir_mensaje = 1;
    pthread_join(hilo, NULL);
    mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%*s", (int)strlen(mensaje2), " ");
    refresh();
}

void mostrar_pantalla_victoria_guardian() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    const char *titulo = "¡VICTORIA!";
    const char *mensaje1 = "¡Has ganado! Los guardianes han derrotado a todos los raiders.";
    const char *mensaje2 = "Presiona 'q' para volver al menú principal...";

    // Título y mensaje 1
    if (has_colors()) attron(COLOR_PAIR(9) | A_BOLD);
    mvprintw(max_y / 2 - 2, (max_x - strlen(titulo)) / 2, "%s", titulo);
    if (has_colors()) attroff(COLOR_PAIR(9) | A_BOLD);

    if (has_colors()) attron(COLOR_PAIR(10));
    mvprintw(max_y / 2, (max_x - strlen(mensaje1)) / 2, "%s", mensaje1);
    if (has_colors()) attroff(COLOR_PAIR(10));

    refresh();

    // Lanzar hilo para mensaje parpadeante
    salir_mensaje = 0;
    pthread_t hilo;
    int args[2] = {max_y, max_x};
    pthread_create(&hilo, NULL, hilo_mensaje_parpadeante, args);

    int ch;
    timeout(-1);
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'Q');

    // Terminar hilo y limpiar mensaje
    salir_mensaje = 1;
    pthread_join(hilo, NULL);
    mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%*s", (int)strlen(mensaje2), " ");
    refresh();
}


void mostrar_pantalla_derrota() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    const char *titulo = "¡DERROTA!";
    const char *mensaje1 = "Los exploradores recogieron todos los tesoros!";
    const char *mensaje2 = "Presiona 'q' para volver al menú principal...";

    // Título y mensaje 1
    if (has_colors()) attron(COLOR_PAIR(8) | A_BOLD);
    mvprintw(max_y / 2 - 2, (max_x - strlen(titulo)) / 2, "%s", titulo);
    if (has_colors()) attroff(COLOR_PAIR(8) | A_BOLD);

    if (has_colors()) attron(COLOR_PAIR(10));
    mvprintw(max_y / 2, (max_x - strlen(mensaje1)) / 2, "%s", mensaje1);
    if (has_colors()) attroff(COLOR_PAIR(10));

    refresh();

    // Lanzar hilo para mensaje parpadeante
    salir_mensaje = 0;
    pthread_t hilo;
    int args[2] = {max_y, max_x};
    pthread_create(&hilo, NULL, hilo_mensaje_parpadeante, args);

    int ch;
    timeout(-1);
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'Q');

    // Terminar hilo y limpiar mensaje
    salir_mensaje = 1;
    pthread_join(hilo, NULL);
    mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%*s", (int)strlen(mensaje2), " ");
    refresh();
}

void mostrar_pantalla_victoria_tesoros() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    const char *titulo = "¡VICTORIA!";
    const char *mensaje1 = " ¡No hay más tesoros!, los exploradores han ganado!";
    const char *mensaje2 = "Presiona 'q' para volver al menú principal...";

    // Título y mensaje 1
    if (has_colors()) attron(COLOR_PAIR(9) | A_BOLD);
    mvprintw(max_y / 2 - 2, (max_x - strlen(titulo)) / 2, "%s", titulo);
    if (has_colors()) attroff(COLOR_PAIR(9) | A_BOLD);

    if (has_colors()) attron(COLOR_PAIR(10));
    mvprintw(max_y / 2, (max_x - strlen(mensaje1)) / 2, "%s", mensaje1);
    if (has_colors()) attroff(COLOR_PAIR(10));

    refresh();

    // Lanzar hilo para mensaje parpadeante
    salir_mensaje = 0;
    pthread_t hilo;
    int args[2] = {max_y, max_x};
    pthread_create(&hilo, NULL, hilo_mensaje_parpadeante, args);

    int ch;
    timeout(-1);
    do {
        ch = getch();
    } while (ch != 'q' && ch != 'Q');

    // Terminar hilo y limpiar mensaje
    salir_mensaje = 1;
    pthread_join(hilo, NULL);
    mvprintw(max_y / 2 + 2, (max_x - strlen(mensaje2)) / 2, "%*s", (int)strlen(mensaje2), " ");
    refresh();
}