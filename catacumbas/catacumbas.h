#ifndef CATACUMBAS_H
#define CATACUMBAS_H


// VALORES MAPA
#define PARED '#'
#define VACIO ' '
#define TESORO '$'
#define RAIDER 'R'
#define GUARDIAN 'G'

// TAMAÑOS MÁXIMOS (restricciones de configuraciones)
#define MAX_TESOROS 50
#define MAX_JUGADORES 50

// LONG. DE STRINGS
#define MAX_LONGITUD_NOMBRE_RUTAS 50
#define MAX_LONGITUD_NOMBRE_JUGADOR 10
#define MAX_LONGITUD_MENSAJES 50

// DIMENSION DEL MAPA
#define FILAS 25
#define COLUMNAS 80

// PREFIJO RECURSOS COMPARTIDOS
#define MEMORIA_MAPA_PREFIJO "/servidor-mapa-"
#define MEMORIA_ESTADO_PREFIJO "/servidor-estado-"
#define MAILBOX_SOLICITUDES_SUFIJO 10

// CODIGOS DE ACCIONES
// Los que envia servidor:
#define ERROR -1
#define S_OK 1
#define SIN_TESOROS 2
#define SIN_RAIDERS 3
#define MUERTO 4        // te capturaron
// Los que envian los clientes:
#define CONEXION 5
#define DESCONEXION 6
#define MOVIMIENTO 7
#define TESORO_CAPTURADO 8
#define RAIDER_CAPTURADO 9

struct Posicion {
    int fila;
    int columna;
};

struct Jugador {
    long pid;
    struct Posicion posicion;
    int clave_mailbox_respuestas;
    // char nombre[MAX_LONGITUD_NOMBRE_JUGADOR];
    // int cantidadCapturada; // tesoro o exploradores
    char tipo;
};

struct Tesoro {
    int id;
    struct Posicion posicion;
};

struct Estado {
    int max_jugadores;
    int cant_jugadores;
    int cant_raiders;
    int cant_guardianes;
    int cant_tesoros;
};

// Para comunicación con clientes
// Puede cambiar segun lo que Cliente haga o tenga
struct SolicitudServidor {
    long mtype; // el PID del cliente
    int codigo; // codigos especiales para tipos de mensajes
    int clave_mailbox_respuestas; // Mailbox del cliente
    int fila; // Posición
    int columna; 
    char tipo; // Raider o guardian
};

struct RespuestaServidor {
    long mtype;
    int codigo;
    char mensaje[MAX_LONGITUD_MENSAJES];
};

// mapa (jugadores, tesoros, estado) con sus reglas
struct Arena {
    struct Tesoro tesoros[MAX_TESOROS];
    struct Jugador jugadores[MAX_JUGADORES];
    struct Estado *estado;       // Necesario si usa mmap
    char (*mapa)[COLUMNAS];      // Tambien aca
    int max_guardianes;
    int max_raiders;
    int max_tesoros;
    int size_mapa;
    int size_estado;
};

// memoria compartida y mensajeria
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