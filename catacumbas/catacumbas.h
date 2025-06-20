#ifndef CATACUMBAS_H
#define CATACUMBAS_H

// VALORES MAPA
#define PARED '#'
#define VACIO ' '
#define TESORO '$'
#define RAIDER 'R'
#define GUARDIAN 'G'

// LONG. DE STRINGS
#define MAX_LONGITUD_NOMBRE_RUTAS 50
#define MAX_LONGITUD_NOMBRE_JUGADOR 10
#define MAX_LONGITUD_MENSAJES 50

//  REGLAS DEL JUEGO
#define MAX_RAIDERS 8
#define MAX_GUARDIANES 8
#define MAX_JUGADORES MAX_GUARDIANES + MAX_RAIDERS
#define MAX_TESOROS 10

// DIMENSION DEL MAPA
#define FILAS 25
#define COLUMNAS 80

// PREFIJO RECURSOS COMPARTIDOS
#define MEMORIA_MAPA_PREFIJO "/servidor-mapa-"
#define MEMORIA_ESTADO_PREFIJO "/servidor-estado-"
#define MAILBOX_SOLICITUDES_SUFIJO 10

// ACCIONES
#define CONEXION 1
#define DESCONEXION 2
#define MOVERSE 3
#define NOTIFICACION 4

struct Posicion {
    int fila;
    int columna;
};

struct Jugador {
    long pid;
    struct Posicion posicion;
    char nombre[MAX_LONGITUD_NOMBRE_JUGADOR];
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

#endif