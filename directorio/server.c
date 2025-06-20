#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include "directorio.h"

// ==================== VARIABLES GLOBALES PARA LIMPIEZA ====================

// Variables globales para poder limpiar desde el manejador de señales
static int mailbox_solicitudes_global = -1;
static int mailbox_respuestas_global = -1;
static struct catacumba *catacumbas_global = NULL;
static int *num_catacumbas_global = NULL;

// ==================== PROTOTIPOS DE FUNCIONES ====================

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
void manejarSenalTerminacion(int sig);

/**
 * @brief Configura los manejadores de señales para terminación limpia
 *
 * Establece los manejadores para SIGINT (Ctrl+C) y SIGTERM para que
 * el servidor pueda terminar de forma ordenada, limpiando los recursos
 * del sistema operativo.
 **/
void configurarManejoSenales(void);

/**
 * @brief Elimina los mailboxes del sistema
 *
 * Utiliza msgctl con IPC_RMID para eliminar los mailboxes de solicitudes
 * y respuestas del sistema. Esto evita que queden recursos huérfanos.
 **/
void limpiarMailboxes(void);

/**
 * @brief Lista todas las catacumbas registradas en el directorio
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 * @param catacumbas Array de catacumbas registradas
 * @param num_catacumbas Puntero al número actual de catacumbas
 **/
void listarCatacumbas(struct respuesta *resp, struct catacumba catacumbas[], int *num_catacumbas);

/**
 * @brief Agrega una nueva catacumba al directorio
 * @param catacumbas Array donde se almacenan las catacumbas
 * @param num_catacumbas Puntero al número actual de catacumbas (se incrementa si se agrega)
 * @param msg Mensaje de solicitud con los datos de la catacumba (formato: "nombrecat|dircat|dirpropcat|dirmailbox")
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 **/
void agregarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp);

/**
 * @brief Busca una catacumba específica por su nombre
 * @param catacumbas Array de catacumbas donde buscar
 * @param num_catacumbas Puntero al número actual de catacumbas
 * @param msg Mensaje de solicitud con el nombre de la catacumba a buscar
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 **/
void buscarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp);

/**
 * @brief Elimina una catacumba del directorio
 * @param catacumbas Array de catacumbas donde buscar y eliminar
 * @param num_catacumbas Puntero al número actual de catacumbas (se decrementa si se elimina)
 * @param msg Mensaje de solicitud con el nombre de la catacumba a eliminar
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 **/
void eliminarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp);

/**
 * @brief Recibe solicitudes de los clientes desde el mailbox de solicitudes
 * @param recibido Puntero donde se almacena el número de bytes recibidos
 * @param mailbox_solicitudes_id ID del mailbox de solicitudes
 * @param msg Puntero donde se almacenará la solicitud recibida
 **/
void RecibirSolicitudes(int *recibido, int mailbox_solicitudes_id, struct solicitud *msg);

/**
 * @brief Envía una respuesta al cliente a través del mailbox de respuestas
 * @param mailbox_respuestas_id ID del mailbox de respuestas
 * @param resp Puntero a la estructura de respuesta a enviar
 **/
void enviarRespuesta(int mailbox_respuestas_id, struct respuesta *resp);

// ==================== PROTOTIPOS DE FUNCIONES DE PERSISTENCIA ====================

/**
 * @brief Carga las catacumbas desde el archivo de persistencia al iniciar el servidor
 * @param catacumbas Array donde se cargarán las catacumbas
 * @param num_catacumbas Puntero al contador de catacumbas (se actualiza)
 * @return 0 si se carga correctamente, -1 si hay error o no existe el archivo
 **/
int cargarCatacumbas(struct catacumba catacumbas[], int *num_catacumbas);

/**
 * @brief Guarda las catacumbas actuales en el archivo de persistencia
 * @param catacumbas Array de catacumbas a guardar
 * @param num_catacumbas Número de catacumbas en el array
 * @return 0 si se guarda correctamente, -1 si hay error
 **/
int guardarCatacumbas(struct catacumba catacumbas[], int num_catacumbas);

