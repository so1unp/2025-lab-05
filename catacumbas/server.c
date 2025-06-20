#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "catacumbas.h"
#include "../directorio/directorio.h"
#include <signal.h>
#define _GNU_SOURCE




// =========================
//      VARIABLES GLOBALES
// =========================
struct Tesoro tesoros[MAX_TESOROS];
struct Jugador jugadores[MAX_JUGADORES];
struct Estado *estado;
char (*mapa)[COLUMNAS];
int max_guardianes;
int max_raiders;
int max_tesoros;
int size_mapa = sizeof(char) * FILAS * COLUMNAS;
int size_estado = sizeof(struct Estado);
char memoria_mapa_nombre[128];
char memoria_estado_nombre[128];
int mailbox_solicitudes_clave;
int memoria_mapa_fd, memoria_estado_fd, mailbox_solicitudes_id;
int mailbox_directorio_solicitudes_id, mailbox_directorio_respuestas_id;





// =========================
//      UTILS
// =========================
void fatal(char msg[])
{
    perror(msg);
    exit(EXIT_FAILURE);
}

// Dibujar
#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
void mostrarMapa()
{
    printf("\n=== MAPA DE LA CATACUMBA ===\n");
    int i, j;
    for (i = 0; i < FILAS; i++)
    {
        for (j = 0; j < COLUMNAS; j++)
        {
            const char *color = ANSI_RESET;

            if (mapa[i][j] == PARED)
            {
                color = ANSI_BLUE;
            }
            else if (mapa[i][j] == TESORO)
            {
                color = ANSI_YELLOW;
            }
            else
            {
                color = ANSI_RESET;
            }
            printf("%s%c%s", color, mapa[i][j], ANSI_RESET);
        }
        putchar('\n');
    }
    printf("===========================\n");
}









// =========================
//      MENSAJERIA
// =========================


/**
 * @brief enviar un mensaje a un cliente
 *  
 */
void enviarRespuestaCliente(struct Jugador *jugador, int codigo, char *mensaje[]){

}

/**
 * @brief Revisar los mailboxes en busca de solicitudes posibles
 *  
 */
void recibirSolicitudesClientes(int mailBox){
    // fijate que no sea bloqueante el msgrcv
    while (1)
    {
        
    }
    
}

void enviarSolicitudDirectorio() {

    struct solicitud solicitud_directorio;

    solicitud_directorio.mtype = getpid();
    solicitud_directorio.tipo = OP_AGREGAR;
    snprintf(
        solicitud_directorio.texto,                 
        sizeof(solicitud_directorio.texto),        
        "%s|%s|%d",
        "nombre_generico_1",
        memoria_mapa_nombre,
        mailbox_solicitudes_clave
    );

    printf("%s\n", solicitud_directorio.texto);

    msgsnd(mailbox_directorio_solicitudes_id, &solicitud_directorio, sizeof(struct solicitud), 0);
}





// =========================
//      GESTION MAPA
// =========================

void designArena(char mapa[FILAS][COLUMNAS])
{
    int i, j;
    for (i = 0; i < FILAS; ++i)
    {
        for (j = 0; j < COLUMNAS; ++j)
        {
            if (i == 0 || i == FILAS - 1 || j == 0 || j == COLUMNAS - 1)
            {
                mapa[i][j] = -1;
            }
            else
            {
                mapa[i][j] = (rand() % 10 < 2) ? -1 : 0;
            }
        }
    }
}

/**
 * @brief Regenera el estado del mapa en la memoria compartida.
 *
 * Esta función limpia el mapa de todas las entidades móviles (jugadores) y
 * luego vuelve a dibujar a cada jugador y tesoro en su posición actual
 * basándose en los arreglos globales 'jugadores' y 'tesoros'.
 * Las paredes permanecen intactas.
 * Esto asegura que el mapa compartido siempre refleje el estado más reciente
 * del juego.
 */
