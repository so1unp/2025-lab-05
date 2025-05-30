#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 20
#define HEIGHT 10
#define SHM_NAME "/mapa_memoria"

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

int main() {
    // Crear memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    // Ajustar tamaño
    ftruncate(shm_fd, HEIGHT * (WIDTH + 1));
    // Mapear memoria
    char (*shm_map)[WIDTH + 1] = mmap(NULL, HEIGHT * (WIDTH + 1), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    // Copiar el mapa inicial
    memcpy(shm_map, map, sizeof(map));

    printf("Servidor listo. Presiona Enter para salir...\n");
    getchar();

    // Liberar recursos
    munmap(shm_map, HEIGHT * (WIDTH + 1));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    return 0;
}