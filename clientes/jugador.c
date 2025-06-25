#include "juego_constantes.h"
#include <ncurses.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include "../catacumbas/catacumbas.h"

char caracter_atrapable;
char caracter_jugador;
char (*map)[WIDTH + 1];
extern char selected_role[50];
extern char selected_map[50];

// Variables para comunicación con el servidor
int mailbox_servidor_id = -1;
int mailbox_cliente_id = -1;
char *mapa_compartido = NULL;

// Función para conectar con el servidor de catacumbas
int conectar_servidor(char *nombre_catacumba) {
    
    // Crear mailbox único para este cliente
    key_t key_cliente = ftok("/tmp",getpid());
    mailbox_cliente_id = msgget(key_cliente, IPC_CREAT | 0666);
    if (mailbox_cliente_id == -1) {
        perror("Error creando mailbox cliente");
        return -1;
    }
    
    // Conectar al mailbox del servidor (usando el nombre de la catacumba)
    key_t key_servidor = ftok(nombre_catacumba, MAILBOX_SOLICITUDES_SUFIJO);
    mailbox_servidor_id = msgget(key_servidor, 0666);
    if (mailbox_servidor_id == -1) {
        perror("Error conectando al servidor");
        return -1;
    }
    
    return 0;
}

// Función para obtener la memoria compartida del mapa
int obtener_memoria_compartida(char *nombre_catacumba) {
    char nombre_mapa[256];
    char nombre_estado[256];
    
    // Construir nombres de memoria compartida
    snprintf(nombre_mapa, sizeof(nombre_mapa), "%s%s", MEMORIA_MAPA_PREFIJO, nombre_catacumba);
    snprintf(nombre_estado, sizeof(nombre_estado), "%s%s", MEMORIA_ESTADO_PREFIJO, nombre_catacumba);
    
    // Abrir memoria compartida del mapa
    int fd_mapa = shm_open(nombre_mapa, O_RDONLY, 0664);
    if (fd_mapa == -1) {
        perror("Error abriendo memoria compartida del mapa");
        return -1;
    }
    
    // Mapear memoria del mapa
    mapa_compartido = mmap(NULL, FILAS * COLUMNAS, PROT_READ, MAP_SHARED, fd_mapa, 0);
    if (mapa_compartido == MAP_FAILED) {
        perror("Error mapeando memoria del mapa");
        close(fd_mapa);
        return -1;
    }
    close(fd_mapa);
}

