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
#include "config.h"
#define _GNU_SOURCE


/**
 * @brief Libera todos los recursos IPC (memoria compartida, buzones) y memoria
 * dinámica.
 *
 * Realiza una limpieza ordenada: desvincula la memoria compartida, desmapea
 * los segmentos, cierra los descriptores de archivo, elimina el buzón de
 * mensajes y libera la memoria de la estructura del servidor.
 *
 */
void finish();

/**
 * @brief Orquesta la secuencia de inicialización completa del servidor.
 *
 * Llama en orden a todas las funciones `inicializar*` y `registrarEnDirectorio`
 * para poner en marcha el servidor. También imprime un banner de inicio.
 *
 * @param rutaMapa Ruta al archivo del mapa.
 * @param rutaConfig Ruta al archivo de configuración.
 */
void setup(char rutaMapa[], char rutaConfig[]);

/**
 * @brief Reiniciar los valores de estado, jugadores y tesoros,
 * y vacia la casilla de mensajes de la anterior partida.
 * 
 * @param rutaMapa Ruta al archivo del mapa.
 */
void reiniciarPartida(char *ruta);

// =========================
//      VARIABLES GLOBALES
// =========================

struct Arena *arena;
struct Comunicacion *comunicacion;


// =========================
//      MAIN
// =========================

/**
 * @brief Punto de entrada principal y bucle de atención de solicitudes.
 *
 * Valida los argumentos de línea de comandos, inicializa el servidor
 * y entra en un bucle infinito para recibir y procesar solicitudes de clientes
 * de forma bloqueante.
 *
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Vector de argumentos de línea de comandos.
 * @return 0 en caso de éxito, 1 en caso de error.
 */
int main(int argc, char *argv[]) {
    // USO
    if (argc < 3){
        usage(argv);
        exit(EXIT_FAILURE);
    }

    // CONFIGURACION
    setup(argv[1], argv[2]);

    // PARTIDAS
    while (1) {

        // RECIBIR SOLICITUDES DE CLIENTES
        while ((arena->estado->cant_raiders >= 0)  && arena->estado->cant_tesoros > 0) {
            printf("Esperando solicitudes...\n");
            struct SolicitudServidor solicitud;
            if (recibirSolicitudes(&solicitud, comunicacion->mailbox_solicitudes_id))
                atenderSolicitud(&solicitud, arena);
        }

        notificarFinalJuego(arena);
        sleep(1); // cliente necesita un momento antes de desconectarlo

        reiniciarPartida(argv[1]);
    }

    return EXIT_SUCCESS;
}

void finish() {
    notificarFinalJuego(arena);

    if (shm_unlink(comunicacion->memoria_mapa_nombre) < 0)
        fatal("Error al borrar memoria mapa");
    if (shm_unlink(comunicacion->memoria_estado_nombre) < 0)
        fatal("Error al borrar memoria estado");
    munmap(arena->mapa, arena->size_mapa);
    munmap(arena->estado, arena->size_estado);
    close(comunicacion->memoria_mapa_fd);
    close(comunicacion->memoria_estado_fd);
    if (msgctl(comunicacion->mailbox_solicitudes_id, IPC_RMID, NULL) == -1)
    {
        perror("🚫 Error al eliminar el buzón de solicitudes, por favor eliminelo manualmente con ipcs -q y luego ipcrm -q <id>");
        exit(EXIT_FAILURE);
    }
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR DE CATACUMBA TERMINADO 🗿🚬                 \n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    exit(EXIT_SUCCESS);
}

void setup(char rutaMapa[], char rutaConfig[]) {
    // El orden es importante aqui

    // muy necesario
    arena = malloc(sizeof(struct Arena));
    if (arena == NULL) fatal("Fallo malloc de arena");

    comunicacion = malloc(sizeof(struct Comunicacion));
    if (comunicacion == NULL) fatal("Fallo malloc de comunicacion");
   
    arena->size_mapa = sizeof(char) * FILAS * COLUMNAS;
    arena->size_estado = sizeof(struct Estado);

    // Preparar la señal para detener el proceso y eliminar las cosas
    // que haya creado
    signal(SIGINT, finish);

    // Inicializaciones    
    inicializarComunicacion(comunicacion);

    abrirMemoria(arena, comunicacion);
    cargarArchivoConfiguracion(arena, rutaConfig);
    cargarArchivoMapa(arena, rutaMapa);

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR DE CATACUMBA INICIADO 🗿🚬                 \n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    printf("- Mapa: %s\n", comunicacion->memoria_mapa_nombre);
    printf("- Estado: %s\n", comunicacion->memoria_estado_nombre);
    printf("- Mailbox: %i\n", comunicacion->mailbox_solicitudes_clave);
    printf("- El mapa se cargó del archivo %s\n", rutaMapa);
    printf("- Las configuraciones se cargaron del archivo: %s\n", rutaConfig);
    printf("  - Cantidad máxima de guardianes: %i\n", arena->max_guardianes);
    printf("  - Cantidad máxima de exploradores: %i\n", arena->max_exploradores);
    printf("  - Cantidad máxima de tesoros: %i\n", arena->max_tesoros);
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    // Comunicación    
    abrirMensajeria(comunicacion);
    
    // Generar el estado
    memset(arena->estado, 0, sizeof(struct Estado));
    inicializarEstado(arena);

    generarTesoros(arena);

    // Iniciar comunicación con directorio
    int respuesta = registrarServidor(comunicacion);

    if (respuesta == RESP_LIMITE_ALCANZADO || respuesta == ERROR) finish();
}

void reiniciarPartida(char *ruta){

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR DE CATACUMBA REINICIANDO...                \n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Descartando mensajes de la partida anterior...\n\n");
    struct SolicitudServidor dummy;
    while (msgrcv(comunicacion->mailbox_solicitudes_id, &dummy, sizeof(dummy) - sizeof(long), 0, IPC_NOWAIT) != -1) {
        printf("🗑️  Mensaje descartado\n");
    }
    cargarArchivoMapa(arena, ruta);
    memset(arena->jugadores, 0, sizeof(arena->jugadores));
    memset(arena->tesoros, 0, sizeof(arena->tesoros));
    inicializarEstado(arena);
    generarTesoros(arena);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR DE CATACUMBA REINICIADO 🍤                 \n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Nueva partida iniciada\n");
    imprimirEstado(arena->estado);

}