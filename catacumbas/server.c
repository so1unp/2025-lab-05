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

void finish();
void setup(char rutaMapa[], char rutaConfig[]);
void usage(char *argv[]);

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

    // RECIBIR SOLICITUDES DE CLIENTES
    while (1) {
        printf("Esperando solicitudes...\n");
        struct SolicitudServidor solicitud;
        if (recibirSolicitudes(&solicitud, comunicacion->mailbox_solicitudes_id))
            atenderSolicitud(&solicitud, arena);
    }
    finish();
}


/**
 * @brief Libera todos los recursos IPC (memoria compartida, buzones) y memoria
 * dinámica.
 *
 * Realiza una limpieza ordenada: desvincula la memoria compartida, desmapea
 * los segmentos, cierra los descriptores de archivo, elimina el buzón de
 * mensajes y libera la memoria de la estructura del servidor.
 *
 */
void finish()
{
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
    printf("Programa terminado\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Orquesta la secuencia de inicialización completa del servidor.
 *
 * Llama en orden a todas las funciones `inicializar*` y `registrarEnDirectorio`
 * para poner en marcha el servidor. También imprime un banner de inicio.
 *
 * @param rutaMapa Ruta al archivo del mapa.
 * @param rutaConfig Ruta al archivo de configuración.
 */
void setup(char rutaMapa[], char rutaConfig[]) {
    // muy necesario
    arena = malloc(sizeof(struct Arena));
    if (arena == NULL) fatal("Fallo malloc de arena");

    comunicacion = malloc(sizeof(struct Comunicacion));
    if (comunicacion == NULL) fatal("Fallo malloc de comunicacion");
   
    // El orden es importante aqui
    // primero definir los sizes de la arena
    arena->size_mapa = sizeof(char) * FILAS * COLUMNAS;
    arena->size_estado = sizeof(struct Estado);

    // Preparar la señal para detener el proceso y eliminar las cosas
    // que haya creado
    signal(SIGINT, finish);

    // Inicializaciones
    snprintf(comunicacion->memoria_mapa_nombre, sizeof(comunicacion->memoria_mapa_nombre), "%s%d", MEMORIA_MAPA_PREFIJO, getpid());
    snprintf(comunicacion->memoria_estado_nombre, sizeof(comunicacion->memoria_mapa_nombre), "%s%d", MEMORIA_ESTADO_PREFIJO, getpid());
    comunicacion->mailbox_solicitudes_clave = getpid() * MAILBOX_SOLICITUDES_SUFIJO;    
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
    printf("  - Cantidad máxima de raiders: %i\n", arena->max_raiders);
    printf("  - Cantidad máxima de tesoros: %i\n", arena->max_tesoros);
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    // Comunicación    
    abrirMensajeria(comunicacion);
    
    // Generar el estado
    memset(arena->estado, 0, sizeof(struct Estado));
    arena->estado->max_jugadores = arena->max_guardianes + arena->max_raiders;
    arena->estado->cant_guardianes = 0;
    arena->estado->cant_raiders = 0;
    arena->estado->cant_jugadores = 0;
    arena->estado->cant_tesoros = 0;
    generarTesoros(arena);
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
        comunicacion->memoria_mapa_nombre,
        comunicacion->memoria_estado_nombre,
        comunicacion->mailbox_solicitudes_clave
    );
    struct respuesta respuesta_directorio;
    enviarSolicitudDirectorio(comunicacion, &solicitud_directorio, &respuesta_directorio);

}

void usage(char *argv[]) {
    printf("Argumentos inválidos, por favor utilicelo de la siguiente forma:\n");
    printf("\n");
    printf("%s ruta/a/mapa.txt  ruta/a/config.properties\n",argv[0]);
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

}