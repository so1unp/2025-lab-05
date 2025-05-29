#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>


#define MAX_JUGADORES 10;
#define MAX_TESOROS 10;
#define HEIGHT 25;
#define WIDTH 80;

#define SHM_NAME "/mapa_memoria"

struct Tesoro {
    int local_id;
    int fila;
    int columna;
};

struct Jugador {
    int local_id;
    int fila;
    int columna;
    char tipo;
};


char map[25][80 + 1] = {
    "###################",
    "#     #           #",
    "#  ### ######## ###",
    "#      #          #",
    "# ###### ######## #",
    "#        #        #",
    "# ######## ###### #",
    "#                 #",
    "# ############### #",
    "###################"};

int px = 1, py = 1;


int main(int argc, char* argv[])
{
    printf("Catacumba\n");

    /*
    for (int i = 0; i < 25; i++) {
        printf("%s\n", map[i]);
    }
    */

    // crear el espacio y obtener el file descriptor de ese "archivo"
    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0664);

    // la memoria se asoció a un archivo con 0 bytes de espacio
    // asignar cuantos bytes de espacio va a tener
    ftruncate(fd, 25*80);

    // mapear esa zona de memoria a una variable
    char *mapa = mmap(NULL, 25*80, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapa == MAP_FAILED){
        perror("Error al mapear memoria");
        exit(EXIT_FAILURE);
    }



    // Pa eliminar

    // usar memset para llenar string con algo
    // memset(buf, ' ', WIDTH*HEIGHT)
    // despues usar las posiciones buf[argv[4] calcular con argv[5]]

    // Escribir
    strcpy(mapa, 0);

    //printf("Escribe %s en el canvas %s en la posición (%d, %d).\n", argv[3], argv[2], atoi(argv[4]), atoi(argv[5]));

    // Remover el mapping y fd
    munmap(mapa, 25*80);
    close(fd);

    exit(EXIT_SUCCESS);
}


/*

TAREAS
- Generar tesoros en posiciones aleatorias (dentro del rango 25 * 80)
- CRUD memoria compartida

CONSIGNA
Deben gestionar los tesoros (generación, ubicación, etc).
Deben mantener y difundir el estado de la catacumba (ubicación de los jugadores, tesoros, etc).
Debe coordinar la acción (procesar acciones de los jugadores, forzar las reglas, etc)
    - Las colisiones con los bordes la hace el equipo Clientes

Memoria compartida:
- Se puede utilizar también para presentar, en modo lectura, el estado de las catacumbas.
    Los clientes (raiders o guardianes) leen el estado y lo presentan por pantalla al jugador.

Mensajes:
- Entre cliente-catacumba: enviar acciones del jugador en un mensaje
    (por ejemplo los movimientos, acciones, mensajes a otros jugadores, etc).

Semaforos:
- Acceso a los recursos compartidos

Sistema de archivos:
- diseño del mapa de la catacumba, configuración, etc.

*/