// Función para enviar solicitud de conexión al servidor
int enviar_conexion(int tipo_jugador, int fila, int columna) {
    struct SolicitudServidor solicitud;
    struct RespuestaServidor respuesta;
    
    solicitud.mtype = getpid();
    solicitud.codigo = CONEXION;
    solicitud.clave_mailbox_respuestas = mailbox_cliente_id;
    solicitud.fila = fila;
    solicitud.columna = columna;
    solicitud.tipo = (tipo_jugador == JUGADOR_EXPLORADOR) ? RAIDER : GUARDIAN; //futuro caracter_jugador/tipo_jugador
    
    // Enviar solicitud
    if (msgsnd(mailbox_servidor_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1) {
        perror("Error enviando solicitud de conexión");
        return -1;
    }
    
    // Recibir respuesta
    if (msgrcv(mailbox_cliente_id, &respuesta, sizeof(respuesta) - sizeof(long), mi_pid, 0) == -1) {
        perror("Error recibiendo respuesta de conexión");
        return -1;
    }
    
    return respuesta.codigo;
}

// Función para enviar movimiento al servidor
int enviar_accion(int nueva_fila, int nueva_columna,int tipo_mensaje) {
    struct SolicitudServidor solicitud;
    
    solicitud.mtype = getpid();
    solicitud.codigo = tipo_mensaje;
    solicitud.clave_mailbox_respuestas = mailbox_cliente_id;
    solicitud.fila = nueva_fila;
    solicitud.columna = nueva_columna;
    solicitud.tipo = (tipo_jugador == JUGADOR_EXPLORADOR) ? RAIDER : GUARDIAN; //futuro caracter_jugador/tipo_jugador
    
    if (msgsnd(mailbox_servidor_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1) {
        perror("Error enviando movimiento");
        return -1;
    }
    
    return 0;
}
void draw_static_header() {
    // Dibujar barra superior fija con información del juego
    attron(COLOR_PAIR(4));
    
    // Línea superior del marco
    
    // Información del juego
    mvprintw(1, 0, "|");
    mvprintw(1, 2, "ROL: %s                          ", selected_role);
    mvprintw(1, 25, "|");
    mvprintw(1, 27, "MAPA: %s                        ", selected_map);
    
    // Línea inferior del marco
    
    attroff(COLOR_PAIR(4));
}

void draw_map()
{
    // SIEMPRE dibujar el header fijo primero
    draw_static_header();
    
    // Dibujar el mapa desplazado 4 líneas hacia abajo (después del header)
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            if (map[y][x] == CELDA_PARED) {
                //Paredes
                attron(COLOR_PAIR(1));
                mvaddch(y + 4, x + 2, map[y][x]); // +4 para el header, +2 para margen
                attroff(COLOR_PAIR(1));
            } else if (map[y][x] == CELDA_VACIA) {
                //Espacio caminable
                attron(COLOR_PAIR(3));
                mvaddch(y + 4, x + 2, map[y][x]);
                attroff(COLOR_PAIR(3));
            } else if (map[y][x] == CELDA_TESORO) {
                //Tesoro
                attron(COLOR_PAIR(2));
                mvaddch(y + 4, x + 2, map[y][x]);
                attroff(COLOR_PAIR(2));
            } else {
                mvaddch(y + 4, x + 2, map[y][x]);
            }
        }
    }
}

// Función para saber si una celda es caminable
bool es_caminable(char celda) {
    return celda == CELDA_VACIA || celda == caracter_atrapable;
}

// Función para saber si un caracter es atrapable
bool es_atrapable(char celda) {
    return celda == caracter_atrapable;
}

void terminarPartida(int posicion_x, int posicion_y) {
    endwin();
    munmap(map, HEIGHT * (WIDTH + 1));
    enviar_accion(posicion_x, posicion_y, DESCONEXION);
    msgctl(mailbox_cliente_id, IPC_RMID, NULL);
}

int jugar_partida(int tipo, char *shm_name) {

    int px = 1;
    int py = 1;
    Posicion jugador_posicion = {0, 0};

    switch (tipo) {
        case JUGADOR_EXPLORADOR:
            caracter_jugador = CELDA_EXPLORADOR;
            caracter_atrapable = CELDA_TESORO;
            break;
        case JUGADOR_GUARDIAN:
            caracter_jugador = CELDA_GUARDIAN;
            caracter_atrapable = CELDA_EXPLORADOR;
            break;
        default:
            printf("Tipo de jugador no válido.\n");
            return 1;
    }
    

    // conectar con servidor
    conectar_servidor(shm_name);
    enviar_conexion(tipo_jugador, px, py);
    obtener_memoria_compartida(shm_name);
    

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
        case KEY_UP:    new_py--; break;
        case KEY_DOWN:  new_py++; break;
        case KEY_LEFT:  new_px--; break;
        case KEY_RIGHT: new_px++; break;
        }

        if (es_caminable(map[new_py][new_px]))
        {
            jugador_posicion.posX = new_px;
            jugador_posicion.posY = new_py;
            if (es_atrapable(map[new_px][new_py]))
            {
                // Implementar mensajes al servidor
                mvprintw(HEIGHT + 1, 0, "¡Ganaste! Toca 'q' para salir del juego");
            }else{
                // Implementar mensajes al servidor
            }
        }

        draw_map();
        mvaddch(py, px, caracter_jugador);
        refresh();
    }
    terminarPartida();
    return 0;
}