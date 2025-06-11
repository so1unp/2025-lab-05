#ifndef CATACUMBAS_H
#define CATACUMBAS_H

// VALORES MAPA
#define PARED -1
#define VACIO 0
#define TESORO_OFFSET 1

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
#define SHM_MAPA_PREFIX "/mapa_memoria_"
#define SHM_ESTADO_PREFIX "/estado_memoria_"

// TIPO JUGADOR
#define RAIDER 'R'
#define GUARDIAN 'G'

// ACCIONES
#define CONECTAR 1
#define MOVERSE 2
#define DESCONECTAR 3
#define NOTIFICACION 4

// CLAVES
#define MAILBOX_SOLICITUD_KEY 12345
#define MAILBOX_MOVIMIENTO_KEY 12346

#define TOTAL_CATACUMBAS 10

const char* catacumbas[TOTAL_CATACUMBAS] = {
    "stack_overflow_abyss",
    "segfault_sanctum",
    "kernel_panic_crypts",
    "syscall_shrine",
    "mmu_maze",
    "dev_null_vaults",
    "trapframe_tomb",
    "bus_error_bastion",
    "deadlock_dungeons",
    "segmented_shadows"
};

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
struct SolicitudConexion {
    long mtype; // IPC lo requiere
    struct Jugador jugador;
    int clave_mailbox_respuestas; //mailbox del cliente
    int clave_mailbox_notificaciones; //mailbox del cliente
};

struct RespuestaConexion {
    long mtype;
    char mensaje[MAX_LONGITUD_MENSAJES];
    int clave_mailbox_movimientos;
    char nombre_memoria_mapa[MAX_LONGITUD_NOMBRE_RUTAS];
};

struct Movimiento {
    long pid_cliente;
    struct Posicion posicion;
};

struct Notificacion {
    int codigo;
    char mensaje[MAX_LONGITUD_MENSAJES];
};

#endif