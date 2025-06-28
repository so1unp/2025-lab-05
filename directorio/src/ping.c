#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ping.h"
#include "persistencia.h"
#include "../../catacumbas/catacumbas.h"

// Variables globales para el hilo de ping (definidas aquí)
pthread_t hilo_ping;
volatile bool servidor_activo = true;
pthread_mutex_t mutex_catacumbas = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Verifica si un proceso está activo independientemente del usuario
 *
 * Esta función verifica la existencia de un proceso consultando el sistema de
 * archivos /proc, lo que permite verificar procesos de cualquier usuario.
 *
 * @param pid PID del proceso a verificar
 * @return 1 si el proceso está activo, 0 si no existe
 **/
int procesoActivo(int pid)
{
    char proc_path[64];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d", pid);
    
    // Verificar si existe el directorio /proc/PID
    if (access(proc_path, F_OK) == 0)
    {
        return 1; // El proceso existe
    }
    else
    {
        return 0; // El proceso no existe
    }
}

/**
 * @brief Obtiene el estado de todas las catacumbas de la lista
 *
 *
 * @param catacumbas Array donde se almacenan las catacumbas
 * @param num_catacumbas Puntero al número actual de catacumbas (se incrementa si se agrega)
 **/
void estadoServidor(struct catacumba catacumbas[], int *num_catacumbas)
{
    time_t tiempo_actual = time(NULL);
    struct tm *tiempo_local = localtime(&tiempo_actual);

    printf("🔍 VERIFICACIÓN DE ESTADO - %02d:%02d:%02d\n",
           tiempo_local->tm_hour, tiempo_local->tm_min, tiempo_local->tm_sec);

    if (*num_catacumbas == 0)
    {
        printf("   ℹ️  No hay catacumbas registradas para verificar\n");
        printf("────────────────────────────────────────────────────────────────\n");
        return;
    }

    printf("   📊 Verificando %d catacumba%s registrada%s:\n",
           *num_catacumbas,
           (*num_catacumbas == 1) ? "" : "s",
           (*num_catacumbas == 1) ? "" : "s");

    int activas = 0, eliminadas = 0;

    for (int i = 0; i < *num_catacumbas; i++)
    {
        printf("   ├─ [%d/%d] \"%s\" (PID: %d) → ",
               i + 1, *num_catacumbas, catacumbas[i].nombre, catacumbas[i].pid);

        if (procesoActivo(catacumbas[i].pid))
        {
            printf("🟢 ACTIVA");

            // Leer estado actualizado de la catacumba
            if (leerEstadoCatacumba(&catacumbas[i]) == 0)
            {
                printf(" | %d/%d jugadores", catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
            }
            else
            {
                printf(" | Estado no disponible");
            }
            printf("\n");

            activas++;
        }
        else
        {
            printf("🔴 INACTIVA - Eliminando del directorio\n");

            // Mover elementos hacia adelante para eliminar la catacumba inactiva
            for (int j = i; j < *num_catacumbas - 1; j++)
            {
                strcpy(catacumbas[j].nombre, catacumbas[j + 1].nombre);
                strcpy(catacumbas[j].direccion, catacumbas[j + 1].direccion);
                strcpy(catacumbas[j].propCatacumba, catacumbas[j + 1].propCatacumba);
                strcpy(catacumbas[j].mailbox, catacumbas[j + 1].mailbox);
                catacumbas[j].cantJug = catacumbas[j + 1].cantJug;
                catacumbas[j].cantMaxJug = catacumbas[j + 1].cantMaxJug;
                catacumbas[j].pid = catacumbas[j + 1].pid;
            }

            (*num_catacumbas)--;
            eliminadas++;
            i--; // Revisar el mismo índice otra vez debido al reordenamiento

            if (guardarCatacumbas(catacumbas, *num_catacumbas) != 0)
            {
                printf("      ⚠️  Error al guardar cambios en persistencia\n");
            }
        }
    }

    // Resumen del ping
    printf("   └─ Resumen: ");
    if (activas > 0)
        printf("🟢 %d activa%s ", activas, (activas == 1) ? "" : "s");
    if (eliminadas > 0)
        printf("🔴 %d eliminada%s ", eliminadas, (eliminadas == 1) ? "" : "s");
    printf("(Total: %d)\n", *num_catacumbas);

    // Guardar estado actualizado si hay catacumbas activas (para persistir cambios de jugadores)
    if (activas > 0)
    {
        if (guardarCatacumbas(catacumbas, *num_catacumbas) != 0)
        {
            printf("      ⚠️  Error al actualizar persistencia con nuevos estados\n");
        }
    }

    printf("────────────────────────────────────────────────────────────────\n");
}

/**
 * @brief Función del hilo que ejecuta ping periódico
 *
 * Esta función se ejecuta en un hilo separado y realiza ping a las catacumbas
 * cada segundo para verificar su estado. Utiliza mutex para acceso seguro
 * a los datos compartidos.
 *
 * @param arg Puntero a los argumentos (no utilizado)
 * @return NULL
 **/
void *hiloPing(void *arg)
{
    // Obtener punteros a las catacumbas desde los argumentos
    struct ping_params *params = (struct ping_params *)arg;

    printf("🔄 Hilo de ping iniciado - ejecutando cada %d segundo%s\n",
           FRECUENCIA_PING, (FRECUENCIA_PING == 1) ? "" : "s");

    while (servidor_activo)
    {
        sleep(FRECUENCIA_PING); // Usar la constante definida

        if (!servidor_activo)
            break; // Verificar si el servidor sigue activo

        // Acceso seguro a las catacumbas con mutex
        pthread_mutex_lock(&mutex_catacumbas);

        if (params->catacumbas != NULL && params->num_catacumbas != NULL)
        {
            estadoServidor(params->catacumbas, params->num_catacumbas);
        }

        pthread_mutex_unlock(&mutex_catacumbas);
    }

    return NULL;
}

/**
 * @brief Lee el estado de una catacumba desde su memoria compartida
 *
 * Accede al archivo de memoria compartida de propiedades de la catacumba
 * para obtener información actualizada sobre jugadores y límites.
 *
 * @param catacumba Puntero a la estructura catacumba a actualizar
 * @return 0 si se lee correctamente, -1 si hay error
 **/
int leerEstadoCatacumba(struct catacumba *catacumba)
{
    int fd = -1;
    struct Estado *estado_ptr = NULL;

    // Abrir el archivo de memoria compartida de propiedades
    fd = shm_open(catacumba->propCatacumba, O_RDONLY, 0666);
    if (fd == -1)
    {
        printf("      ⚠️  Error al abrir SHM de propiedades '%s': %s\n",
               catacumba->propCatacumba, strerror(errno));
        catacumba->cantJug = 0;
        catacumba->cantMaxJug = 0;
        return -1;
    }

    // Mapear la memoria compartida
    estado_ptr = (struct Estado *)mmap(NULL, sizeof(struct Estado),
                                       PROT_READ, MAP_SHARED, fd, 0);
    if (estado_ptr == MAP_FAILED)
    {
        printf("      ⚠️  Error al mapear SHM de propiedades '%s': %s\n",
               catacumba->propCatacumba, strerror(errno));
        close(fd);
        catacumba->cantJug = 0;
        catacumba->cantMaxJug = 0;
        return -1;
    }

    // Leer los valores del estado
    catacumba->cantJug = estado_ptr->cant_jugadores;
    catacumba->cantMaxJug = estado_ptr->max_jugadores;

    // Limpiar recursos
    if (munmap(estado_ptr, sizeof(struct Estado)) == -1)
    {
        printf("      ⚠️  Error al desmapear SHM de propiedades '%s': %s\n",
               catacumba->propCatacumba, strerror(errno));
    }

    close(fd);
    return 0;
}
