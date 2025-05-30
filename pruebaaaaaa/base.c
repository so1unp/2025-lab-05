#include <ncurses.h>

#define WIDTH 20 
#define HEIGHT 10

char map[HEIGHT][WIDTH + 1] = {
    "###################",
    "#     #          $#",
    "#  ### ######## ###",
    "#      #          #",
    "# ###### ######## #",
    "#        #        #",
    "# ######## ###### #",
    "#      G          #",
    "# ############### #",
    "###################"};

int px = 1, py = 1;

void draw_map()
{
    for (int y = 0; y < HEIGHT; y++)
    {
        mvprintw(y, 0, map[y]);
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
            mvprintw(HEIGHT + 1, 0, "ya ganaste pa, toca la q para salir del juego");
        }
        refresh();
    }
    endwin();
    return 0;
}