#include <ncurses.h>

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

void draw_map()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            if (map[y][x] == '#') {
                //Paredes
                attron(COLOR_PAIR(1));
                mvaddch(y, x, map[y][x]);
                attroff(COLOR_PAIR(1));
            } else if (map[y][x] == ' ') {
                //Espacio caminale
                attron(COLOR_PAIR(3));
                mvaddch(y, x, map[y][x]);
                attroff(COLOR_PAIR(2));
            } else if (map[y][x] == '$') {
                //Tesoro
                attron(COLOR_PAIR(2));
                mvaddch(y, x, map[y][x]);
                attroff(COLOR_PAIR(3));
            } else {
                mvaddch(y, x, map[y][x]);
            }
        }
    }
    refresh();
}


int is_walkable(int y, int x)
{
    char cell = map[y][x];
    return cell == ' ' || cell == '$';
}


int main()
{
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
        mvaddch(py, px, 'E');

        if (map[py][px] == '$')
        {
            mvprintw(HEIGHT + 1, 0, "Conseguiste todos los tesoros, felicitaciones! :)");
        }
        refresh();
    }
    endwin();
    return 0;
}