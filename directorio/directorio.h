#ifndef DIRECTORIO_H
#define DIRECTORIO_H

/**
 * @file directorio.h
 * @brief Definiciones y estructuras para el sistema de directorio de catacumbas
 *
 * Este archivo contiene todas las constantes, códigos de operación y estructuras
 * necesarias para la comunicación entre el servidor de directorio y los clientes.
 * El sistema utiliza colas de mensajes (message queues) para la comunicación IPC.
 */

// ==================== CONSTANTES DEL SISTEMA ====================

/** @brief Número máximo de catacumbas que puede manejar el directorio */
#define MAX_CATACUMBAS 10

/** @brief Longitud máxima para el nombre de una catacumba */
#define MAX_NOM 50

/** @brief Clave para el mailbox de solicitudes (cliente -> servidor) */
#define MAILBOX_KEY 12345

/** @brief Clave para el mailbox de respuestas (servidor -> cliente) */
#define MAILBOX_RESPUESTA_KEY 12346

/** @brief Longitud máxima para la ruta de una catacumba */
#define MAX_RUTA 100

/** @brief Longitud máxima para el texto de un mensaje */
#define MAX_TEXT 4096

/** @brief Longitud máxima para los datos de una respuesta */
#define MAX_DAT_RESP 4096

/** @brief Archivo donde se persisten las catacumbas */
#define ARCHIVO_CATACUMBAS "catacumbas_persistidas.dat"

/** @brief Archivo de configuración del directorio */
#define ARCHIVO_CONFIG "directorio_config.txt"

/** @brief Frecuencia de ping a catacumbas en segundos */
#define FRECUENCIA_PING 3

// ==================== CÓDIGOS DE OPERACIÓN ====================
// Estos códigos identifican qué operación quiere realizar el cliente

/** @brief Operación: Listar todas las catacumbas registradas */
#define OP_LISTAR 1

/** @brief Operación: Agregar una nueva catacumba al directorio */
#define OP_AGREGAR 2

/** @brief Operación: Buscar una catacumba específica por nombre */
#define OP_BUSCAR 3

/** @brief Operación: Eliminar una catacumba del directorio */
#define OP_ELIMINAR 4

// ==================== CÓDIGOS DE RESPUESTA ====================
// Estos códigos indican el resultado de la operación solicitada

/** @brief Respuesta: Operación ejecutada exitosamente */
#define RESP_OK 1

/** @brief Respuesta: Error durante la ejecución de la operación */
#define RESP_ERROR 2

/** @brief Respuesta: La catacumba solicitada no fue encontrada */
#define RESP_NO_ENCONTRADO 3

/** @brief Respuesta: Se alcanzó el límite máximo de catacumbas */
#define RESP_LIMITE_ALCANZADO 4

// ==================== ESTRUCTURAS DE DATOS ====================

/**
 * @brief Estructura que representa una catacumba en el directorio
 *
 * Contiene la información básica de cada catacumba registrada:
 * el nombre identificador y la dirección/ruta donde se encuentra.
 */
struct catacumba
{
    int pid;                      /**< PID del proceso que maneja la catacumba */
    char nombre[MAX_NOM];         /**< Nombre único identificador de la catacumba */
    char direccion[MAX_RUTA];     /**< Ruta al archivo de memoria compartida de la catacumba */
    char propCatacumba[MAX_RUTA]; /**< Ruta al archivo de memoria compartida de las propiedades la catacumba */
    char mailbox[MAX_NOM];        /**< Ruta al mailbox de mensajes de la catacumba */
    int cantJug;                  /**< Cantidad de jugadores en la catacumba */
    int cantMaxJug;               /**< Cantidad máxima de jugadores permitidos en la catacumba */
};

/**
 * @brief Estructura para mensajes de solicitud (cliente -> servidor)
 *
 * Los clientes envían estas solicitudes al servidor para realizar operaciones
 * sobre el directorio de catacumbas.
 */
struct solicitud
{
    long mtype;           /**< PID del cliente (requerido por las funciones msgrcv/msgsnd) */
    int tipo;             /**< Código de operación (OP_LISTAR, OP_AGREGAR, etc.) */
    char texto[MAX_TEXT]; /**< Datos adicionales según la operación (nombre, dirección, etc.) */
};

/**
 * @brief Estructura para mensajes de respuesta (servidor -> cliente)
 *
 * El servidor envía estas respuestas a los clientes para informar
 * el resultado de las operaciones solicitadas.
 */
struct respuesta
{
    long mtype;               /**< PID del cliente destinatario (requerido por msgrcv/msgsnd) */
    int codigo;               /**< Código de respuesta (RESP_OK, RESP_ERROR, etc.) */
    char datos[MAX_DAT_RESP]; /**< Datos de respuesta (lista de catacumbas, mensajes, etc.) */
    int num_elementos;        /**< Número de elementos en la respuesta (útil para listados) */
};

// ==================== PROTOTIPOS DE FUNCIONES DE PERSISTENCIA ====================

/**
 * @brief Carga las catacumbas desde el archivo de persistencia
 * @param catacumbas Array donde se cargarán las catacumbas
 * @param num_catacumbas Puntero al contador de catacumbas (se actualiza)
 * @return 0 si se carga correctamente, -1 si hay error o no existe el archivo
 */
int cargarCatacumbas(struct catacumba catacumbas[], int *num_catacumbas);

/**
 * @brief Guarda las catacumbas actuales en el archivo de persistencia
 * @param catacumbas Array de catacumbas a guardar
 * @param num_catacumbas Número de catacumbas en el array
 * @return 0 si se guarda correctamente, -1 si hay error
 */
int guardarCatacumbas(struct catacumba catacumbas[], int num_catacumbas);

#endif // DIRECTORIO_H
