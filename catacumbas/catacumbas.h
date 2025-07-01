#ifndef CATACUMBAS_H
#define CATACUMBAS_H


// =====================================
//   VALORES DEL MAPA
// =====================================

/** @brief Caracter que indica una pared en el mapa */
#define PARED '#'

/** @brief Caracter que indica un espacio vacío y transitable en el mapa */
#define VACIO ' '

/** @brief Caracter que indica un tesoro en el mapa */
#define TESORO '$'

/** @brief Caracter que indica un explorador en el mapa */
#define RAIDER 'E'

/** @brief Caracter que indica un guardián en el mapa */
#define GUARDIAN 'G'

// =====================================
//   TAMAÑOS MÁXIMOS
// =====================================

/** @brief Cantidad máxima de tesoros en el mapa */
#define MAX_TESOROS 50

/** @brief Cantidad máxima de guardianes en el mapa */
#define MAX_GUARDIANES 25

/** @brief Cantidad máxima de exploradores en el mapa */
#define MAX_EXPLORADORES 25

/** @brief Cantidad máxima de jugadores en total, en el mapa */
#define MAX_JUGADORES (MAX_GUARDIANES + MAX_EXPLORADORES)

/** @brief Longitud máxima para una ruta de archivo */
#define MAX_LONGITUD_NOMBRE_RUTAS 50

/** @brief Longitud máxima para texto de un mensaje*/
#define MAX_LONGITUD_MENSAJES 50

/** @brief Altura de caracteres del mapa */
#define FILAS 25

/** @brief Ancho de caracteres del mapa */
#define COLUMNAS 80

/** @brief Prefijo para el nombre de memoria compartida del mapa */
#define MEMORIA_MAPA_PREFIJO "/servidor-mapa-"

/** @brief Prefijo para el nombre de memoria compartida del estado del servidor */
#define MEMORIA_ESTADO_PREFIJO "/servidor-estado-"

/** @brief Sufijo para la clave del mailbox de solicitudes */
#define MAILBOX_SOLICITUDES_SUFIJO 10

// =====================================
//   CÓDIGOS DE ACCIONES
// =====================================

/** @brief Código de error, para enviar a los clientes */
#define ERROR -1

/** @brief Código de éxito, para enviar a los clientes */
#define S_OK 1

/** @brief Código de partida terminada por victoria de los exploradores, para enviar a los clientes */
#define SIN_TESOROS 2

/** @brief Código de partida terminada por victoria de los guardianes, para enviar a los clientes */
#define SIN_RAIDERS 3

/** @brief Código de desconexión por muerte, para enviar a un cliente que perdió */
#define MUERTO 4  

/** @brief Código de solicitud de conexión, que se recibe de un cliente */
#define CONEXION 5

/** @brief Código de solicitud de desconexión, que se recibe de un cliente */
#define DESCONEXION 6

/** @brief Código de movimiento de un jugador, que se recibe de un cliente */
#define MOVIMIENTO 7

/** @brief Código de tesoro capturado por un jugador, que se recibe de un cliente */
#define TESORO_CAPTURADO 8

/** @brief Código de explorador capturado por un jugador, que se recibe de un cliente */
#define RAIDER_CAPTURADO 9

/**
 * @brief Estructura para almacenar una posición en el mapa.
 * Contiene la fila y columna.
 * 
 */
struct Posicion {
    int fila;
    int columna;
};

/**
 * @brief Estructura para almacenar un jugador.
 * Contiene la posición en la que se encuentra dentro del mapa,
 * su PID (que es interpretado como su identificador único), 
 * la clave de su mailbox de respuestas, y el caracter
 * que determina el tipo de jugador.
 * 
 */
struct Jugador {
    long pid;
    struct Posicion posicion;
    int clave_mailbox_respuestas;
    char tipo;
};

/**
 * @brief Estructura para almacenar un tesoro.
 * Contiene la posición y un ID.
 * 
 */
struct Tesoro {
    int id;
    struct Posicion posicion;
};

/**
 * @brief Estructura para almacenar el estado del juego.
 * Contiene las cantidades de jugadores, exploradores, guardianes y tesoros.
 * 
 */
struct Estado {
    int max_jugadores;
    int cant_jugadores;
    int cant_raiders;
    int cant_guardianes;
    int cant_tesoros;
};

/**
 * @brief Estructura para transferir solicitudes de clientes a servidores.
 * Contiene el código de operación, la clave del mailbox de respuestas del cliente,
 * la fila y columna en caso de comunicar un movimiento, el tipo de jugador, y 
 * su PID en el campo mtype.
 * 
 */
struct SolicitudServidor {
    long mtype; 
    int codigo;
    int clave_mailbox_respuestas; 
    int fila; 
    int columna; 
    char tipo;
};

/**
 * @brief Estructura para transferir respuestas de servidores a clientes.
 * Contiene el código de operación, el PID del cliente en el campo mtype,
 * y un mensaje.
 * 
 */
struct RespuestaServidor {
    long mtype;
    int codigo;
    char mensaje[MAX_LONGITUD_MENSAJES];
};

/**
 * @brief Estructura para almacenar la Arena o catacumba del servidor.
 * Contiene a los jugadores, tesoros, el mapa y su estado, y la configuración
 * de cantidades máximas
 * 
 */
struct Arena {
    struct Tesoro tesoros[MAX_TESOROS];
    struct Jugador jugadores[MAX_JUGADORES];
    struct Estado *estado;       
    char (*mapa)[COLUMNAS];
    int max_guardianes;
    int max_exploradores;
    int max_tesoros;
    int size_mapa;
    int size_estado;
};

/**
 * @brief Estructura para almacenar datos de comunicación con procesos.
 * Contiene nombres de memorias compartidas, claves e IDs de mailboxes, y file descriptors.
 * 
 */
struct Comunicacion {
    char memoria_mapa_nombre[128];
    char memoria_estado_nombre[128];
    int mailbox_solicitudes_clave;
    int memoria_mapa_fd;
    int memoria_estado_fd;
    int mailbox_solicitudes_id;
    int mailbox_directorio_solicitudes_id;
    int mailbox_directorio_respuestas_id;
};

#endif