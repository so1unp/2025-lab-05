

#include <ncurses.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#define WIDTH 20
#define HEIGHT 10
#define SHM_NAME "/mapa_memoria"

char (*map)[WIDTH + 1]; // puntero a memoria compartida

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
    // Abrir memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0664);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    // Mapear memoria compartida
    map = mmap(NULL, HEIGHT * (WIDTH + 1), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

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
    munmap(map, HEIGHT * (WIDTH + 1));
    close(shm_fd);
    return 0;
}