#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "../directorio/directorio.h"
#include "catacumbas.h"
#include "solicitudes.h"
#include "utils.h"
#define _GNU_SOURCE


// expresiones utiles para la generacion posiciones
#define RANDOM_FILAS()(1 + rand() % (FILAS-2));
#define RANDOM_COLMS()(1 + rand() % (COLUMNAS-2));

// void responderSolicitud(int clave_mailbox_respuestas, struct RespuestaServidor *respuesta);
void recibirSolicitudes();
void atenderSolicitud(struct SolicitudServidor *solicitud);

void atenderConexion(struct Jugador *jugador, struct RespuestaServidor *respuesta);
void atenderDesconexion(struct Jugador *jugador, struct RespuestaServidor *respuesta);
void atenderMovimiento(struct Jugador *jugador, struct RespuestaServidor *respuesta);
void atenderCapturaTesoro(struct Jugador *jugador, struct RespuestaServidor *respuesta);
void atenderCapturaRaider(struct Jugador *jugador, struct RespuestaServidor *respuesta);

int conectarJugador(struct Jugador *jugador);
int desconectarJugador(long pid);
int moverJugador(struct Jugador *jugador);
int capturarTesoro(struct Jugador *jugador);
int capturarRaider(struct Jugador *jugador);
int buscarJugador(long pid);
void generarTesoros();

void construirRespuesta(struct RespuestaServidor *respuesta, int codigo, const char *mensaje);

// UTILS
void consultarMostrar();


// =========================
//      VARIABLES GLOBALES
// =========================
struct Tesoro tesoros[MAX_TESOROS];
struct Jugador jugadores[MAX_JUGADORES];
struct Estado *estado;
char (*mapa)[COLUMNAS];
int max_guardianes = 0;
int max_raiders = 0;
int max_tesoros = 0;
int size_mapa = sizeof(char) * FILAS * COLUMNAS;
int size_estado = sizeof(struct Estado);
char memoria_mapa_nombre[128];
char memoria_estado_nombre[128];
int mailbox_solicitudes_clave;
int memoria_mapa_fd, memoria_estado_fd, mailbox_solicitudes_id;
int mailbox_directorio_solicitudes_id, mailbox_directorio_respuestas_id;

pthread_t hilo_directorio; // util en un futuro