void regenerarMapa()
{
    printf("[LOG] Regenerando el mapa con %d jugadores y %d tesoros.\n",
           estado->cant_jugadores, estado->cant_tesoros);
    int i, j;
    for (i = 0; i < FILAS; i++)
    {
        for (j = 0; j < COLUMNAS; j++)
        {
            if (mapa[i][j] != VACIO && mapa[i][j] != PARED)
            {
                mapa[i][j] = VACIO;
            }
        }
    }

    for (i = 0; i < estado->cant_jugadores; i++)
    {
        int fila = jugadores[i].posicion.fila;
        int columna = jugadores[i].posicion.columna;
        char tipo = jugadores[i].tipo;

        if (fila >= 0 && fila < FILAS && columna >= 0 && columna < COLUMNAS)
        {
            mapa[fila][columna] = tipo;
        }
    }

    for (i = 0; i < estado->cant_tesoros; i++)
    {
        int fila = tesoros[i].posicion.fila;
        int columna = tesoros[i].posicion.columna;

        if (fila >= 0 && fila < FILAS && columna >= 0 && columna < COLUMNAS)
        {
            if (mapa[fila][columna] == VACIO)
            {
                mapa[fila][columna] = TESORO;
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
 * @param jugador Puntero a la estructura del jugador que intenta conectarse.
 *                Se utiliza para verificar el equipo solicitado
 * (`jugador->tipo`).
 * @return Devuelve 1 si el jugador puede ser aceptado, 0 en caso contrario
 *         (servidor lleno, equipo lleno o tipo de jugador inválido).
 */
int aceptarJugador(struct Jugador *jugador)
{
    if (estado->cant_jugadores >= MAX_JUGADORES)
    {
        printf("[LOG] Rechazado: servidor lleno (%d/%d).\n", estado->cant_jugadores,
               MAX_JUGADORES);
        return EXIT_SUCCESS;
    }

    switch (jugador->tipo)
    {
    case RAIDER:
        if (estado->cant_raiders >= MAX_RAIDERS)
        {
            printf("[LOG] Rechazado: equipo RAIDER lleno (%d/%d).\n",
                   estado->cant_raiders, MAX_RAIDERS);
            return EXIT_SUCCESS;
        }
        break;

    case GUARDIAN:
        if (estado->cant_guardianes >= MAX_GUARDIANES)
        {
            printf("[LOG] Rechazado: equipo GUARDIAN lleno (%d/%d).\n",
                   estado->cant_guardianes, MAX_GUARDIANES);
            return EXIT_SUCCESS;
        }
        break;

    default:
        printf("[LOG] Rechazado: tipo de jugador desconocido '%c'.\n",
               jugador->tipo);
        return EXIT_SUCCESS;
    }

    printf("[LOG] Jugador '%s' (tipo: %c) puede ser aceptado.\n", jugador->nombre,
           jugador->tipo);
    return EXIT_FAILURE;
}

/**
 * @brief Genera un jugador en la zona exterior del mapa.
 *
 * La función intenta encontrar una posición vacía en un anillo exterior
 * del mapa, definido como la zona entre la mitad del radio del mapa y el borde.
 * Si no encuentra ninguna posición válida en esa zona (por ejemplo, porque está
 * llena de paredes u otros jugadores), recurre a la estrategia de
 * buscar cualquier celda vacía en todo el mapa.
 * Una vez encontrada una posición, actualiza las coordenadas en la struct
 * Jugador y modifica el mapa para marcar la celda como ocupada.
 *
 * @param jugador Un puntero a la estructura del jugador que se va a 'spawnear'.
 *                La función actualizará la posición de este jugador.
 */
void spawnearJugador(struct Jugador *jugador)
{
    int centro_fila = FILAS / 2;
    int centro_columna = COLUMNAS / 2;
    int max_radius = (FILAS > COLUMNAS) ? (FILAS / 2) : (COLUMNAS / 2);
    int min_radius = max_radius / 2;

    struct Posicion valid_spawns[FILAS * COLUMNAS];
    int count = 0;
    int fila, columna;

    for (fila = 0; fila < FILAS; fila++)
    {
        for (columna = 0; columna < COLUMNAS; columna++)
        {
            if (mapa[fila][columna] == VACIO)
            {
                int dist_fila = abs(fila - centro_fila);
                int dist_columna = abs(columna - centro_columna);
                int distancia = (dist_fila > dist_columna) ? dist_fila : dist_columna;

                if (distancia > min_radius)
                {
                    valid_spawns[count].fila = fila;
                    valid_spawns[count].columna = columna;
                    count++;
                }
            }
        }
    }

    if (count > 0)
    {
        int choice = rand() % count;
        fila = valid_spawns[choice].fila;
        columna = valid_spawns[choice].columna;

        jugador->posicion.fila = fila;
        jugador->posicion.columna = columna;

        mapa[fila][columna] = jugador->tipo;
    }
    else
    {
        // Fallback: si no hay lugar en el anillo exterior, buscar en cualquier
        // lado.
        do
        {
            fila = rand() % FILAS;
            columna = rand() % COLUMNAS;
        } while (mapa[fila][columna] != VACIO);

        jugador->posicion.fila = fila;
        jugador->posicion.columna = columna;
        mapa[fila][columna] = jugador->tipo;
    }
    printf("[LOG] Jugador '%s' spawneado en (%d, %d).\n", jugador->nombre,
           jugador->posicion.fila, jugador->posicion.columna);
}

// Este no hay que hacerlo
// ya está hecho en server-epullan.c pero lo adaptamos después
void generarTesoro() {}





// =========================
//      SETUP
// =========================
void abrirMensajeria()
{
    // TODO
    // Unificar mailbox para usar un solo struct
    // Abrir mailbox solicitudes
    mailbox_solicitudes_id = msgget(mailbox_solicitudes_clave, 0777 | IPC_CREAT);
    if (mailbox_solicitudes_id == -1)
    {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
    mailbox_directorio_solicitudes_id = msgget(MAILBOX_KEY, 0777);
    if (mailbox_directorio_solicitudes_id == -1)
    {
        perror("Error al abrir mailbox de solicitudes de directorio");
    }

    mailbox_directorio_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0777);
    if (mailbox_directorio_respuestas_id == -1)
    {
        perror("Error al abrir mailbox de respuestas de directorio");
    }
}
void abrirMemoria()
{

    memoria_mapa_fd =
        shm_open(memoria_mapa_nombre,
                 O_CREAT | O_RDWR | O_EXCL, 0777);
    if (memoria_mapa_fd == -1)
        fatal("Error creando shm mapa");
    if (ftruncate(memoria_mapa_fd, size_mapa) == -1)
        fatal("Error truncando shm mapa");

    mapa = mmap(NULL,
                size_mapa, PROT_READ | PROT_WRITE, MAP_SHARED, memoria_mapa_fd, 0);
    if (mapa == MAP_FAILED)
        fatal("Error mapeando shm mapa");

    memoria_estado_fd =
        shm_open(memoria_estado_nombre,
                 O_CREAT | O_RDWR | O_EXCL, 0777);
    if (memoria_estado_fd == -1)
        fatal("Error creando shm estado");
    if (ftruncate(memoria_estado_fd, size_estado) == -1)
        fatal("Error truncando shm estado");

    estado = mmap(NULL,
                  size_estado, PROT_READ | PROT_WRITE, MAP_SHARED, memoria_estado_fd, 0);
    if (estado == MAP_FAILED)
        fatal("Error mapeando shm estado");
}

void cargarArchivoConfiguracion(char ruta[])
{
    FILE *archivo = fopen(ruta, "r");
    if (!archivo)
    {
        perror("No se pudo abrir el archivo");
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
            max_guardianes = atoi(valor);
        else if (strcmp(clave, "max_tesoros") == 0)
            max_tesoros = atoi(valor);
        else if (strcmp(clave, "max_raiders") == 0)
            max_raiders = atoi(valor);
    }

    fclose(archivo);
}

void cargarArchivoMapa(char ruta[])
{
    FILE *archivo = fopen(ruta, "r");
    if (!archivo)
    {
        perror("No se pudo abrir el archivo");
        exit(EXIT_FAILURE);
    }

    int fila = 0, columna = 0;
    int c;
    while ((c = fgetc(archivo)) != EOF && fila < FILAS)
    {
        if (c == '\n' || c == '\r')
        {
            continue;
        }
        mapa[fila][columna++] = (char)c;
        if (columna == COLUMNAS)
        {
            columna = 0;
            fila++;
        }
        // printf("guardó: %c\n",  mapa[fila][columna-1]);
    }

    fclose(archivo);
}


void finish()
{
    if (shm_unlink(memoria_mapa_nombre) < 0)
        fatal("Error al borrar memoria mapa");
    if (shm_unlink(memoria_estado_nombre) < 0)
        fatal("Error al borrar memoria estado");
    munmap(mapa, size_mapa);
    munmap(estado, size_estado);
    close(memoria_mapa_fd);
    close(memoria_estado_fd);
    if (msgctl(mailbox_solicitudes_id, IPC_RMID, NULL) == -1)
    {
        perror("Error al eliminar el buzón de solicitudes");
        exit(1);
    }
    printf("Programa terminado\n");
    exit(EXIT_SUCCESS);
}

void setup(char rutaMapa[], char rutaConfig[])
{
    // El orden es importante aqui
    
    // Comunicación
    // Armar nombres y claves
    snprintf(memoria_mapa_nombre, sizeof(memoria_mapa_nombre), "%s%d", MEMORIA_MAPA_PREFIJO, getpid());
    snprintf(memoria_estado_nombre, sizeof(memoria_mapa_nombre), "%s%d", MEMORIA_ESTADO_PREFIJO, getpid());
    mailbox_solicitudes_clave = getpid() * MAILBOX_SOLICITUDES_SUFIJO;
    
    abrirMemoria();
    abrirMensajeria();
    
    // Inicializaciones
    cargarArchivoConfiguracion(rutaConfig);
    cargarArchivoMapa(rutaMapa);
        
    // Generar el estado
    memset(estado, 0, sizeof(struct Estado));
    estado->max_jugadores = MAX_JUGADORES;
    
    // Preparar la señal para detener el proceso y eliminar las cosas
    // que haya creado
    signal(SIGINT, finish);

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR DE CATACUMBA INICIADO 🗿🚬                 \n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    printf("- Mapa: %s\n", memoria_mapa_nombre);
    printf("- Estado: %s\n", memoria_estado_nombre);
    printf("- Mailbox: %i\n", mailbox_solicitudes_clave);
    printf("- El mapa se cargó del archivo %s\n", rutaMapa);
    printf("- Las configuraciones se cargaron del archivo: %s\n", rutaConfig);
    printf("  - Cantidad máxima de guardianes: %i\n", max_guardianes);
    printf("  - Cantidad máxima de raiders: %i\n", max_raiders);
    printf("  - Cantidad máxima de tesoros: %i\n", max_tesoros);
}


// =========================
//      MAIN
// =========================

int main(int argc, char *argv[])
{
    setup(argv[1], argv[2]);

    // ----
    // Prueba de spawneo
    mostrarMapa();
    printf("\n[TEST] Agregando un jugador de prueba (Raider).\n");
    struct Jugador jugador_prueba;
    jugador_prueba.pid = getpid();
    jugador_prueba.tipo = RAIDER;
    strcpy(jugador_prueba.nombre, "R-Test");

    if (aceptarJugador(&jugador_prueba) == EXIT_FAILURE)
    { // EXIT_FAILURE (0) significa que se puede aceptar
        jugadores[estado->cant_jugadores] = jugador_prueba;
        if (jugador_prueba.tipo == RAIDER)
        {
            estado->cant_raiders++;
        }
        else if (jugador_prueba.tipo == GUARDIAN)
        {
            estado->cant_guardianes++;
        }
        estado->cant_jugadores++;
        spawnearJugador(&jugadores[estado->cant_jugadores - 1]);
        printf("[TEST] Jugador de prueba agregado y spawneado.\n");
    }
    else
    {
        printf("[TEST] No se pudo agregar el jugador de prueba.\n");
    }
    mostrarMapa();

    // ---
    // RECIBIR SOLICITUDES
    while (1) {
        printf("Esperando solicitudes...\n");
        struct SolicitudServidor solicitud;
        // Solicitudes del cliente
        // Dejar que se bloquee 
        // Se desbloquea al detectar un mensaje
        if (msgrcv(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 1, 0) == -1) {
            perror("msgrcv");
        } else {
            printf("Mensaje recibido: %d\n", solicitud.codigo);
        }
    }

    finish();

    exit(EXIT_SUCCESS);
}







