#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "directorio.h"

// ==================== PROTOTIPOS DE FUNCIONES ====================

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
 * @param msg Mensaje de solicitud con los datos de la catacumba (formato: "nombre|direccion|mailbox")
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

    // ==================== CREACIÓN DE MAILBOXES ====================
    // Crear o conectar al mailbox de solicitudes
    mailbox_solicitudes_id = msgget(MAILBOX_KEY, 0666 | IPC_CREAT);
    if (mailbox_solicitudes_id == -1)
    {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }

    // Crear o conectar al mailbox de respuestas
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0666 | IPC_CREAT);
    if (mailbox_respuestas_id == -1)
    {
        perror("Error al crear el mailbox de respuestas");
        exit(EXIT_FAILURE);
    }

    printf("\n✓ Mailboxes creados/conectados correctamente.\n");
    printf("  ├─ Solicitudes ID: %d\n", mailbox_solicitudes_id);
    printf("  └─ Respuestas ID:  %d\n\n", mailbox_respuestas_id);

    printf("═══════════════════════════════════════════════════════════════\n");
    printf("           SERVIDOR LISTO - ESPERANDO SOLICITUDES              \n");
    printf("═══════════════════════════════════════════════════════════════\n\n");

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

    // Nunca llega aquí, pero buena práctica
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
 * "nombre|direccion|mailbox|cantJug|maxJug" separadas por ";" y la almacena
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
            char temp[300]; // Buffer para una catacumba en formato |
            snprintf(temp, sizeof(temp), "%s|%s|%s|%d|%d",
                     catacumbas[i].nombre,
                     catacumbas[i].direccion,
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

            printf("   %d. %-15s | %-20s | %-10s | %d/%d jugadores\n",
                   i + 1, catacumbas[i].nombre, catacumbas[i].direccion,
                   catacumbas[i].mailbox, catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
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
 * @brief Agrega una nueva catacumba al directorio
 *
 * Procesa el mensaje del cliente que debe contener el nombre, dirección y mailbox
 * de la catacumba en formato "nombre|direccion|mailbox". Los campos de cantidad
 * de jugadores se inicializan automáticamente (cantJug=0, maxJug=0). Valida que
 * no se exceda el límite máximo de catacumbas y que el formato sea correcto.
 *
 * @param catacumbas Array donde se almacenan las catacumbas
 * @param num_catacumbas Puntero al número actual de catacumbas (se incrementa si se agrega)
 * @param msg Mensaje de solicitud con los datos de la catacumba
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

        // Parsear el mensaje en formato "nombre|direccion|mailbox"
        char *nombre = strtok(texto_copia, "|");
        char *direccion = strtok(NULL, "|");
        char *mailbox = strtok(NULL, "|");

        if (nombre != NULL && direccion != NULL && mailbox != NULL)
        {
            // Copiar los campos básicos de la catacumba
            strncpy(catacumbas[*num_catacumbas].nombre, nombre, MAX_NOM - 1);
            catacumbas[*num_catacumbas].nombre[MAX_NOM - 1] = '\0';

            strncpy(catacumbas[*num_catacumbas].direccion, direccion, MAX_RUTA - 1);
            catacumbas[*num_catacumbas].direccion[MAX_RUTA - 1] = '\0';

            strncpy(catacumbas[*num_catacumbas].mailbox, mailbox, MAX_NOM - 1);
            catacumbas[*num_catacumbas].mailbox[MAX_NOM - 1] = '\0';

            // Inicializar automáticamente los campos de jugadores
            // Estos se actualizarán consultando la dirección de la catacumba
            catacumbas[*num_catacumbas].cantJug = 0;
            catacumbas[*num_catacumbas].cantMaxJug = 0;

            (*num_catacumbas)++; // Incrementar el contador

            printf("   ├─ Nombre:     \"%s\"\n", nombre);
            printf("   ├─ Dirección:  \"%s\"\n", direccion);
            printf("   ├─ Mailbox:    \"%s\"\n", mailbox);
            printf("   └─ Estado:     Inicializada (0/0 jugadores)\n");
            printf("\n✅ Catacumba agregada correctamente (Total: %d/%d)\n\n", *num_catacumbas, MAX_CATACUMBAS);

            // Configurar respuesta exitosa
            resp->codigo = RESP_OK;
            strcpy(resp->datos, "Catacumba agregada correctamente.");
        }
        else
        {
            // Error: formato incorrecto
            printf("   ❌ Error: formato incorrecto - faltan campos.\n");
            printf("      Formato esperado: 'nombre|direccion|mailbox'\n\n");
            resp->codigo = RESP_ERROR;
            strcpy(resp->datos, "Error: formato incorrecto. Use 'nombre|direccion|mailbox'");
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
 * "nombre|direccion|mailbox|cantJug|maxJug"; si no, informa que no fue encontrada.
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
            printf("   ├─ Nombre:     \"%s\"\n", catacumbas[i].nombre);
            printf("   ├─ Dirección:  \"%s\"\n", catacumbas[i].direccion);
            printf("   ├─ Mailbox:    \"%s\"\n", catacumbas[i].mailbox);
            printf("   └─ Jugadores:  %d/%d\n", catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
            printf("\n✅ Catacumba encontrada y datos enviados\n\n");

            resp->codigo = RESP_OK;
            resp->num_elementos = 1;
            snprintf(resp->datos, MAX_DAT_RESP, "%s|%s|%s|%d|%d",
                     catacumbas[i].nombre, catacumbas[i].direccion,
                     catacumbas[i].mailbox, catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
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
    }
    else
    {
        // Catacumba no encontrada
        printf("   ❌ Catacumba no encontrada para eliminar.\n\n");
        resp->codigo = RESP_NO_ENCONTRADO;
        snprintf(resp->datos, MAX_DAT_RESP, "Catacumba '%.40s' no encontrada.", msg->texto);
    }
}
