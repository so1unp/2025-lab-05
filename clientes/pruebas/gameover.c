#include <ncurses.h>
#include <unistd.h> // Para usleep()

void draw_game_over() {
    clear();
    
    attron(COLOR_PAIR(1)); // Color rojo para el texto
    mvprintw(5, 10, "GAME OVER");
    
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    mvprintw(12, 15, "Presiona cualquier tecla para continuar...");
    attroff(COLOR_PAIR(2));

    refresh();
}

int main() {
    initscr();
    start_color();
    
    init_pair(1, COLOR_RED, COLOR_BLACK);   // Rojo para "GAME OVER"
    init_pair(2, COLOR_YELLOW, COLOR_BLACK); // Amarillo para instrucciones
    
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    for (int i = 0; i < 5; i++) {  // Animación de parpadeo
        draw_game_over();
        usleep(500000); // Espera 0.5 segundos
        clear();
        refresh();
        usleep(500000);
    }

    draw_game_over();
    getch();

    endwin();
    return 0;
}
