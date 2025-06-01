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
#define MAX_RUTA 200

/** @brief Longitud máxima para el texto de un mensaje */
#define MAX_TEXT 100

/** @brief Longitud máxima para los datos de una respuesta */
#define MAX_DAT_RESP 256

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
    char nombre[MAX_NOM];     /**< Nombre único identificador de la catacumba */
    char direccion[MAX_RUTA]; /**< Ruta al archivo de memoria compartida de la catacumba */
};

/**
 * @brief Estructura para mensajes de solicitud (cliente -> servidor)
 *
 * Los clientes envían estas solicitudes al servidor para realizar operaciones
 * sobre el directorio de catacumbas.
 */
struct solicitud
{
    long mtype;           /**< Tipo de mensaje (requerido por las funciones msgrcv/msgsnd) */
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
    long mtype;               /**< Tipo de mensaje (requerido por las funciones msgrcv/msgsnd) */
    int codigo;               /**< Código de respuesta (RESP_OK, RESP_ERROR, etc.) */
    char datos[MAX_DAT_RESP]; /**< Datos de respuesta (lista de catacumbas, mensajes, etc.) */
    int num_elementos;        /**< Número de elementos en la respuesta (útil para listados) */
};

#endif // DIRECTORIO_H
