#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include "../directorio.h"
#include "comunicacion.h"
#include "operaciones.h"
#include "persistencia.h"
#include "senales.h"
#include "ping.h"

/**
 * @brief Función principal del servidor de directorio
 *
 * Inicializa los mailboxes de comunicación y mantiene un bucle infinito
 * para procesar solicitudes de los clientes. El servidor maneja:
 * - Listado de catacumbas
 * - Agregado de nuevas catacumbas
 * - Búsqueda de catacumbas por nombre
 * - Eliminación de catacumbas
 **/
int main()
{
    // ==================== VARIABLES LOCALES ====================
    struct catacumba catacumbas[MAX_CATACUMBAS]; // Array para almacenar las catacumbas
    int num_catacumbas = 0;                      // Contador de catacumbas registradas

    int mailbox_solicitudes_id, mailbox_respuestas_id; // IDs de los mailboxes
    struct solicitud msg;                              // Estructura para solicitudes recibidas
    struct respuesta resp;                             // Estructura para respuestas a enviar
    int recibido;                                      // Bytes recibidos en msgrcv

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("              DIRECTORIO DE CATACUMBAS - INICIANDO              \n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    // ==================== CONFIGURACIÓN DE SEÑALES ====================
    // Configurar manejadores de señales antes de crear los mailboxes
    configurarManejoSenales();

    printf("✓ Manejadores de señales configurados correctamente\n");
    printf("  ├─ SIGINT (Ctrl+C) capturado para terminación limpia\n");
    printf("  └─ SIGTERM capturado para terminación limpia\n\n");

    // ==================== CARGA DE CATACUMBAS PERSISTIDAS ====================
    printf("📂 Cargando catacumbas desde archivo persistente...\n");
    if (cargarCatacumbas(catacumbas, &num_catacumbas) == 0)
    {
        printf("✅ Se cargaron %d catacumbas desde el archivo de persistencia\n", num_catacumbas);
        estadoServidor(catacumbas, &num_catacumbas);
    }
    else
    {
        printf("ℹ️  No se encontró archivo de persistencia o estaba vacío\n");
        printf("   Iniciando con directorio vacío\n");
    }

    // ==================== CREACIÓN DE MAILBOXES ====================
    // Crear o conectar al mailbox de solicitudes
    mailbox_solicitudes_id = msgget(MAILBOX_KEY, IPC_CREAT | 0666);
    if (mailbox_solicitudes_id == -1)
    {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }
    establecer_mailbox_solicitudes(mailbox_solicitudes_id); // Guardar para limpieza

    // Crear o conectar al mailbox de respuestas
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, IPC_CREAT | 0666);
    if (mailbox_respuestas_id == -1)
    {
        perror("Error al crear el mailbox de respuestas");
        limpiarMailboxes(); // Limpiar el mailbox ya creado
        exit(EXIT_FAILURE);
    }
    establecer_mailbox_respuestas(mailbox_respuestas_id); // Guardar para limpieza

    printf("\n✓ Mailboxes creados/conectados correctamente.\n");
    printf("  ├─ Solicitudes ID: %d\n", mailbox_solicitudes_id);
    printf("  └─ Respuestas ID:  %d\n\n", mailbox_respuestas_id);

    // Configurar variables globales para el manejo de señales
    establecer_catacumbas_globales(catacumbas, &num_catacumbas);

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR LISTO - ESPERANDO SOLICITUDES              \n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    // ==================== INICIALIZACIÓN DEL HILO DE PING ====================
    printf("🔄 Iniciando hilo de ping...\n");

    // Preparar parámetros para el hilo de ping
    struct ping_params params_ping = {catacumbas, &num_catacumbas};

    if (pthread_create(&hilo_ping, NULL, hiloPing, &params_ping) != 0)
    {
        perror("Error al crear hilo de ping");
        limpiarMailboxes();
        exit(EXIT_FAILURE);
    }
    printf("✓ Hilo de ping iniciado correctamente\n\n");

    // ==================== BUCLE PRINCIPAL ====================
    while (1)
    {
        // Recibir solicitud del cliente (función bloqueante)
        RecibirSolicitudes(&recibido, mailbox_solicitudes_id, &msg);

        // Preparar respuesta común para todas las operaciones
        resp.mtype = msg.mtype; // El PID del cliente que envió la solicitud
        resp.num_elementos = 0; // Inicializar contador de elementos
        resp.datos[0] = '\0';   // Limpiar buffer de datos

        // ==================== PROCESAMIENTO DE SOLICITUDES ====================
        // Procesar la solicitud según el tipo de operación solicitada (con mutex para proteger datos compartidos)
        pthread_mutex_lock(&mutex_catacumbas);
        switch (msg.tipo)
        {
        case OP_LISTAR:
            // Listar todas las catacumbas registradas
            listarCatacumbas(&resp, catacumbas, &num_catacumbas);
            break;
        case OP_AGREGAR:
            // Agregar una nueva catacumba al directorio
            agregarCatacumba(catacumbas, &num_catacumbas, &msg, &resp);
            break;
        case OP_BUSCAR:
            // Buscar una catacumba específica por nombre
            buscarCatacumba(catacumbas, &num_catacumbas, &msg, &resp);
            break;
        case OP_ELIMINAR:
            // Eliminar una catacumba del directorio
            eliminarCatacumba(catacumbas, &num_catacumbas, &msg, &resp);
            break;
        default:
            // Operación no reconocida
            printf("⚠️  OPERACIÓN DESCONOCIDA: %d\n\n", msg.tipo);
            resp.codigo = RESP_ERROR;
            strcpy(resp.datos, "Operación desconocida.");
            break;
        }
        pthread_mutex_unlock(&mutex_catacumbas);

        // Enviar respuesta al cliente que hizo la solicitud
        enviarRespuesta(mailbox_respuestas_id, &resp);
    }

    exit(EXIT_SUCCESS);
}
