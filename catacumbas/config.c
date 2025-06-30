#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "../directorio/directorio.h"
#include "utils.h"
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <string.h>

#define RANDOM_FILAS()(1 + rand() % (FILAS-2));
#define RANDOM_COLMS()(1 + rand() % (COLUMNAS-2));


void inicializarEstado(struct Arena *arena) {
    arena->estado->max_jugadores = arena->max_guardianes + arena->max_raiders;
    arena->estado->cant_guardianes = 0;
    arena->estado->cant_raiders = 0;
    arena->estado->cant_jugadores = 0;
    arena->estado->cant_tesoros = 0;
}

void inicializarComunicacion(struct Comunicacion *comunicacion) {
    snprintf(comunicacion->memoria_mapa_nombre, sizeof(comunicacion->memoria_mapa_nombre), "%s%d", MEMORIA_MAPA_PREFIJO, getpid());
    snprintf(comunicacion->memoria_estado_nombre, sizeof(comunicacion->memoria_mapa_nombre), "%s%d", MEMORIA_ESTADO_PREFIJO, getpid());
    comunicacion->mailbox_solicitudes_clave = getpid() * MAILBOX_SOLICITUDES_SUFIJO;      
}

void abrirMemoria(struct Arena *arena, struct Comunicacion *comunicacion) {
    comunicacion->memoria_mapa_fd =
        shm_open(comunicacion->memoria_mapa_nombre,
                 O_CREAT | O_RDWR | O_EXCL, 0666);
    if (comunicacion->memoria_mapa_fd == -1)
        fatal("Error creando shm mapa");
    if (ftruncate(comunicacion->memoria_mapa_fd, arena->size_mapa) == -1)
        fatal("Error truncando shm mapa");

    arena->mapa = mmap(NULL,
                arena->size_mapa, PROT_READ | PROT_WRITE, MAP_SHARED, comunicacion->memoria_mapa_fd, 0);
    if (arena->mapa == MAP_FAILED)
        fatal("Error mapeando shm mapa");

    comunicacion->memoria_estado_fd =
        shm_open(comunicacion->memoria_estado_nombre,
                 O_CREAT | O_RDWR | O_EXCL, 0666);
    if (comunicacion->memoria_estado_fd == -1)
        fatal("Error creando shm estado");
    if (ftruncate(comunicacion->memoria_estado_fd, arena->size_estado) == -1)
        fatal("Error truncando shm estado");

    arena->estado =  mmap(NULL,
                  arena->size_estado, PROT_READ | PROT_WRITE, MAP_SHARED, comunicacion->memoria_estado_fd, 0);
    if (arena->estado == MAP_FAILED)
        fatal("Error mapeando shm estado");
}

void cargarArchivoConfiguracion(struct Arena *arena, char ruta[]) {
    FILE *archivo = fopen(ruta, "r");
    if (!archivo) {
        perror("🚫 No se pudo abrir el archivo de configuraciones");
        exit(EXIT_FAILURE);
    }

    char linea[100];

    // FIX acá puede tirar excepción si config.properties está vacio
    while (fgets(linea, sizeof(linea), archivo))
    {
        // Buscar la clave y el valor
        char *separador = strchr(linea, '=');
        if (!separador)
            continue;

        *separador = '\0'; // Separar la línea en dos strings
        char *clave = linea;
        char *valor = separador + 1;

        // Eliminar salto de línea del valor
        valor[strcspn(valor, "\r\n")] = 0;

        // Comparar claves
        if (strcmp(clave, "max_guardianes") == 0)
            // max_guardianes = atoi(valor);
            arena->max_guardianes = atoi(valor);
        else if (strcmp(clave, "max_tesoros") == 0)
            // max_tesoros = atoi(valor);
            arena->max_tesoros = atoi(valor);
        else if (strcmp(clave, "max_raiders") == 0)
            // max_raiders = atoi(valor);
            arena->max_raiders = atoi(valor);
    }

    fclose(archivo);
}

void cargarArchivoMapa(struct Arena *arena, char ruta[]) {
    FILE *archivo = fopen(ruta, "r");
    if (!archivo) {
        perror("🚫 No se pudo abrir el archivo del mapa");
        exit(EXIT_FAILURE);
    }

    int fila = 0, columna = 0;
    int c;
    while ((c = fgetc(archivo)) != EOF && fila < FILAS) {
        if (c == '\n' || c == '\r') {
            continue;
        }
        arena->mapa[fila][columna++] = (char)c;
        if (columna == COLUMNAS) {
            columna = 0;
            fila++;
        }
        // printf("guardó: %c\n",  mapa[fila][columna-1]);
    }

    fclose(archivo);
}

void abrirMensajeria(struct Comunicacion *comunicacion) {
    // Nuestros mailboxes
    comunicacion->mailbox_solicitudes_id = msgget(comunicacion->mailbox_solicitudes_clave, 0777 | IPC_CREAT);
    if (comunicacion->mailbox_solicitudes_id == -1) {
        perror("🚫 Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
    // Mailboxes de los demás}
    comunicacion->mailbox_directorio_solicitudes_id = msgget(MAILBOX_KEY, 0);
    if (comunicacion->mailbox_directorio_solicitudes_id == -1) {
        perror("🚫 Error al abrir mailbox de solicitudes de directorio");
    }
    comunicacion->mailbox_directorio_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0);
    if (comunicacion->mailbox_directorio_respuestas_id == -1) {
        perror("🚫 Error al abrir mailbox de respuestas de directorio");
    } 
}

void generarTesoros(struct Arena *arena) {
    int fila, columna, i = 0;
    while (arena->estado->cant_tesoros < arena->max_tesoros && i < MAX_TESOROS) {
        fila = RANDOM_FILAS();
        columna = RANDOM_COLMS();
        if (arena->mapa[fila][columna] == VACIO) {
            arena->mapa[fila][columna] = TESORO;
            arena->tesoros[i].id = i;
            arena->tesoros[i].posicion = (struct Posicion){fila, columna};
            arena->estado->cant_tesoros++;
            i++;
        }
    }
}

void designArena(char mapa[FILAS][COLUMNAS]) {
    int i, j;
    for (i = 0; i < FILAS; ++i) {
        for (j = 0; j < COLUMNAS; ++j) {
            if (i == 0 || i == FILAS - 1 || j == 0 || j == COLUMNAS - 1) {
                mapa[i][j] = -1;
            } else {
                mapa[i][j] = (rand() % 10 < 2) ? -1 : 0;
            }
        }
    }
}