/**
 * @brief Verifica el estado del servidor
 *
 * Esta función comprueba si el servidor de directorio está activo y
 * funcionando correctamente. En este caso, simplemente retorna true.
 *
 * @return true si el servidor está activo, false en caso contrario
 **/
void estadoServidor(struct catacumba catacumbas[], int *num_catacumbas);

/**
 * @brief Función principal del servidor de directorio
 *
 * Inicializa los mailboxes de comunicación y mantiene un bucle infinito
 * para procesar solicitudes de los clientes. El servidor maneja:
 * - Listado de catacumbas
 * - Agregado de nuevas catacumbas
 * - Búsqueda de catacumbas por nombre
 * - Eliminación de catacumbas
 *
 * @param argc Número de argumentos de línea de comandos (no utilizado)
 * @param argv Array de argumentos de línea de comandos (no utilizado)
 * @return EXIT_SUCCESS si termina correctamente (nunca en este caso)
 **/
int main(int argc, char *argv[])
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
    // Configurar variables globales para el manejo de señales
    catacumbas_global = catacumbas;
    num_catacumbas_global = &num_catacumbas;

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
    mailbox_solicitudes_global = mailbox_solicitudes_id; // Guardar para limpieza

    // Crear o conectar al mailbox de respuestas
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, IPC_CREAT | 0666);
    if (mailbox_respuestas_id == -1)
    {
        perror("Error al crear el mailbox de respuestas");
        limpiarMailboxes(); // Limpiar el mailbox ya creado
        exit(EXIT_FAILURE);
    }
    mailbox_respuestas_global = mailbox_respuestas_id; // Guardar para limpieza

    printf("\n✓ Mailboxes creados/conectados correctamente.\n");
    printf("  ├─ Solicitudes ID: %d\n", mailbox_solicitudes_id);
    printf("  └─ Respuestas ID:  %d\n\n", mailbox_respuestas_id);

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR LISTO - ESPERANDO SOLICITUDES              \n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

    // ==================== BUCLE PRINCIPAL ====================
    time_t tiempo_anterior = time(NULL);

    while (1)
    {   
        if (difftime(time(NULL), tiempo_anterior) >= 1) {
            printf("------------------------------- REALIZANDO PING -------------------------------");
            estadoServidor(catacumbas, &num_catacumbas);
            tiempo_anterior = time(NULL);
        }

        // Recibir solicitud del cliente (función bloqueante)
        RecibirSolicitudes(&recibido, mailbox_solicitudes_id, &msg);

        // Preparar respuesta común para todas las operaciones
        resp.mtype = msg.mtype; // El PID del cliente que envió la solicitud
        resp.num_elementos = 0; // Inicializar contador de elementos
        resp.datos[0] = '\0';   // Limpiar buffer de datos

        // ==================== PROCESAMIENTO DE SOLICITUDES ====================
        // Procesar la solicitud según el tipo de operación solicitada
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

        // Enviar respuesta al cliente que hizo la solicitud
        enviarRespuesta(mailbox_respuestas_id, &resp);
    }

    exit(EXIT_SUCCESS);
}

/**
 * @brief Recibe y procesa solicitudes de los clientes
 *
 * Esta función es bloqueante y espera a que llegue un mensaje al mailbox
 * de solicitudes. Imprime información de depuración sobre el mensaje recibido.
 *
 * @param recibido Puntero donde se almacena el número de bytes recibidos
 * @param mailbox_solicitudes_id ID del mailbox de solicitudes
 * @param msg Puntero donde se almacenará la solicitud recibida
 **/
void RecibirSolicitudes(int *recibido, int mailbox_solicitudes_id, struct solicitud *msg)
{
    printf("⏳ Esperando nuevas solicitudes...\n");

    // Recibir solicitud (bloqueante)
    *recibido = msgrcv(mailbox_solicitudes_id, msg, sizeof(struct solicitud) - sizeof(long), 0, 0);
    if (*recibido == -1)
    {
        perror("❌ Error al recibir solicitud");
        return;
    }

    printf("───────────────────────────────────────────────────────────────\n");
    printf("📨 NUEVA SOLICITUD RECIBIDA\n");
    printf("  ├─ Cliente PID: %ld\n", msg->mtype);
    printf("  ├─ Tipo Op.:    %d\n", msg->tipo);
    printf("  └─ Datos:       \"%s\"\n", msg->texto);
    printf("───────────────────────────────────────────────────────────────\n\n");
}

