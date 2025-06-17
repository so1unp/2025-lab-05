#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define _GNU_SOURCE
#include "catacumbas.h"



// =========================
//      VARIABLES GLOBALES
// =========================
struct Tesoro tesoros[MAX_TESOROS];
struct Jugador jugadores[MAX_JUGADORES];
char (*mapa)[COLUMNAS];
struct Estado *estado;
int max_tesoros; //añadir despues al struct Estado
int max_guardianes; //añadir despues al struct Estado
int max_raiders; //añadir despues al struct Estado
struct Jugador* jugador;
int mailbox_solicitudes_id, mailbox_movimientos_id;
int recibido;
int size_mapa = sizeof(char) * FILAS * COLUMNAS;
int size_estado = sizeof(struct Estado);

char shm_mapa_nombre[128];
char shm_estado_nombre[128];
int shm_mapa_fd, shm_estado_fd;

// ver despues, es solo para armar el nombre dado un numero randon y un prefijo
int catacumba_id = 0; 


// =========================
//      UTILS
// =========================
void fatal(char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Dibujar
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
void mostrarMapa(char mapa[FILAS][COLUMNAS]) {
    printf("\n=== MAPA DE LA CATACUMBA ===\n");
    int i,j;
    for (i = 0; i < FILAS; i++) {
        for (j = 0; j < COLUMNAS; j++) {
            char c;
            const char *color = ANSI_RESET;

            if (mapa[i][j] == PARED) {
                c = '#';  
                color = ANSI_BLUE;
            } else if (mapa[i][j] == VACIO) {
                c = ' ';  
            } else if (mapa[i][j] >= TESORO_OFFSET && mapa[i][j] <= MAX_TESOROS) {
                c = '$';  
                color = ANSI_YELLOW;
            } else {
                c = 'J';
                color = ANSI_RED;
            }
            // putchar(c);
            printf("%s%c%s", color, c, ANSI_RESET);

        }
        putchar('\n');
    }
    printf("===========================\n");
}










// =========================
//      MENSAJERIA
// =========================
void abrirMensajeria(){
    // TODO
    // Unificar mailbox para usar un solo struct
    // Abrir mailbox solicitudes
    mailbox_solicitudes_id = msgget(MAILBOX_SOLICITUD_KEY, 0666 | IPC_CREAT);
    if (mailbox_solicitudes_id == -1) {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
    mailbox_movimientos_id = msgget(MAILBOX_MOVIMIENTO_KEY, 0666 | IPC_CREAT);
    if (mailbox_movimientos_id == -1) {
        perror("Error al crear el mailbox de respuestas");
        exit(EXIT_FAILURE);
    }
}

#define MSG_MOVE 1
#define MSG_LOGIN 2

struct msg_move 
{
    long mtype;
    struct Jugador player_move;
};

struct msg_logIn 
{
    long mtype;
    struct Jugador player_move;
};

union msg {
    long mtype;
    struct msg_move move;
    struct msg_logIn logIn;
};

/**
 * @brief enviar un mensaje a un cliente
 *  
 */
void enviarRespuesta(struct Jugador *jugador, int codigo, char *mensaje[]){

}

/**
 * @brief Revisar los mailboxes en busca de solicitudes posibles
 *  
 */
void recibirSolicitudes(int mailBox){
    // fijate que no sea bloqueante el msgrcv
    while (1)
    {
        
    }
    
}









// =========================
//      GESTION MAPA
// =========================

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

/**
 * @brief Verificar que se puede aceptar al jugador
 *
 * Se aceptará si el servidor tiene espacio para otro jugador
 * y en el equipo que seleccionó
 * 
 * @param jugador 
 * @return código de resultado
 */
int aceptarJugador(struct Jugador *jugador){
    
}

/**
 * @brief Genera un jugador en la zona exterior del mapa.
 * 
 * La función intenta encontrar una posición vacía en un anillo exterior
 * del mapa, definido como la zona entre la mitad del radio del mapa y el borde.
 * Si no encuentra ninguna posición válida en esa zona (por ejemplo, porque está
 * llena de paredes u otros jugadores), recurre a la estrategia de
 * buscar cualquier celda vacía en todo el mapa.
 * Una vez encontrada una posición, actualiza las coordenadas en la struct Jugador
 * y modifica el mapa para marcar la celda como ocupada.
 * 
 * @param jugador Un puntero a la estructura del jugador que se va a 'spawnear'. 
 *                La función actualizará la posición de este jugador.
 */
void spawnearJugador(struct Jugador *jugador){
    int centro_fila = FILAS / 2;
    int centro_columna = COLUMNAS / 2;
    // Usamos la distancia de Chebyshev (máxima diferencia de coordenadas) como "radio"
    int max_radius = (FILAS > COLUMNAS) ? (FILAS / 2) : (COLUMNAS / 2);
    int min_radius = max_radius / 2;

    struct Posicion valid_spawns[FILAS * COLUMNAS];
    int count = 0;
    int fila, columna;

    for (fila = 0; fila < FILAS; fila++) {
        for (columna = 0; columna < COLUMNAS; columna++) {
            if (mapa[fila][columna] == VACIO) {
                int dist_fila = abs(fila - centro_fila);
                int dist_columna = abs(columna - centro_columna);
                int distancia = (dist_fila > dist_columna) ? dist_fila : dist_columna;

                if (distancia > min_radius) {
                    valid_spawns[count].fila = fila;
                    valid_spawns[count].columna = columna;
                    count++;
                }
            }
        }
    }

    if (count > 0) {
        int choice = rand() % count;
        fila = valid_spawns[choice].fila;
        columna = valid_spawns[choice].columna;

        jugador->posicion.fila = fila;
        jugador->posicion.columna = columna;
        
        mapa[fila][columna] = MAX_TESOROS + 1;
    } else {
        // Fallback: si no hay lugar en el anillo exterior, buscar en cualquier lado.
        do {
            fila = rand() % FILAS;
            columna = rand() % COLUMNAS;
        } while (mapa[fila][columna] != VACIO);

        jugador->posicion.fila = fila;
        jugador->posicion.columna = columna;
        mapa[fila][columna] = MAX_TESOROS + 1; 
    }
}

void regenerarMapa(){
    // actualizar mapa en base a todo lo que hay (nuevas posiciones)
}

// Este no hay que hacerlo
// ya está hecho en server-epullan.c pero lo adaptamos después
void generarTesoro(){

}






// =========================
//      MEMORIA
// =========================
void abrirMemoria(){
    shm_mapa_fd = 
    shm_open(shm_mapa_nombre,
            O_CREAT | O_RDWR | O_EXCL, 0664);
    if (shm_mapa_fd == -1) fatal("Error creando shm mapa");
    if (ftruncate(shm_mapa_fd, size_mapa) == -1) fatal("Error truncando shm mapa");

    mapa = mmap(NULL,
            size_mapa, PROT_READ | PROT_WRITE, MAP_SHARED, shm_mapa_fd, 0);
    if (mapa == MAP_FAILED) fatal("Error mapeando shm mapa");

    shm_estado_fd = 
        shm_open(shm_estado_nombre,
                O_CREAT | O_RDWR | O_EXCL, 0664);
    if (shm_estado_fd == -1) fatal("Error creando shm estado");
    if (ftruncate(shm_estado_fd, size_estado) == -1) fatal("Error truncando shm estado");

    estado =  mmap(NULL,
        size_estado, PROT_READ | PROT_WRITE, MAP_SHARED, shm_estado_fd, 0);
    if (estado == MAP_FAILED) fatal("Error mapeando shm estado");    
}


void cargarArchivoConfiguracion(char ruta[]){
    FILE *archivo = fopen(ruta, "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo");
        exit(EXIT_FAILURE);
    }

    char linea[100];

    while (fgets(linea, sizeof(linea), archivo)) {
        // Buscar la clave y el valor
        char *separador = strchr(linea, '=');
        if (!separador) continue;

        *separador = '\0'; // Separar la línea en dos strings
        char *clave = linea;
        char *valor = separador + 1;

        // Eliminar salto de línea del valor
        valor[strcspn(valor, "\r\n")] = 0;

        // Comparar claves
        if (strcmp(clave, "max_guardianes") == 0)
            max_guardianes = atoi(valor);
        else if (strcmp(clave, "max_tesoros") == 0)
            max_tesoros = atoi(valor);
        else if (strcmp(clave, "max_raiders") == 0)
            max_raiders = atoi(valor);
    }

    fclose(archivo);
}

void cargarArchivoMapa(char ruta[]){
    FILE *archivo = fopen(ruta, "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo");
        exit(EXIT_FAILURE);
    }

    int fila = 0, columna = 0;
    int c;
    while ((c = fgetc(archivo)) != EOF && fila < FILAS) {
        if (c == '\n' || c == '\r') {
            continue;
        }
        mapa[fila][columna++] = (char)c;
        if (columna == COLUMNAS) {
            columna = 0;
            fila++;
        }
    }

    fclose(archivo);
}