// =========================
//      UTILS
// =========================
void fatal(char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
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
 * @brief Revisar el mailbox en busca de solicitudes posibles
 *  
 */
void recibirSolicitudesClientes(int mailBox){
    // fijate que no sea bloqueante el msgrcv
    while (1) {   
    }    
}

/**
 * @brief Leer el mailbox de respuestas del directorio
 * para buscar una respuesta para este servidor
 *  
 */
void recibirRespuestaDirectorio(struct respuesta *respuesta){
    printf("Recibiendo respuesta de directorio...\n");
    // MUY IMPORTANTE usar getpid() para solo recibir los mensajes de este servidor
    if (msgrcv(mailbox_directorio_respuestas_id, respuesta, sizeof(struct respuesta) - sizeof(long), getpid(), 0) == -1) {
        perror("🚫 Error al recibir respuesta de Directorio");
    } else {
        printf("Respuesta de directorio recibida:\n");
        printf("- Mtype: %li\n", respuesta->mtype);
        printf("- Código: %i\n", respuesta->codigo);
        printf("- Datos: %s\n", respuesta->datos);
        printf("- Elementos: %i\n", respuesta->num_elementos);
    }
}

/**
 * @brief Enviar una solicitud al mailbox de Directorio
 *  
 */
void enviarSolicitudDirectorio(struct solicitud *solicitud, struct respuesta *respuesta) {

    printf("Enviando solicitud a directorio:\n");
    printf("- Mtype: %li\n", solicitud->mtype);
    printf("- Tipo: %i\n", solicitud->tipo);
    printf("- Texto: %s\n", solicitud->texto);
    msgsnd(mailbox_directorio_solicitudes_id, solicitud, sizeof(struct solicitud) - sizeof(long), 0);

    recibirRespuestaDirectorio(respuesta);
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
 * @brief Regenera el estado del mapa en la memoria compartida.
 *
 * Esta función limpia el mapa de todas las entidades móviles (jugadores) y
 * luego vuelve a dibujar a cada jugador y tesoro en su posición actual
 * basándose en los arreglos globales 'jugadores' y 'tesoros'.
 * Las paredes permanecen intactas.
 * Esto asegura que el mapa compartido siempre refleje el estado más reciente
 * del juego.
 */
void regenerarMapa() {
    printf("[LOG] Regenerando el mapa con %d jugadores y %d tesoros.\n",
           estado->cant_jugadores, estado->cant_tesoros);
    int i, j;
    for (i = 0; i < FILAS; i++) {
        for (j = 0; j < COLUMNAS; j++) {
            if (mapa[i][j] != VACIO && mapa[i][j] != PARED) {
                mapa[i][j] = VACIO;
            }
        }
    }
    for (i = 0; i < estado->cant_jugadores; i++) {
        int fila = jugadores[i].posicion.fila;
        int columna = jugadores[i].posicion.columna;
        char tipo = jugadores[i].tipo;

        if (fila >= 0 && fila < FILAS && columna >= 0 && columna < COLUMNAS) {
            mapa[fila][columna] = tipo;
        }
    }
    for (i = 0; i < estado->cant_tesoros; i++) {
        int fila = tesoros[i].posicion.fila;
        int columna = tesoros[i].posicion.columna;

        if (fila >= 0 && fila < FILAS && columna >= 0 && columna < COLUMNAS) {
            if (mapa[fila][columna] == VACIO) {
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
        if (estado->cant_raiders >= max_raiders)
        {
            printf("[LOG] Rechazado: equipo RAIDER lleno (%d/%d).\n",
                   estado->cant_raiders, max_raiders);
            return EXIT_SUCCESS;
        }
        break;

    case GUARDIAN:
        if (estado->cant_guardianes >= max_guardianes)
        {
            printf("[LOG] Rechazado: equipo GUARDIAN lleno (%d/%d).\n",
                   estado->cant_guardianes, max_guardianes);
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
    int dist_fila, dist_columna, distancia;
    for (fila = 0; fila < FILAS; fila++)
    {
        for (columna = 0; columna < COLUMNAS; columna++) {
            if (mapa[fila][columna] == VACIO) {
                dist_fila = abs(fila - centro_fila);
                dist_columna = abs(columna - centro_columna);
                distancia = (dist_fila > dist_columna) ? dist_fila : dist_columna;

                if (distancia > min_radius) {
                    valid_spawns[count].fila = fila;
                    valid_spawns[count].columna = columna;
                    count++;
                }
            }
        }
    }
    int choice;
    if (count > 0) {
        choice = rand() % count;
        fila = valid_spawns[choice].fila;
        columna = valid_spawns[choice].columna;
    } else {
        // Fallback: si no hay lugar en el anillo exterior, buscar en cualquier
        // lado.
        do {
            // fila = rand() % FILAS;
            // columna = rand() % COLUMNAS;
            fila = RANDOM_FILAS();
            columna = RANDOM_COLMS();
        } while (mapa[fila][columna] != VACIO);
    }
    jugador->posicion = (struct Posicion){fila, columna};
    mapa[fila][columna] = jugador->tipo;
    printf("[LOG] Jugador '%s' spawneado en (%d, %d).\n", jugador->nombre,
           jugador->posicion.fila, jugador->posicion.columna);
}


void generarTesoros() {
    int fila, columna, i = 0;
    while (estado->cant_tesoros < max_tesoros && i < MAX_TESOROS) {
        fila = RANDOM_FILAS();
        columna = RANDOM_COLMS();
        if (mapa[fila][columna] == VACIO) {
            mapa[fila][columna] = TESORO;
            tesoros[i].id = i;
            tesoros[i].posicion = (struct Posicion){fila, columna};
            estado->cant_tesoros++;
            i++;
        }
    }
}

// =========================
//      SETUP
// =========================
void abrirMensajeria()
{
    // Nuestros mailboxes
    mailbox_solicitudes_id = msgget(mailbox_solicitudes_clave, 0777 | IPC_CREAT);
    if (mailbox_solicitudes_id == -1)
    {
        perror("🚫 Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
    // Mailboxes de los demás}
    mailbox_directorio_solicitudes_id = msgget(MAILBOX_KEY, 0);
    if (mailbox_directorio_solicitudes_id == -1)
    {
        perror("🚫 Error al abrir mailbox de solicitudes de directorio");
    }
    mailbox_directorio_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0);
    if (mailbox_directorio_respuestas_id == -1)
    {
        perror("🚫 Error al abrir mailbox de respuestas de directorio");
    }
    
}

void abrirMemoria()
{

    memoria_mapa_fd =
        shm_open(memoria_mapa_nombre,
                 O_CREAT | O_RDWR | O_EXCL, 0666);
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
                 O_CREAT | O_RDWR | O_EXCL, 0666);
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
        perror("🚫 No se pudo abrir el archivo del mapa");
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
        perror("🚫 Error al eliminar el buzón de solicitudes, por favor eliminelo manualmente con ipcs -q y luego ipcrm -q <id>");
        exit(EXIT_FAILURE);
    }
    printf("Programa terminado\n");
    exit(EXIT_SUCCESS);
}

void setup(char rutaMapa[], char rutaConfig[])
{
    // El orden es importante aqui

    // Preparar la señal para detener el proceso y eliminar las cosas
    // que haya creado
    signal(SIGINT, finish);

    // Inicializaciones
    snprintf(memoria_mapa_nombre, sizeof(memoria_mapa_nombre), "%s%d", MEMORIA_MAPA_PREFIJO, getpid());
    snprintf(memoria_estado_nombre, sizeof(memoria_mapa_nombre), "%s%d", MEMORIA_ESTADO_PREFIJO, getpid());
    mailbox_solicitudes_clave = getpid() * MAILBOX_SOLICITUDES_SUFIJO;    
    abrirMemoria();
    cargarArchivoConfiguracion(rutaConfig);
    cargarArchivoMapa(rutaMapa);

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
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    // Comunicación    
    abrirMensajeria();
    
    // Generar el estado
    memset(estado, 0, sizeof(struct Estado));
    estado->max_jugadores = max_guardianes + max_raiders;
    estado->cant_guardianes = 0;
    estado->cant_raiders = 0;
    estado->cant_jugadores = 0;
    estado->cant_tesoros = 0;
    generarTesoros();
    // Avisar al proceso directorio
    // Iniciar comunicación con directorio
    struct solicitud solicitud_directorio;
    solicitud_directorio.mtype = getpid();
    solicitud_directorio.tipo = OP_AGREGAR;
    snprintf(
        solicitud_directorio.texto,                 
        sizeof(solicitud_directorio.texto),        
        "%s|%s|%s|%d",
        "servidor_generico",
        memoria_mapa_nombre,
        memoria_estado_nombre,
        mailbox_solicitudes_clave
    );
    struct respuesta respuesta_directorio;
    enviarSolicitudDirectorio(&solicitud_directorio, &respuesta_directorio);

}

// =========================
//      MAIN
// =========================

int main(int argc, char *argv[])
{
    if (argc < 3){
        printf("Argumentos inválidos, por favor utilicelo de la siguiente forma:\n");
        printf("\n");
        printf("./server  ruta/a/mapa.txt  ruta/a/config.properties\n");
        printf("\n");
        printf("    mapa.txt:\n");
        printf("        El archivo del que se cargará el mapa.\n");
        printf("        Debe ser de 80x25 sin espacios ni líneas extra\n");        
        printf("\n");
        printf("    config.properties:\n");
        printf("        El archivo de configuraciones de mapa.\n");
        printf("        Debe especificar:\n");
        printf("        - max_tesoros=<maximo de tesoros>\n");
        printf("        - max_raiders=<maximo de raiders>\n");
        printf("        - max_guardianes=<maximo de guardianes>\n");

        exit(EXIT_FAILURE);
    }
    setup(argv[1], argv[2]);

    // ---
    // RECIBIR SOLICITUDES DE CLIENTES
    while (1) {
        printf("Esperando solicitudes...\n");
        // Solicitudes del cliente
        // Dejar que se bloquee 
        // Se desbloquea al detectar un mensaje
        struct SolicitudServidor solicitud;
        if (msgrcv(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0, 0) == -1) {
            perror("🚫 msgrcv");
        } else {
            printf("Mensaje recibido:\n");
            printf("- Mtype: %li\n", solicitud.mtype);
            printf("- Codigo: %i\n", solicitud.codigo);
            printf("- Mailbox: %i\n", solicitud.clave_mailbox_respuestas);
            printf("- Fila: %i\n", solicitud.fila);
            printf("- Columna: %i\n", solicitud.columna);
            printf("- Tipo: %c\n\n", solicitud.tipo);
        }
        atenderSolicitud(&solicitud);
    }

    finish();

    exit(EXIT_SUCCESS);
}

// 
void recibirSolicitudes() {
    while (1) {
        printf("Esperando solicitudes...\n");

        struct SolicitudServidor solicitud;
        if (msgrcv(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0, 0) == -1) {
            perror("🚫 msgrcv");
        } else {
            printf("Mensaje recibido:\n");
            printf("- Mtype: %li\n", solicitud.mtype);
            printf("- Codigo: %i\n", solicitud.codigo);
            printf("- Mailbox: %i\n", solicitud.clave_mailbox_respuestas);
            printf("- Fila: %i\n", solicitud.fila);
            printf("- Columna: %i\n", solicitud.columna);
            printf("- Tipo: %c\n\n", solicitud.tipo);
        }
        atenderSolicitud(&solicitud);
    }
}

// Delega la atencion a los distintos atender X accion
void atenderSolicitud(struct SolicitudServidor *solicitud) {
    struct Jugador jugador; // el jugador que realizo la solicitud
    jugador.pid = solicitud->mtype;
    jugador.posicion = (struct Posicion) {solicitud->fila , solicitud->columna};
    // TODO: jugador.nombre
    jugador.tipo = solicitud->tipo;
    
    struct RespuestaServidor respuesta;
    respuesta.mtype = solicitud->mtype;

    switch (solicitud->codigo) {
    case CONEXION:
        atenderConexion(&jugador, &respuesta);
        break;
    case DESCONEXION:
        atenderDesconexion(&jugador, &respuesta);
        break;
    case MOVIMIENTO:
        atenderMovimiento(&jugador, &respuesta);
        break;
    case TESORO_CAPTURADO:
        atenderCapturaTesoro(&jugador, &respuesta);
        break;
    case RAIDER_CAPTURADO:
        atenderCapturaRaider(&jugador, &respuesta);
        break;
    default:
        break;
    }
    responderSolicitud(solicitud->clave_mailbox_respuestas, &respuesta);
    imprimirEstado(estado);
}

int conectarJugador(struct Jugador *jugador) {
    // if (buscarJugador(jugador->pid) != -1) return -1; // ya existe
    if (!aceptarJugador(jugador)) return -1; // verificar
    spawnearJugador(jugador); // darle una posicion
    jugadores[estado->cant_jugadores++] = *jugador; // incrementar cantidad
    (jugador->tipo == RAIDER) ? estado->cant_raiders++ : estado->cant_guardianes++;
    return 0;
}

int moverJugador(struct Jugador *jugador) {
    int pos = buscarJugador(jugador->pid);
    if (pos < 0) return -1;

    mapa[jugadores[pos].posicion.fila][jugadores[pos].posicion.columna] = VACIO;
    jugadores[pos].posicion = jugador->posicion;
    mapa[jugador->posicion.fila][jugador->posicion.columna] = jugador->tipo; 
    return 0;
}

int desconectarJugador(long pid) {
    int pos = buscarJugador(pid);
    if (pos < 0) return -1;

    mapa[jugadores[pos].posicion.fila]
        [jugadores[pos].posicion.columna] = VACIO;

    (jugadores[pos].tipo == RAIDER) ?
        estado->cant_raiders--:
        estado->cant_guardianes--;
    
    int j;
    for (j = pos; j < estado->cant_jugadores - 1; j++) {
        jugadores[j] = jugadores[j + 1];
    }
    estado->cant_jugadores--;
    return 0;
}

int capturarTesoro(struct Jugador *jugador) {
    if (estado->cant_tesoros == 0) return SIN_TESOROS;

    int pos = buscarJugador(jugador->pid);
    if (pos < 0) return -1;
    if (jugadores[pos].tipo == GUARDIAN) return -1;
    
    int fila = jugador->posicion.fila;
    int columna = jugador->posicion.columna;

    if (mapa[fila][columna] == TESORO) {
        mapa[fila][columna] = RAIDER;
        estado->cant_tesoros--;
        mapa[jugadores[pos].posicion.fila]
            [jugadores[pos].posicion.columna] = VACIO;
        jugadores[pos].posicion = jugador->posicion;
        return 0;
    }
    return -1;
}

int capturarRaider(struct Jugador *jugador) {
    if (estado->cant_raiders == 0) return SIN_RAIDERS;
    
    int pos = buscarJugador(jugador->pid);
    if (pos < 0) return -1;
    if (jugador->tipo == RAIDER) return -1;

    int fila = jugador->posicion.fila;      
    int columna = jugador->posicion.columna;

    int i, j;
    for (i = 0; i < estado->cant_jugadores; i++) {
        if (i == pos) continue; // no se captura
        if (jugadores[i].tipo == RAIDER &&
            jugadores[i].posicion.fila == fila &&
            jugadores[i].posicion.columna == columna) {
            mapa[fila][columna] = GUARDIAN;
            
            mapa[jugadores[pos].posicion.fila]
                [jugadores[pos].posicion.columna] = VACIO;
            jugadores[pos].posicion = jugador->posicion;
            for (j = i; j < estado->cant_jugadores - 1; j++) {
                jugadores[j] = jugadores[j + 1];
            }
            estado->cant_jugadores--;
            estado->cant_raiders--;
            return 0;
        }
    }
    return -1;
}

int buscarJugador(long pid) {
    int i;
    for (i = 0; i < estado->cant_jugadores; i++)
        if (jugadores[i].pid == pid) return i;
    return -1;
}

void construirRespuesta(struct RespuestaServidor *respuesta,
     int codigo, const char *mensaje) {
    snprintf(respuesta->mensaje, MAX_LONGITUD_MENSAJES, mensaje);
    respuesta->codigo = codigo;
}

void atenderConexion(struct Jugador *jugador, struct RespuestaServidor *respuesta) {
    imprimirTituloSolicitud(jugador->pid, "solicita conectarse...");
    if (conectarJugador(jugador) < 0) {
        construirRespuesta(respuesta, ERROR, "No se conectó el jugador");
    } else {
        construirRespuesta(respuesta, _OK, "Jugador conectado con éxito");
    }
}

void atenderDesconexion(struct Jugador *jugador, struct RespuestaServidor *respuesta) {
    imprimirTituloSolicitud(jugador->pid, "solicita desconectarse...");
    if (desconectarJugador(jugador->pid) <0) {
        construirRespuesta(respuesta, ERROR, "no encontro al jugador");
    } else {
        construirRespuesta (respuesta, _OK, "Jugador desconectado con exito");
    }
}

void atenderMovimiento(struct Jugador *jugador, struct RespuestaServidor *respuesta) {
    imprimirTituloSolicitud(jugador->pid, "se mueve..."); 
    if (moverJugador(jugador) < 0) { 
        construirRespuesta(respuesta,ERROR, "no encontro al jugador");
    } else {
        construirRespuesta (respuesta, _OK, "Jugador se movio con exito");
    } 
}


void atenderCapturaTesoro(struct Jugador *jugador, struct RespuestaServidor *respuesta) {
    imprimirTituloSolicitud(jugador->pid, "intenta capturar tesoro..."); 
    int codigo = capturarTesoro(jugador);
    if (codigo == 0) {
        construirRespuesta(respuesta, _OK,"Tesoro capturado con exito");
    } else if (codigo == SIN_TESOROS) {
        construirRespuesta(respuesta, SIN_TESOROS,"Ya no quedan tesoros en el mapa");
    } else {
        construirRespuesta(respuesta, ERROR, "No hay tesoro en esta posicion");
    }
}

void atenderCapturaRaider(struct Jugador *jugador, struct RespuestaServidor *respuesta) {
        imprimirTituloSolicitud(jugador->pid, "intenta capturar raider..."); 
    int codigo = capturarRaider(jugador);
    if (codigo == 0) {
        construirRespuesta(respuesta, _OK,"Raider capturado con exito");
    } else if (codigo == SIN_RAIDERS) {
        construirRespuesta(respuesta, SIN_TESOROS,"Ya no quedan raiders en el mapa");
    } else {
        construirRespuesta(respuesta, ERROR, "No hay raider en esta posicion");
    }
}