/**
 * @brief Envía una respuesta al cliente a través del mailbox de respuestas
 *
 * @param mailbox_respuestas_id ID del mailbox de respuestas
 * @param resp Puntero a la estructura de respuesta a enviar
 **/
void enviarRespuesta(int mailbox_respuestas_id, struct respuesta *resp)
{
    if (msgsnd(mailbox_respuestas_id, resp, sizeof(struct respuesta) - sizeof(long), 0) == -1)
    {
        perror("❌ Error al enviar respuesta");
    }
    else
    {
        printf("📤 Respuesta enviada al cliente PID %ld\n", resp->mtype);
        printf("═══════════════════════════════════════════════════════════════\n\n");
    }
}

/**
 * @brief Lista todas las catacumbas registradas en el directorio
 *
 * Construye una cadena de texto con todas las catacumbas disponibles en formato
 * "nombre|direccion|propCatacumba|mailbox|cantJug|maxJug" separadas por ";" y la almacena
 * en la estructura de respuesta. Si no hay catacumbas, informa que el directorio está vacío.
 *
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 * @param catacumbas Array de catacumbas registradas
 * @param num_catacumbas Puntero al número actual de catacumbas
 **/
void listarCatacumbas(struct respuesta *resp, struct catacumba catacumbas[], int *num_catacumbas)
{
    printf("📋 LISTANDO CATACUMBAS\n");
    printf("   Total registradas: %d\n\n", *num_catacumbas);

    resp->codigo = RESP_OK;
    resp->num_elementos = *num_catacumbas;

    if (*num_catacumbas > 0)
    {
        resp->datos[0] = '\0'; // Limpiar el buffer de datos

        for (int i = 0; i < *num_catacumbas; i++)
        {
            char temp[MAX_TEXT]; // Buffer para una catacumba en formato | (usando MAX_TEXT de directorio.h)
            snprintf(temp, sizeof(temp), "%s|%s|%s|%s|%d|%d",
                     catacumbas[i].nombre,
                     catacumbas[i].direccion,
                     catacumbas[i].propCatacumba,
                     catacumbas[i].mailbox,
                     catacumbas[i].cantJug,
                     catacumbas[i].cantMaxJug);

            // Verificar que no se exceda el buffer de respuesta
            if (strlen(resp->datos) + strlen(temp) < MAX_DAT_RESP - 1)
            {
                strncat(resp->datos, temp, MAX_DAT_RESP - strlen(resp->datos) - 1);

                // Agregar separador solo si no es la última catacumba
                if (i < *num_catacumbas - 1)
                {
                    strncat(resp->datos, ";", MAX_DAT_RESP - strlen(resp->datos) - 1);
                }
            }

            printf("   %d. %-15s | %-20s | %-20s | %-10s | %d/%d jugadores\n",
                   i + 1, catacumbas[i].nombre, catacumbas[i].direccion,
                   catacumbas[i].propCatacumba, catacumbas[i].mailbox,
                   catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
        }
        printf("\n✅ Listado completado (%d catacumbas enviadas)\n\n", *num_catacumbas);
    }
    else
    {
        strcpy(resp->datos, "No hay catacumbas registradas.");
        printf("   ℹ️  No hay catacumbas registradas en el directorio.\n\n");
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
    for (int i = 0; i < *num_catacumbas; i++)
    {
        if (kill(catacumbas[i].pid, 0) == 0) {
            printf("✅  Petición enviada a la catacumba: %d, %s\n", catacumbas[i].pid, catacumbas[i].nombre);
        } else {
            if (errno == EPERM) {
               printf("⚠️  No se dispone de los permisos necesarios para enviar la solicitud a la catacumba: %d, %s \n", catacumbas[i].pid, catacumbas[i].nombre); 
            } else if (errno == ESRCH) {

                for (int j = i; j < *num_catacumbas - 1; j++)
                {
                    strcpy(catacumbas[j].nombre, catacumbas[j + 1].nombre);
                    strcpy(catacumbas[j].direccion, catacumbas[j + 1].direccion);
                    strcpy(catacumbas[j].propCatacumba, catacumbas[j + 1].propCatacumba);
                    strcpy(catacumbas[j].mailbox, catacumbas[j + 1].mailbox);
                    catacumbas[j].cantJug = catacumbas[j + 1].cantJug;
                    catacumbas[j].cantMaxJug = catacumbas[j + 1].cantMaxJug;
                }

                (*num_catacumbas)--;

                if (guardarCatacumbas(catacumbas, *num_catacumbas) != 0)
                {
                    printf("⚠️  Advertencia: No se pudo guardar la persistencia\n");
                }

                printf("⚠️  Catacumba eliminada: %d, %s\n", catacumbas[i].pid, catacumbas[i].nombre);

            } else {
                perror("❌ Error desconocido al verificar proceso");
            }
        }
    }
}

/**
 * @brief Agrega una nueva catacumba al directorio
 *
 * Procesa el mensaje del cliente que debe contener el nombre, dirección, propiedades y mailbox
 * de la catacumba en formato "nombrecat|dircat|dirpropcat|dirmailbox". Los campos de cantidad
 * de jugadores se inicializan automáticamente (cantJug=0, maxJug=0). Valida que
 * no se exceda el límite máximo de catacumbas y que el formato sea correcto.
 *
 * @param catacumbas Array donde se almacenan las catacumbas
 * @param num_catacumbas Puntero al número actual de catacumbas (se incrementa si se agrega)
 * @param msg Mensaje de solicitud con los datos de la catacumba (formato: "nombrecat|dircat|dirpropcat|dirmailbox")
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 **/
void agregarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp)
{
    printf("➕ AGREGANDO NUEVA CATACUMBA\n");

    // Verificar que no se haya alcanzado el límite máximo
    if (*num_catacumbas < MAX_CATACUMBAS)
    {
        // Hacer una copia del mensaje para usar con strtok
        char texto_copia[MAX_TEXT];
        strncpy(texto_copia, msg->texto, MAX_TEXT - 1);
        texto_copia[MAX_TEXT - 1] = '\0';

        // Parsear el mensaje en formato "nombrecat|dircat|dirpropcat|dirmailbox"
        char *nombre = strtok(texto_copia, "|");
        char *direccion = strtok(NULL, "|");
        char *propCatacumba = strtok(NULL, "|");
        char *mailbox = strtok(NULL, "|");

        if (nombre != NULL && direccion != NULL && propCatacumba != NULL && mailbox != NULL)
        {
            // Copiar los campos básicos de la catacumba
            catacumbas[*num_catacumbas].pid = msg->mtype;                      // Asignar el PID del proceso actual
            printf("   ├─ PID:        %d\n", catacumbas[*num_catacumbas].pid); // DEBUG

            strncpy(catacumbas[*num_catacumbas].nombre, nombre, MAX_NOM - 1);
            catacumbas[*num_catacumbas].nombre[MAX_NOM - 1] = '\0';

            strncpy(catacumbas[*num_catacumbas].direccion, direccion, MAX_RUTA - 1);
            catacumbas[*num_catacumbas].direccion[MAX_RUTA - 1] = '\0';

            strncpy(catacumbas[*num_catacumbas].propCatacumba, propCatacumba, MAX_RUTA - 1);
            catacumbas[*num_catacumbas].propCatacumba[MAX_RUTA - 1] = '\0';

            strncpy(catacumbas[*num_catacumbas].mailbox, mailbox, MAX_NOM - 1);
            catacumbas[*num_catacumbas].mailbox[MAX_NOM - 1] = '\0';

            // Inicializar automáticamente los campos de jugadores
            // Estos se actualizarán consultando la dirección de la catacumba
            catacumbas[*num_catacumbas].cantJug = 0;
            catacumbas[*num_catacumbas].cantMaxJug = 0;

            (*num_catacumbas)++; // Incrementar el contador

            printf("   ├─ Nombre:        \"%s\"\n", nombre);
            printf("   ├─ Dirección:     \"%s\"\n", direccion);
            printf("   ├─ Propiedades:   \"%s\"\n", propCatacumba);
            printf("   ├─ Mailbox:       \"%s\"\n", mailbox);
            printf("   └─ Estado:        Inicializada (0/0 jugadores)\n");
            printf("\n✅ Catacumba agregada correctamente (Total: %d/%d)\n\n", *num_catacumbas, MAX_CATACUMBAS);

            // Configurar respuesta exitosa
            resp->codigo = RESP_OK;
            strcpy(resp->datos, "Catacumba agregada correctamente.");

            // Persistir el estado actualizado
            if (guardarCatacumbas(catacumbas, *num_catacumbas) != 0)
            {
                printf("⚠️  Advertencia: No se pudo guardar la persistencia\n");
            }
        }
        else
        {
            // Error: formato incorrecto
            printf("   ❌ Error: formato incorrecto - faltan campos.\n");
            printf("      Formato esperado: 'nombrecat|dircat|dirpropcat|dirmailbox'\n\n");
            resp->codigo = RESP_ERROR;
            strcpy(resp->datos, "Error: formato incorrecto. Use 'nombrecat|dircat|dirpropcat|dirmailbox'");
        }
    }
    else
    {
        // Error: se alcanzó el límite máximo de catacumbas
        printf("   ❌ Error: máximo de catacumbas alcanzado (%d/%d)\n\n", *num_catacumbas, MAX_CATACUMBAS);
        resp->codigo = RESP_LIMITE_ALCANZADO;
        strcpy(resp->datos, "Error: máximo de catacumbas alcanzado.");
    }
}

/**
 * @brief Busca una catacumba específica por su nombre
 *
 * Recorre el array de catacumbas buscando una que coincida con el nombre
 * proporcionado en la solicitud. Si la encuentra, devuelve sus datos en formato
 * "nombre|direccion|propCatacumba|mailbox|cantJug|maxJug"; si no, informa que no fue encontrada.
 *
 * @param catacumbas Array de catacumbas donde buscar
 * @param num_catacumbas Puntero al número actual de catacumbas
 * @param msg Mensaje de solicitud con el nombre de la catacumba a buscar
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 **/
void buscarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp)
{
    printf("🔍 BUSCANDO CATACUMBA: \"%s\"\n", msg->texto);

    int encontrado = 0;
    for (int i = 0; i < *num_catacumbas; i++)
    {
        if (strcmp(catacumbas[i].nombre, msg->texto) == 0)
        {
            printf("   ├─ Nombre:        \"%s\"\n", catacumbas[i].nombre);
            printf("   ├─ Dirección:     \"%s\"\n", catacumbas[i].direccion);
            printf("   ├─ Propiedades:   \"%s\"\n", catacumbas[i].propCatacumba);
            printf("   ├─ Mailbox:       \"%s\"\n", catacumbas[i].mailbox);
            printf("   └─ Jugadores:     %d/%d\n", catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
            printf("\n✅ Catacumba encontrada y datos enviados\n\n");

            resp->codigo = RESP_OK;
            resp->num_elementos = 1;
            snprintf(resp->datos, MAX_DAT_RESP, "%s|%s|%s|%s|%d|%d",
                     catacumbas[i].nombre, catacumbas[i].direccion,
                     catacumbas[i].propCatacumba, catacumbas[i].mailbox,
                     catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
            encontrado = 1;
            break;
        }
    }

    if (!encontrado)
    {
        printf("   ❌ Catacumba no encontrada en el directorio.\n\n");
        resp->codigo = RESP_NO_ENCONTRADO;
        resp->num_elementos = 0;
        snprintf(resp->datos, MAX_DAT_RESP, "Catacumba '%.40s' no encontrada.", msg->texto);
    }
}

/**
 * @brief Elimina una catacumba del directorio por su nombre
 *
 * Busca una catacumba por su nombre y la elimina del array si la encuentra.
 * Para mantener la integridad del array, mueve todos los elementos posteriores
 * una posición hacia adelante y decrementa el contador de catacumbas.
 *
 * @param catacumbas Array de catacumbas donde buscar y eliminar
 * @param num_catacumbas Puntero al número actual de catacumbas (se decrementa si se elimina)
 * @param msg Mensaje de solicitud con el nombre de la catacumba a eliminar
 * @param resp Puntero a la estructura de respuesta donde se almacenará el resultado
 *
 * @note Si la catacumba no existe, se devuelve RESP_NO_ENCONTRADO
 * @note Si se elimina correctamente, se devuelve RESP_OK
 **/
void eliminarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp)
{
    printf("🗑️  ELIMINANDO CATACUMBA: \"%s\"\n", msg->texto);

    int encontrado = -1; // Índice de la catacumba encontrada (-1 si no se encuentra)

    // Buscar la catacumba en el array
    for (int i = 0; i < *num_catacumbas; i++)
    {
        if (strcmp(catacumbas[i].nombre, msg->texto) == 0)
        {
            encontrado = i; // Guardar el índice donde se encontró
            break;          // Salir del bucle una vez encontrada
        }
    }

    if (encontrado >= 0)
    {
        // Catacumba encontrada: proceder con la eliminación
        printf("   ├─ Catacumba localizada en posición %d\n", encontrado + 1);
        printf("   ├─ Reorganizando array de catacumbas...\n");

        // Mover todos los elementos posteriores una posición hacia adelante
        for (int i = encontrado; i < *num_catacumbas - 1; i++)
        {
            strcpy(catacumbas[i].nombre, catacumbas[i + 1].nombre);
            strcpy(catacumbas[i].direccion, catacumbas[i + 1].direccion);
            strcpy(catacumbas[i].propCatacumba, catacumbas[i + 1].propCatacumba);
            strcpy(catacumbas[i].mailbox, catacumbas[i + 1].mailbox);
            catacumbas[i].cantJug = catacumbas[i + 1].cantJug;
            catacumbas[i].cantMaxJug = catacumbas[i + 1].cantMaxJug;
        }
        (*num_catacumbas)--; // Decrementar el contador de catacumbas

        printf("   └─ Total actual: %d catacumbas\n", *num_catacumbas);
        printf("\n✅ Catacumba eliminada correctamente\n\n");

        // Configurar respuesta exitosa
        resp->codigo = RESP_OK;
        snprintf(resp->datos, MAX_DAT_RESP, "Catacumba '%.40s' eliminada correctamente.", msg->texto);

        // Persistir el estado actualizado
        if (guardarCatacumbas(catacumbas, *num_catacumbas) != 0)
        {
            printf("⚠️  Advertencia: No se pudo guardar la persistencia\n");
        }
    }
    else
    {
        // Catacumba no encontrada
        printf("   ❌ Catacumba no encontrada para eliminar.\n\n");
        resp->codigo = RESP_NO_ENCONTRADO;
        snprintf(resp->datos, MAX_DAT_RESP, "Catacumba '%.40s' no encontrada.", msg->texto);
    }
}

// ==================== IMPLEMENTACIONES DE FUNCIONES DE PERSISTENCIA ====================

/**
 * @brief Carga las catacumbas desde el archivo de persistencia al iniciar el servidor
 *
 * Lee el archivo binario donde se almacenan las catacumbas persistidas. Si el archivo
 * no existe o está vacío, la función retorna -1 y el servidor inicia con el directorio vacío.
 * Los datos se cargan en el array proporcionado y se actualiza el contador.
 *
 * @param catacumbas Array donde se cargarán las catacumbas desde el archivo
 * @param num_catacumbas Puntero al contador de catacumbas (se actualiza con el número cargado)
 * @return 0 si se carga correctamente, -1 si hay error o no existe el archivo
 **/
int cargarCatacumbas(struct catacumba catacumbas[], int *num_catacumbas)
{
    FILE *archivo = fopen(ARCHIVO_CATACUMBAS, "rb");
    if (archivo == NULL)
    {
        // El archivo no existe, es normal en la primera ejecución
        *num_catacumbas = 0;
        return -1;
    }

    // Leer el número de catacumbas del archivo
    size_t elementos_leidos = fread(num_catacumbas, sizeof(int), 1, archivo);
    if (elementos_leidos != 1)
    {
        printf("⚠️  Advertencia: No se pudo leer el contador del archivo de persistencia\n");
        fclose(archivo);
        *num_catacumbas = 0;
        return -1;
    }

    // Verificar que el número sea válido
    if (*num_catacumbas < 0 || *num_catacumbas > MAX_CATACUMBAS)
    {
        printf("⚠️  Advertencia: Número de catacumbas inválido en archivo (%d)\n", *num_catacumbas);
        fclose(archivo);
        *num_catacumbas = 0;
        return -1;
    }

    // Leer las catacumbas del archivo
    if (*num_catacumbas > 0)
    {
        elementos_leidos = fread(catacumbas, sizeof(struct catacumba), *num_catacumbas, archivo);
        if (elementos_leidos != (size_t)*num_catacumbas)
        {
            printf("⚠️  Advertencia: No se pudieron leer todas las catacumbas del archivo\n");
            printf("     Esperadas: %d, Leídas: %zu\n", *num_catacumbas, elementos_leidos);
            *num_catacumbas = (int)elementos_leidos; // Usar las que se pudieron leer
        }
    }

    fclose(archivo);

    // Mostrar resumen de carga
    if (*num_catacumbas > 0)
    {
        printf("   📋 Catacumbas cargadas desde archivo:\n");
        for (int i = 0; i < *num_catacumbas; i++)
        {
            printf("     %d. %-15s | %-20s | %-20s | %-10s\n",
                   i + 1, catacumbas[i].nombre, catacumbas[i].direccion,
                   catacumbas[i].propCatacumba, catacumbas[i].mailbox);
        }
    }

    return 0;
}

/**
 * @brief Guarda las catacumbas actuales en el archivo de persistencia
 *
 * Escribe todas las catacumbas del array en un archivo binario para persistir el estado
 * del directorio. Esto permite que el servidor mantenga la información entre reinicios.
 *
 * @param catacumbas Array de catacumbas a guardar en el archivo
 * @param num_catacumbas Número de catacumbas en el array
 * @return 0 si se guarda correctamente, -1 si hay error
 **/
int guardarCatacumbas(struct catacumba catacumbas[], int num_catacumbas)
{
    FILE *archivo = fopen(ARCHIVO_CATACUMBAS, "wb");
    if (archivo == NULL)
    {
        perror("Error al abrir archivo de persistencia para escritura");
        return -1;
    }

    // Escribir el número de catacumbas primero
    size_t elementos_escritos = fwrite(&num_catacumbas, sizeof(int), 1, archivo);
    if (elementos_escritos != 1)
    {
        printf("❌ Error al escribir contador de catacumbas\n");
        fclose(archivo);
        return -1;
    }

    // Escribir las catacumbas si hay alguna
    if (num_catacumbas > 0)
    {
        elementos_escritos = fwrite(catacumbas, sizeof(struct catacumba), num_catacumbas, archivo);
        if (elementos_escritos != (size_t)num_catacumbas)
        {
            printf("❌ Error al escribir catacumbas al archivo\n");
            printf("   Esperadas: %d, Escritas: %zu\n", num_catacumbas, elementos_escritos);
            fclose(archivo);
            return -1;
        }
    }

    fclose(archivo);
    printf("💾 Persistencia actualizada: %d catacumbas guardadas\n", num_catacumbas);
    return 0;
}

// ==================== IMPLEMENTACIONES DE FUNCIONES DE MANEJO DE SEÑALES ====================

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