// =========================
//      MAIN
// =========================


void setup(){

    abrirMensajeria();

    // Formar los nombres
    snprintf(shm_mapa_nombre, sizeof(shm_mapa_nombre), 
        SHM_MAPA_PREFIX "%s", catacumbas[catacumba_id]);
    
    snprintf(shm_estado_nombre, sizeof(shm_estado_nombre), 
        SHM_ESTADO_PREFIX "%s", catacumbas[catacumba_id]);

    // Abrir memorias compartidas
    abrirMemoria();

    // Generar el mapa
    designArena(mapa);

    // Generar el estado
    memset(estado, 0, sizeof(struct Estado));
    estado->max_jugadores = MAX_JUGADORES;

}


int main(int argc, char* argv[])
{

    setup();

    mostrarMapa(mapa);

    /*
    // RECIBIR SOLICITUDES
    while (1) { 
        // TODO: espera solicitud conexion.
        solicitudJugador(&recibido,mailbox_solicitudes_id, &solicitud);

        if (!ingresarJugador(solicitud.jugador, mapa)){
            snprintf(respuesta.mensaje, MAX_LONGITUD_MENSAJES, "Intento fallido, no se conecto jugador");
            respuesta.clave_mailbox_movimientos = 0;
        }  else {
            snprintf(respuesta.mensaje, MAX_LONGITUD_MENSAJES, "Jugador conectado con éxito");                respuesta.clave_mailbox_movimientos = MAILBOX_MOVIMIENTO_KEY;
            strncpy(respuesta.nombre_memoria_mapa, shm_mapa_nombre, 
                sizeof(respuesta.nombre_memoria_mapa));
            // asegurarse que termine bien el nombre.
            respuesta.nombre_memoria_mapa[sizeof(respuesta.nombre_memoria_mapa) - 1] = '\0';
        }
        respuesta.mtype = solicitud.jugador.pid;
        responder(solicitud.clave_mailbox_respuestas, &respuesta);
    }
    */

    if (shm_unlink(shm_mapa_nombre) < 0) fatal("Error al borrar memoria mapa");
    if (shm_unlink(shm_estado_nombre) < 0) fatal("Error al borrar memoria estado");
    munmap(mapa, size_mapa);
    munmap(estado, size_estado);
    close(shm_mapa_fd);
    close(shm_estado_fd);

    exit(EXIT_SUCCESS);
}
