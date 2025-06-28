#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include "senales.h"
#include "ping.h"
#include "persistencia.h"

// Variables globales para poder limpiar desde el manejador de señales
static int mailbox_solicitudes_global = -1;
static int mailbox_respuestas_global = -1;
static struct catacumba *catacumbas_global = NULL;
static int *num_catacumbas_global = NULL;

// Funciones para establecer las variables globales (llamadas desde main)
void establecer_mailbox_solicitudes(int id) { mailbox_solicitudes_global = id; }
void establecer_mailbox_respuestas(int id) { mailbox_respuestas_global = id; }
void establecer_catacumbas_globales(struct catacumba *catacumbas, int *num_catacumbas) {
    catacumbas_global = catacumbas;
    num_catacumbas_global = num_catacumbas;
}

/**
 * @brief Configura los manejadores de señales para terminación limpia
 *
 * Establece los manejadores para SIGINT (Ctrl+C) y SIGTERM para que
 * el servidor pueda terminar de forma ordenada, limpiando los recursos
 * del sistema operativo.
 **/
void configurarManejoSenales(void)
{
    // Registrar el manejador para SIGINT (Ctrl+C)
    if (signal(SIGINT, manejarSenalTerminacion) == SIG_ERR)
    {
        perror("Error al configurar manejador SIGINT");
        exit(EXIT_FAILURE);
    }

    // Registrar el manejador para SIGTERM
    if (signal(SIGTERM, manejarSenalTerminacion) == SIG_ERR)
    {
        perror("Error al configurar manejador SIGTERM");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Maneja la señal de terminación (SIGINT/SIGTERM)
 *
 * Esta función se ejecuta cuando el usuario presiona Ctrl+C o cuando
 * se envía una señal de terminación al proceso. Se encarga de:
 * - Guardar el estado actual de las catacumbas
 * - Eliminar los mailboxes del sistema
 * - Terminar el proceso de forma ordenada
 *
 * @param sig Número de la señal recibida
 **/
void manejarSenalTerminacion(int sig)
{
    printf("\n\n═══════════════════════════════════════════════════════════════\n");
    printf("           SEÑAL DE TERMINACIÓN RECIBIDA (Señal %d)            \n", sig);
    printf("═══════════════════════════════════════════════════════════════\n");

    if (sig == SIGINT)
    {
        printf("🛑 Usuario presionó Ctrl+C - Iniciando terminación limpia...\n");
    }
    else if (sig == SIGTERM)
    {
        printf("🛑 Señal SIGTERM recibida - Iniciando terminación limpia...\n");
    }

    // Guardar estado actual antes de terminar
    if (catacumbas_global != NULL && num_catacumbas_global != NULL)
    {
        printf("💾 Guardando estado actual de las catacumbas...\n");
        if (guardarCatacumbas(catacumbas_global, *num_catacumbas_global) == 0)
        {
            printf("✅ Estado guardado correctamente (%d catacumbas)\n", *num_catacumbas_global);
        }
        else
        {
            printf("⚠️  Advertencia: No se pudo guardar el estado actual\n");
        }
    }

    // Terminar el hilo de ping
    printf("🔄 Terminando hilo de ping...\n");
    servidor_activo = false;
    pthread_join(hilo_ping, NULL);
    printf("✅ Hilo de ping terminado correctamente\n");

    // Limpiar mailboxes del sistema
    printf("🧹 Limpiando mailboxes del sistema...\n");
    limpiarMailboxes();

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("          SERVIDOR TERMINADO CORRECTAMENTE - ADIÓS             \n");
    printf("═══════════════════════════════════════════════════════════════\n");

    exit(EXIT_SUCCESS);
}

/**
 * @brief Elimina los mailboxes del sistema
 *
 * Utiliza msgctl con IPC_RMID para eliminar los mailboxes de solicitudes
 * y respuestas del sistema. Esto evita que queden recursos huérfanos.
 **/
void limpiarMailboxes(void)
{
    int errores = 0;

    // Eliminar mailbox de solicitudes
    if (mailbox_solicitudes_global != -1)
    {
        if (msgctl(mailbox_solicitudes_global, IPC_RMID, NULL) == -1)
        {
            perror("  ❌ Error al eliminar mailbox de solicitudes");
            errores++;
        }
        else
        {
            printf("  ✅ Mailbox de solicitudes eliminado (ID: %d)\n", mailbox_solicitudes_global);
        }
    }

    // Eliminar mailbox de respuestas
    if (mailbox_respuestas_global != -1)
    {
        if (msgctl(mailbox_respuestas_global, IPC_RMID, NULL) == -1)
        {
            perror("  ❌ Error al eliminar mailbox de respuestas");
            errores++;
        }
        else
        {
            printf("  ✅ Mailbox de respuestas eliminado (ID: %d)\n", mailbox_respuestas_global);
        }
    }

    if (errores == 0)
    {
        printf("  🎉 Todos los mailboxes eliminados correctamente\n");
    }
    else
    {
        printf("  ⚠️  Se encontraron %d errores durante la limpieza\n", errores);
    }
}
